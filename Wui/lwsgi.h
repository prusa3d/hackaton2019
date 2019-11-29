#pragma once

#include "lwip/tcp.h"
#include "http_states.h"
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

//! Headers list. Creator is responsible to clean the values.
typedef struct _Header_t {
	_Header_t * next;
	char * key;
	char * value;
} Header_t;

//! Environment struct like as WSGI environment as possible could be.
typedef struct _Environment_t {
	char * method;				/**< HTTP METHOD (HEADER|GET|POST|PUT|etc..) */
	char * request_uri;			/**< Full HTTP request uri */
	Header_t * headers;			/**< List of request headers */
	// TODO: input callback which is call from tcp_pull to read full request
} Environment_t;

//! Message which must be returned from coroutine generator.
typedef struct _Message_t {
	const uint8_t* data;
	int length;
} Message_t;

//! coroutine_fn typedef, which must be returned from application_fn
/*!
   \param arg coroutine argument which could be set in application_fn. This
        could be pointer to struct or class which must be control in
        application_fn and coroutine_fn.
   \return Message contains data and length. If length is EOF, all data was
        sent. Data in message must be exists to next coroutine_fn call!
*/
typedef Message_t (*coroutine_fn)(void* arg);

//! application_fn typedef, which is called in tcp_recv callback.
/*!
   This is "WSGI" application handler, which gets pointer to Environment_t
   structure and pointer to pointer to internal request/response structure,
   which must be in control of application_fn and coroutine_fn.

   \return coroutine_fn as generator of data, which is call from tcp_pull.
*/

typedef coroutine_fn (application_fn)(Environment_t* env, void** arg);

//! Define of application functions
coroutine_fn application(Environment_t* env, void **arg);


//! Init (start) the LwIP WSGI server.
err_t lwsgi_init(void);

#ifdef __cplusplus
}
#endif
