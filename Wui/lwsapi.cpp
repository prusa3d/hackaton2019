
#include <cstring>
#include "dbg.h"

#include "lwsapi.h"
#include "../Common/marlin_client.h"

#define LIGHT_WSAPI_PORT 80
#define LIGHT_WSAPI_RETRIES 4              // 8 seconds timeout
#define LIGHT_WSAPI_POLL_INTERVAL 4        // once a two seconds

#define lwsapi_dbg _dbg
#define lwsapi_error _dbg

static const char* HTTP_10 = "HTTP/1.0";

struct Context_t {
	struct tcp_pcb * pcb;
	struct pbuf * input;		/*< wsapi input >*/
	uint8_t retries;			/*< retries >*/
	Environment_t env;			/**< Request WSAPI environment */
	void* coroutine_arg;		/**< Argument set by application */
	coroutine_fn coroutine;		/**< Coroutine returned by application */
	Message_t message;			/**< Last message returned from application */
	char* buffer;				/**< Buffer for internal output data */
	size_t m_position;			/**< Position of data in last message which is
									 write not yet. */

	Context_t(struct tcp_pcb * pcb)
		:pcb(pcb),input(nullptr),retries(0),coroutine_arg(nullptr),
		coroutine(nullptr),message({nullptr, nullptr, nullptr, 0}),
		buffer(nullptr)
	{
		tcp_arg(pcb, this);
		/* Environment_t C struct constructor */
		env.method = nullptr;
		env.request_uri = nullptr;
		env.headers = nullptr;
		env.body = nullptr;
	}

	~Context_t()
	{
		/* Destructor for the Environment_t struct which is pure C. */
		free(env.method);
		free(env.body);
		free(env.request_uri);
		Header_t* it = nullptr;
		while (env.headers != nullptr){
			it = env.headers->next;
			free(const_cast<char*>(env.headers->key));
			free(const_cast<char*>(env.headers->value));
			delete(env.headers);
			env.headers = it;
		}
		if (input != nullptr){
			//pbuf_free(input);
		}
		tcp_arg(this->pcb, nullptr);
	}

	err_t parse_request(struct pbuf *p);
	err_t parse_headers(struct pbuf *p, char **headersEnd);
	err_t parse_body(struct pbuf *p, char *headersEnd);
};


static inline bool empty_message(const Message_t& msg)
{
	return (msg.response == nullptr && msg.headers == nullptr && msg.length == 0);
}

static size_t lwsapi_write(Context_t* ctx, const uint8_t* data, size_t len);
static inline size_t lwsapi_write(Context_t* ctx, const char * data)
{
	return lwsapi_write(ctx, (const uint8_t*)(data), strlen(data));
}

//! Proccess message from coroutine_fn, and call coroutine_fn for next work
static void lwsapi_call(Context_t* ctx);

static err_t check_len(size_t len, size_t p_len)
{
	if (len > p_len) {		// len or tot-len ?!
		lwsapi_error("parse_headers: End of line not found!");
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

	env.method = (char*)calloc(len+1, sizeof(char));
 	memcpy(env.method, start, len);

	start = end + 1;
	end = strchr(start,' ');
	len = end - start;
	env.request_uri = (char*)calloc(len+1, sizeof(char));
 	memcpy(env.request_uri, start, len);

	return ERR_OK;
}

err_t Context_t::parse_headers(struct pbuf *p, char **headersEnd)
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
		header->key = (char*)calloc(siz+1, sizeof(char));
		memcpy(const_cast<char*>(header->key), start, siz);

		key +=2;					// skip: ": "
		val = strchr(key, '\r');
		siz = val - key;
		header->value = (char*)calloc(siz+1, sizeof(char));
		memcpy(const_cast<char*>(header->value), key, siz);

		start = end + 2;			// skip full LF CR
		end = strchr(start, '\r');
		len = end - start;
		readed += len;

		if (check_len(readed, p->len) != ERR_OK){
			return ERR_VAL;
		}
	}

	// TODO: set payload start position to end +1
	*headersEnd = end+1;

	return ERR_OK;
}

err_t Context_t::parse_body(struct pbuf *p, char *headersEnd)
{
	//char *start = (char*)p->payload;
	char *start = headersEnd;
	if(*start == '\n')
		start++;

	int bodyLen = p->len - (headersEnd-(char*)p->payload);

	env.body = (char*)malloc(bodyLen+1);
	memcpy(env.body, start, bodyLen);
	env.body[bodyLen]='\0';

	return ERR_OK;
}

static err_t lwsapi_poll(void *arg, struct tcp_pcb *pcb);

const char * http_bad_request =
	"HTTP/1.0 400 Bad Request\n"
	"Server: LwIP WSAPI\n"
	"Content-Type: text/plain\n"
	"\n"
	"Bed Request";

const char * http_internal_server =
	"HTTP/1.0 500 Internal Server Error\n"
	"Server: LwIP WSAPI\n"
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
		tcp_poll(pcb, lwsapi_poll, LIGHT_WSAPI_POLL_INTERVAL);
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
static err_t lwsapi_poll(void *arg, struct tcp_pcb *pcb)
{
	if (arg == nullptr){
		if (close_conn(pcb) == ERR_MEM){
			tcp_abort(pcb);
			return ERR_ABRT;		// abort in close memmory error
		}
	}

	Context_t * ctx = static_cast<Context_t *>(arg);
	ctx->retries++;
	if (ctx->retries == LIGHT_WSAPI_RETRIES) {
		lwsapi_dbg("\tmax retries, close\n");
		return close_conn(pcb, ctx);
	}

	return ERR_OK;
}

//! process message.response to internal ctx->buffer
static void lwsapi_prepare_response(Context_t* ctx)
{
	// HTTP/1.0\ 200 OK\r\n\0
	size_t size = strlen(HTTP_10) + strlen(ctx->message.response) + 4;
	// TODO: check size
	ctx->buffer = (char*)calloc(size, sizeof(char));
	snprintf(ctx->buffer, size, "%s %s\r\n", HTTP_10, ctx->message.response);
	ctx->message.response = nullptr;
}

//! process message.headers to internal ctx->buffer
static void lwsapi_prepare_header(Context_t* ctx)
{
	const Header_t* header = ctx->message.headers;
	// Key\:\ Value\r\n\r\n\0
	size_t size = strlen(header->key) + strlen(header->value) + 7;
	// TODO: check size
	ctx->buffer = (char*)calloc(size, sizeof(char));
	if (header->next != nullptr) {
		snprintf(ctx->buffer, size, "%s: %s\r\n",
				header->key, header->value);
	} else {  // last header needs one CRLF before payload
		snprintf(ctx->buffer, size, "%s: %s\r\n\r\n",
				header->key, header->value);
	}
	ctx->message.headers = header->next;
}

static void lwsapi_call(Context_t* ctx)
{
	while (1) {
		// internal buffer and message was sent, call the coroutine
		if (ctx->buffer == nullptr && empty_message(ctx->message))
		{
			ctx->message = ctx->coroutine(ctx->coroutine_arg);
			ctx->m_position = 0;
			if (ctx->message.length == EOF){
				close_conn(ctx->pcb, ctx);
				return;
			}
		}

		// try to send internal buffer if exists
		if (ctx->buffer != nullptr) {
			size_t size = strlen(ctx->buffer+ctx->m_position);
			size_t send = lwsapi_write(ctx,
					(const uint8_t*)ctx->buffer+ctx->m_position, size);
			if (send == size) {
				free(ctx->buffer);
				ctx->buffer = nullptr;
				ctx->m_position = 0;
			} else {  // not send all data, tcp buffer is full
				ctx->m_position += send;
				tcp_output(ctx->pcb);
				return;
			}
		}

		if (ctx->message.response != nullptr){
			lwsapi_prepare_response(ctx);
			continue;
		}

		if (ctx->message.headers != nullptr){
			lwsapi_prepare_header(ctx);
			continue;
		}

		size_t send = lwsapi_write(ctx, ctx->message.payload+ctx->m_position,
				ctx->message.length);
		ctx->m_position += send;
		ctx->message.length -= send;
		if (ctx->message.length > 0)
		{
			tcp_output(ctx->pcb);	// flush the data => free the buffer
			return;
		}
	}
}

static size_t lwsapi_write(Context_t* ctx, const uint8_t* data, size_t len)
{
	if (ctx == nullptr)
	{
		lwsapi_error("lwsapi_write: Bad input!\n");
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
			lwsapi_dbg("[%p] lwsapi_write: tcp_write error: %d\n", ctx->pcb, err);
			return offset;
		}
	}

	return 0;     // len is zero
}


static err_t lwsapi_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
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
		lwsapi_write(ctx, http_bad_request);
		tcp_output(pcb);
		return close_conn(pcb, ctx);
	}
	char *headersEnd;
	if (ctx->parse_headers(p, &headersEnd) != ERR_OK){
		lwsapi_write(ctx, http_bad_request);
		tcp_output(pcb);
		return close_conn(pcb, ctx);
	}
	if (ctx->parse_body(p, headersEnd) != ERR_OK){
		lwsapi_write(ctx, http_bad_request);
		tcp_output(pcb);
		return close_conn(pcb, ctx);
	}
	ctx->coroutine = application(&ctx->env, &ctx->coroutine_arg);
	pbuf_free(p);
	lwsapi_call(ctx);		// first try to call coroutine

	return ERR_OK;

}

/**
 * The pcb had an error and is already deallocated.
 * The argument might still be valid (if != nullptr).
 */
static void lwsapi_err(void *arg, err_t err)
{
	// lwsapi_error("lwsapi_err: (%d) %s\n", err, lwip_strerr(err));
	lwsapi_error("lwsapi_err: %d", err);

	if (arg != nullptr) {
		Context_t * ctx = static_cast<Context_t *>(arg);
		delete ctx;
	}
}

/**
 * Data has been sent and acknowledged by the remote host.
 * This means that more data can be sent.
 */
static err_t lwsapi_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	if (arg == nullptr) {
		return ERR_OK;
	}

	Context_t * ctx = static_cast<Context_t *>(arg);
	ctx->retries = 0;

	lwsapi_call(ctx);
	// lwsapi_send(pcb, env); todo: really ?

	return ERR_OK;
}

static err_t lwsapi_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
	if ((err != ERR_OK) || (pcb == nullptr)) {
		return ERR_VAL;
	}

	tcp_setprio(pcb, TCP_PRIO_MIN);
	Context_t *ctx = new Context_t(pcb);
	if (ctx == nullptr) {
		lwsapi_error("lwsapi_accept: Out of memory, RST\n");
		return ERR_MEM;
	}

	/* Set up the various callback functions */
	tcp_recv(pcb, lwsapi_recv);
	tcp_err(pcb, lwsapi_err);
	tcp_poll(pcb, lwsapi_poll, LIGHT_WSAPI_POLL_INTERVAL);
	tcp_sent(pcb, lwsapi_sent);

	return ERR_OK;
}

err_t lwsapi_init(void){
	lwsapi_dbg("lwsapi: start\n");
	struct tcp_pcb *pcb;
	err_t err;

	pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (pcb == nullptr){
		return ERR_MEM;
	}

	tcp_setprio(pcb, TCP_PRIO_MIN);
	err = tcp_bind(pcb, IP_ANY_TYPE, LIGHT_WSAPI_PORT);
	if (err != ERR_OK){
		lwsapi_error("lwsapi: tcp_bind failed: %d", err);
		return err;
	}

	pcb = tcp_listen(pcb);
	if (pcb == nullptr){
		lwsapi_error("lwsapi: tcp_listen failed");
		return ERR_CLSD;
	}

	//TODO sem?
	marlin_client_init();

	tcp_accept(pcb, lwsapi_accept);
	return ERR_OK;
}
