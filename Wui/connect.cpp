#include "lwsgi.h"
#include "http_states.h"
#include <cstring>

#include "../Marlin/src/module/temperature.h"

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
	RESPONSE,
	HEADERS,
	DATA,
	DONE
};

struct FileResponse_t {

	char http_state[50];   // longest response is 444 (47 chars)
	char h_buffer[128];
	const FileHandler_t* handler;
	State_t state = RESPONSE;

	FileResponse_t(const FileHandler_t* handler):handler(handler)
	{
		snprintf(http_state, 50, "%s %s\r\n", HTTP_10, HTTP_200);
		snprintf(
			h_buffer, 128,
			"Content-Type: %s\r\n"
			"Content-Length: %d\r\n"
			"\r\n", handler->content_type, handler->content_length);
	}
};

struct BufferResponse_t {
	char buffer[256] = {'\0'};
	State_t state = RESPONSE;
};

Message_t file_coroutine(void* arg)
{
	Message_t msg = {nullptr, EOF};

	if (arg != nullptr){
		FileResponse_t* res = (FileResponse_t*)arg;
		switch (res->state)
		{
			case RESPONSE:
				msg = {(const uint8_t*)res->http_state,
						(int)strlen(res->http_state)};
				res->state = State_t::HEADERS;
				break;
			case HEADERS:
				msg = {(const uint8_t*)res->h_buffer,
						(int)strlen(res->h_buffer)};
				res->state = State_t::DATA;
				break;
			case DATA:
				msg = {res->handler->data, res->handler->content_length};
				res->state = State_t::DONE;
				break;
			case DONE:
				delete res;		// clean up
		}
	}
	return msg;
}

Message_t buffer_coroutine(void* arg)
{
	//const char* dummny = "This is dummy data. ";
	Message_t msg = {nullptr, EOF};

	if (arg != nullptr){
		BufferResponse_t* res = (BufferResponse_t*)arg;
		switch (res->state)
		{
			case RESPONSE:
			case HEADERS:
			case DATA:
				msg ={(const uint8_t*)res->buffer,
						(int)strlen(res->buffer)};
				res->state = State_t::DONE;
				break;
			case DONE:
				//msg = {(const uint8_t*)dummny, (int)strlen(dummny)};
				delete res;		// clean up
		}
	}
	return msg;
}

coroutine_fn api_job(Environment_t* env, void** arg)
{
	BufferResponse_t* res = new BufferResponse_t();
	*arg = res;

	snprintf(
		res->buffer, 256,
		"%s %s\r\n"
		"Content-Type: application/json\r\n"
		"\r\n"
		"{}", HTTP_10, HTTP_200);

	return &buffer_coroutine;
}


coroutine_fn api_printer(Environment_t* env, void** arg)
{
	BufferResponse_t *res = new BufferResponse_t();
	*arg = res;

	float actual_nozzle = thermalManager.degHotend(0);
	float target_nozzle = thermalManager.degTargetHotend(0);
	float actual_heatbed = thermalManager.degBed();
	float target_heatbed = thermalManager.degTargetBed();

	snprintf(
		res->buffer, 256,
		"%s %s\r\n"
		"Content-Type: application/json\r\n"
		"\r\n"
		"{\"temperature\":{"
			"\"tool0\":{\"actual\":%f, \"target\":%f},"
			"\"bed\":{\"actual\":%f, \"target\":%f}}}",
		HTTP_10, HTTP_200,
		(double)actual_nozzle, (double)target_nozzle,
		(double)actual_heatbed, (double)target_heatbed);

	return &buffer_coroutine;
}

coroutine_fn not_found(Environment_t* env, void** arg)
{
	BufferResponse_t *res = new BufferResponse_t();
	*arg = res;

	snprintf(
		res->buffer, 256,
		"%s %s\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"Not Found", HTTP_10, HTTP_404);

	return &buffer_coroutine;
}


coroutine_fn application(Environment_t * env, void** arg){
	/*
	char header[64] = {'\0'};

	printf("HTTP Request: %s %s\n", env->method, env->request_uri);

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
