#include "lwsapi.h"
#include "http_states.h"
#include <cstring>

#include "../Marlin/src/module/temperature.h"

#ifdef RFC1123_DATETIME
	#define LAST_MODIFY RFC1123_DATETIME
#else
	#define LAST_MODIFY __TIMESTAMP__
#endif

#include "Res/cc/index_html.c"
#include "Res/cc/index_css.c"
#include "Res/cc/index_js.c"
#include "Res/cc/favicon_ico.c"
#include "Res/cc/connect_black_svg.c"

#include "Res/cc/status_filament_svg.c"
#include "Res/cc/status_heatbed_svg.c"
#include "Res/cc/status_nozzle_svg.c"
#include "Res/cc/status_prnflow_svg.c"
#include "Res/cc/status_prnspeed_svg.c"
#include "Res/cc/status_z_axis_svg.c"

struct FileHandler_t {
	const char* file_name;
	const char* content_type;
	int content_length;
	const uint8_t* data;
};

const FileHandler_t files[] = {
	{"/", "text/html", sizeof(index_html), index_html},
	{"/index.css", "text/css", sizeof(index_css), index_css},
	{"/index.js", "application/javasript", sizeof(index_js), index_js},
	{"/favicon.ico", "image/x-icon", sizeof(favicon_ico), favicon_ico},
	{"/connect_black.svg", "image/svg+xml", sizeof(connect_black_svg),
		connect_black_svg},
	{"/status_filament.svg", "image/svg+xml", sizeof(status_filament_svg),
		status_filament_svg},
	{"/status_heatbed.svg", "image/svg+xml", sizeof(status_heatbed_svg),
		status_heatbed_svg},
	{"/status_nozzle.svg", "image/svg+xml", sizeof(status_nozzle_svg),
		status_nozzle_svg},
	{"/status_prnflow.svg", "image/svg+xml", sizeof(status_prnflow_svg),
		status_prnflow_svg},
	{"/status_prnspeed.svg", "image/svg+xml", sizeof(status_prnspeed_svg),
		status_prnspeed_svg},
	{"/status_z_axis.svg", "image/svg+xml", sizeof(status_z_axis_svg),
		status_z_axis_svg},
};

enum State_t {
	FIRST,
	NEXT,
	DONE
};

struct FileResponse_t {

	const char* response;
	char clength[11];		// 2^32+1
	Header_t headers[4];
	const FileHandler_t* handler;
	bool done;
	static constexpr const char* last_modified = LAST_MODIFY;

	FileResponse_t(const FileHandler_t* handler):
			response(HTTP_200), handler(handler), done(false)
	{
		snprintf(clength, 11, "%d", handler->content_length);

		headers[0] = (Header_t){"Content-Type", handler->content_type,
				&headers[1]};
		headers[1] = (Header_t){"Content-Length", clength, &headers[2]};
		headers[2] = (Header_t){"Last-Modify", last_modified, &headers[3]};
		headers[3] = (Header_t){"Cache-Control", "public, max-age=31536000",
				nullptr};
	}

	~FileResponse_t()
	{}
};

struct BufferResponse_t {
	const char* response = nullptr;
	Header_t* headers = nullptr;
	char buffer[256] = {'\0'};
	bool done = false;

	~BufferResponse_t()
	{
		free(headers);
	}
};

Message_t file_coroutine(void* arg)
{
	Message_t msg = {nullptr, nullptr, nullptr, EOF};

	if (arg != nullptr)
	{
		FileResponse_t* res = (FileResponse_t*)arg;
		if (res->done){
			delete res;
		} else {
			msg = {res->response, res->headers, res->handler->data,
					res->handler->content_length};
			res->done = true;
		}
	}
	return msg;
}

Message_t buffer_coroutine(void* arg)
{
	//const char* dummny = "This is dummy data. ";
	Message_t msg = {nullptr, nullptr, nullptr, EOF};

	if (arg != nullptr)
	{
		BufferResponse_t* res = (BufferResponse_t*)arg;
		if (res->done){
			delete res;
		} else {
			msg = {res->response, res->headers,(const uint8_t*)res->buffer,
					(int)strlen(res->buffer)};
			res->done = true;
		}
	}
	return msg;
}

coroutine_fn api_job(Environment_t* env, void** arg)
{
	BufferResponse_t* res = new BufferResponse_t();
	*arg = res;

	res->response = HTTP_200;
	res->headers = (Header_t*)calloc(1, sizeof(Header_t));
	*res->headers = {"Content-Type", "application/json", nullptr};

	snprintf(res->buffer, 256, "{}");
	return &buffer_coroutine;
}


coroutine_fn api_printer(Environment_t* env, void** arg)
{
	BufferResponse_t *res = new BufferResponse_t();
	*arg = res;

	res->response = HTTP_200;
	res->headers = (Header_t*)calloc(1, sizeof(Header_t));
	*res->headers = {"Content-Type", "application/json", nullptr};

	#if 1
	float actual_nozzle = thermalManager.degHotend(0);
	float target_nozzle = thermalManager.degTargetHotend(0);
	float actual_heatbed = thermalManager.degBed();
	float target_heatbed = thermalManager.degTargetBed();
	#else
	float actual_nozzle = 26;
	float target_nozzle = 32;
	float actual_heatbed = 55;
	float target_heatbed = 16;
	#endif

	snprintf(res->buffer, 256,
		"{\"temperature\":{"
			"\"tool0\":{\"actual\":%f, \"target\":%f},"
			"\"bed\":{\"actual\":%f, \"target\":%f}}}",
		(double)actual_nozzle, (double)target_nozzle,
		(double)actual_heatbed, (double)target_heatbed);

	return &buffer_coroutine;
}

coroutine_fn not_found(Environment_t* env, void** arg)
{
	BufferResponse_t *res = new BufferResponse_t();
	*arg = res;

	res->response = HTTP_404;
	res->headers = (Header_t*)calloc(1, sizeof(Header_t));
	*res->headers = {"Content-Type", "text/plain", nullptr};

	snprintf(res->buffer, 256, "Not Found");

	return &buffer_coroutine;
}


coroutine_fn application(Environment_t * env, void** arg){

	printf("HTTP Request: %s %s\n", env->method, env->request_uri);

	/*
	char header[64] = {'\0'};

	auto it = env->headers;
	while (it){
		printf("\t`%s:%s'\n", it->key, it->value);
		it = it->next;
	}
	*/


	// Static files
	for (size_t i=0; i<(sizeof(files)/sizeof(FileHandler_t)); i++)
	{
		if (!strcmp(env->request_uri, files[i].file_name))
		{
			*arg = (void*)new FileResponse_t(&files[i]);
			return &file_coroutine;
		}
	}

	if (!strcmp(env->request_uri, "/api/job")){
		return api_job(env, arg);
	} else if (!strcmp(env->request_uri, "/api/printer")){
		return api_printer(env, arg);
	}

	return not_found(env, arg);
}
