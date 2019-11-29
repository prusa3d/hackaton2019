
#include <cstring>
#include "dbg.h"

#include "lwsgi.h"

#define LIGHT_WSGI_PORT 80
#define LIGHT_WSGI_RETRIES 4              // 8 seconds timeout
#define LIGHT_WSGI_POLL_INTERVAL 4        // once a two seconds

#define lwsgi_dbg _dbg
#define lwsgi_error _dbg

struct Context_t {
	struct tcp_pcb * pcb;
	struct pbuf * input;		/*< wsgi input >*/
	uint8_t retries;			/*< retries >*/
	Environment_t env;			/**< Request WSGI environment */
	void* coroutine_arg;		/**< Argument set by application */
	coroutine_fn coroutine;		/**< Coroutine returned by application */
	Message_t message;			/**< Last message returned from application */
	size_t m_position;			/**< Position of data in last message which is
									 write not yet. */

	Context_t(struct tcp_pcb * pcb)
		:pcb(pcb),input(nullptr),retries(0),coroutine_arg(nullptr),
		coroutine(nullptr)
	{
		tcp_arg(pcb, this);
		/* Environment_t C struct constructor */
		env.method = nullptr;
		env.request_uri = nullptr;
		env.headers = nullptr;
		/* Message_t C struct consturctor */
		message.data = nullptr;
		message.length = 0;
	}

	~Context_t()
	{
		/* Destructor for the Environment_t struct which is pure C. */
		free(env.method);
		free(env.request_uri);
		Header_t* it = nullptr;
		while (env.headers != nullptr){
			it = env.headers->next;
			free(env.headers->key);
			free(env.headers->value);
			free(env.headers);
			env.headers = it;
		}
		if (input != nullptr){
			//pbuf_free(input);
		}
		tcp_arg(this->pcb, nullptr);
	}

	err_t parse_request(struct pbuf *p);
	err_t parse_headers(struct pbuf *p);
};

size_t lwsgi_write(Context_t* ctx, const uint8_t* data, size_t len);
inline size_t lwsgi_write(Context_t* ctx, const char * data)
{
	return lwsgi_write(ctx, (const uint8_t*)(data), strlen(data));
}

void lwsgi_call(Context_t* ctx);


/*
inline size_t lwsgi_write_state(Environment_t* env, const char* state){
	return
		lwsgi_write(env, HTTP_10) +
		lwsgi_write(env, " ") +
		lwsgi_write(env, state) +
		lwsgi_write(env, "\r\n");
}
*/

err_t check_len(size_t len, size_t p_len)
{
	if (len > p_len) {		// len or tot-len ?!
		lwsgi_error("parse_headers: End of line not found!");
		return ERR_VAL;
	}
	return ERR_OK;
}

err_t Context_t::parse_request(struct pbuf *p)
{
	char * start = (char*)p->payload;
	char * end = start;

	end = strchr(start, ' ');
	size_t len = end - start;

	if (check_len(len, p->len) != ERR_OK){
		return ERR_VAL;
	}

	env.method = (char*)calloc(sizeof(char)*len+1, sizeof(char));
 	memcpy(env.method, start, len);

	start = end + 1;
	end = strchr(start,' ');
	len = end - start;
	env.request_uri = (char*)calloc(sizeof(char)*len+1, sizeof(char));
 	memcpy(env.request_uri, start, len);

	return ERR_OK;
}

err_t Context_t::parse_headers(struct pbuf *p)
{
	// TODO: use regex instead of this ...

	char *start = (char*)p->payload;
	char *end = strchr(start, '\r');	// skip first line METHOD /URI HTTPX.Y
	size_t len = end - start;
	size_t readed = len;

	if (check_len(readed, p->len) != ERR_OK){
		return ERR_VAL;
	}

	start = end + 2;		// skip full LR CR
	end = strchr(start, '\r');
	len = end - start;
	readed += len;

	if (check_len(readed, p->len) != ERR_OK){
		return ERR_VAL;
	}

	Header_t *header, *last;
	char *key, *val;		// end position of header key, value chars
	size_t siz;
	while (len > 1) {
		header = new Header_t();
		if (env.headers == nullptr) {		// append to list first!
			env.headers = header;
		} else {
			last->next = header;
		}
		last = header;

		key = strchr(start, ':');
		siz = key - start;
		header->key = (char*)calloc(sizeof(char)*siz+1, sizeof(char));
		memcpy(header->key, start, siz);

		key +=2;					// skip: ": "
		val = strchr(key, '\r');
		siz = val - key;
		header->value = (char*)calloc(sizeof(char)*siz+1, sizeof(char));
		memcpy(header->value, key, siz);

		start = end + 2;			// skip full LF CR
		end = strchr(start, '\r');
		len = end - start;
		readed += len;

		if (check_len(readed, p->len) != ERR_OK){
			return ERR_VAL;
		}
	}

	// TODO: set payload start position to end +1
	return ERR_OK;
}


static err_t lwsgi_poll(void *arg, struct tcp_pcb *pcb);

const char * http_bad_request =
	"HTTP/1.0 400 Bad Request\n"
	"Server: LwIP WSGI\n"
	"Content-Type: text/plain\n"
	"\n"
	"Bed Request";

const char * http_internal_server =
	"HTTP/1.0 500 Internal Server Error\n"
	"Server: LwIP WSGI\n"
	"Content-Type: text/plain\n"
	"\n"
	"Internal Server Error";



static err_t close_conn(struct tcp_pcb* pcb, Context_t* ctx=nullptr)
{
	if (ctx != nullptr) {
		delete ctx;
	}

	tcp_arg(pcb, nullptr);
	tcp_recv(pcb, nullptr);
	tcp_err(pcb, nullptr);
	tcp_poll(pcb, nullptr, 0);
	tcp_sent(pcb, nullptr);

	tcp_output(pcb);		// flush all data before close
	err_t err = tcp_close(pcb);

	if (false) {	// TODO: [need study] http abort
		tcp_abort(pcb);
		return ERR_ABRT;
	}

	if (err != ERR_OK) {
		// close in poll, but could be happened ?
		tcp_poll(pcb, lwsgi_poll, LIGHT_WSGI_POLL_INTERVAL);
	}

	return err;
}

/**
 * The poll function is called every 2nd second.
 * If there has been no data sent (which resets the retries) in 8 seconds, close.
 * If the last portion of a file has not been sent in 1 seconds, close.
 *
 * This could be increased, but we don't want to waste resources for bad connections.
 */
static err_t lwsgi_poll(void *arg, struct tcp_pcb *pcb)
{
	if (arg == nullptr){
		if (close_conn(pcb) == ERR_MEM){
			tcp_abort(pcb);
			return ERR_ABRT;		// abort in close memmory error
		}
	}

	Context_t * ctx = static_cast<Context_t *>(arg);
	ctx->retries++;
	if (ctx->retries == LIGHT_WSGI_RETRIES) {
		lwsgi_dbg("\tmax retries, close\n");
		return close_conn(pcb, ctx);
	}

	return ERR_OK;
}

void lwsgi_call(Context_t* ctx)
{
	if (ctx->message.length == 0){		// message was sent, call the coroutine
		ctx->message = ctx->coroutine(ctx->coroutine_arg);
		ctx->m_position = 0;
		if (ctx->message.length == EOF){
			// tcp_output(ctx->pcb);
			close_conn(ctx->pcb, ctx);
			return;
		}
	}

	size_t send = lwsgi_write(ctx, ctx->message.data+ctx->m_position,
			ctx->message.length);
	ctx->m_position += send;
	ctx->message.length -= send;
	if (ctx->message.length > 0)
	{
		tcp_output(ctx->pcb);	// flush the data => free the buffer
		return;
	}

	// have some space in tcp packet
	lwsgi_call(ctx);
}



size_t lwsgi_write(Context_t* ctx, const uint8_t* data, size_t len)
{
	if (ctx == nullptr)
	{
		lwsgi_error("lwsgi_write: Bad input!\n");
		return 0;
	}

	size_t offset = 0;
	size_t max_len;
	size_t snd_len = len;
	while (offset < len)
	{
		/* We cannot send more data than space available in the send buffer. */
		max_len = tcp_sndbuf(ctx->pcb);
		if (max_len < snd_len){
			snd_len = max_len;
		}

		if (max_len == 0){
			return offset;
		}

		/* Additional limitation: e.g. don't enqueue more than 2*mss at once */
		max_len = ((u16_t)(2 * tcp_mss(ctx->pcb)));
		if (max_len < snd_len) {
			snd_len = max_len;
		}

		err_t err;
		do {
			/* Write packet to out-queue, but do not send it until tcp_output() is called. */
			err = tcp_write(ctx->pcb, data+offset, snd_len, TCP_WRITE_FLAG_COPY);
			if (err == ERR_MEM)
			{
				if ((tcp_sndbuf(ctx->pcb) == 0) ||
					(tcp_sndqueuelen(ctx->pcb) >= TCP_SND_QUEUELEN))
				{
					/* no need to try smaller sizes */
					snd_len = 1;
				} else {
					snd_len /= 2;
				}
			}
		} while ((err == ERR_MEM) && (snd_len > 1));
		if (err == ERR_OK) {
			offset += snd_len;
			return offset;
		} else {
			lwsgi_dbg("[%p] lwsgi_write: tcp_write error: %d\n", ctx->pcb, err);
			return offset;
		}
	}

	return 0;     // len is zero
}


static err_t lwsgi_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	if ((err != ERR_OK) || (p == nullptr) || (arg == nullptr)) {
		/* error or closed by other side? */
		if (p != nullptr) {
			/* Inform TCP that we have taken the data. */
			tcp_recved(pcb, p->tot_len);
			pbuf_free(p);
		}
		return close_conn(pcb, static_cast<Context_t *>(arg));
	}

	/* Inform TCP that we have taken the data. */
	tcp_recved(pcb, p->tot_len);

	Context_t * ctx = static_cast<Context_t *>(arg);
	ctx->input = p;
	if (ctx->parse_request(p) != ERR_OK){
		lwsgi_write(ctx, http_bad_request);
		tcp_output(pcb);
		return close_conn(pcb, ctx);
	}
	if (ctx->parse_headers(p) != ERR_OK){
		lwsgi_write(ctx, http_bad_request);
		tcp_output(pcb);
		return close_conn(pcb, ctx);
	}

	ctx->coroutine = application(&ctx->env, &ctx->coroutine_arg);
	pbuf_free(p);
	lwsgi_call(ctx);		// first try to call coroutine

	return ERR_OK;

}

/**
 * The pcb had an error and is already deallocated.
 * The argument might still be valid (if != nullptr).
 */
static void lwsgi_err(void *arg, err_t err)
{
	lwsgi_error("lwsgi_err: (%d) %s\n", err, lwip_strerr(err));

	if (arg != nullptr) {
		Context_t * ctx = static_cast<Context_t *>(arg);
		delete ctx;
	}
}

/**
 * Data has been sent and acknowledged by the remote host.
 * This means that more data can be sent.
 */
static err_t lwsgi_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	if (arg == nullptr) {
		return ERR_OK;
	}

	Context_t * ctx = static_cast<Context_t *>(arg);
	ctx->retries = 0;

	lwsgi_call(ctx);
	// lwsgi_send(pcb, env); todo: really ?

	return ERR_OK;
}

static err_t lwsgi_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
	if ((err != ERR_OK) || (pcb == nullptr)) {
		return ERR_VAL;
	}

	tcp_setprio(pcb, TCP_PRIO_MIN);
	Context_t *ctx = new Context_t(pcb);
	if (ctx == nullptr) {
		lwsgi_error("lwsgi_accept: Out of memory, RST\n");
		return ERR_MEM;
	}

	/* Set up the various callback functions */
	tcp_recv(pcb, lwsgi_recv);
	tcp_err(pcb, lwsgi_err);
	tcp_poll(pcb, lwsgi_poll, LIGHT_WSGI_POLL_INTERVAL);
	tcp_sent(pcb, lwsgi_sent);

	return ERR_OK;
}

err_t lwsgi_init(void){
	lwsgi_dbg("lwsgi: start\n");
	struct tcp_pcb *pcb;
	err_t err;

	pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (pcb == nullptr){
		return ERR_MEM;
	}

	tcp_setprio(pcb, TCP_PRIO_MIN);
	err = tcp_bind(pcb, IP_ANY_TYPE, LIGHT_WSGI_PORT);
	if (err != ERR_OK){
		lwsgi_error("lwsgi: tcp_bind failed: %d", err);
		return err;
	}

	pcb = tcp_listen(pcb);
	if (pcb == nullptr){
		lwsgi_error("lwsgi: tcp_listen failed");
		return ERR_CLSD;
	}

	tcp_accept(pcb, lwsgi_accept);
	return ERR_OK;
}
