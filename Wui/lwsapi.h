//! LwIP WSAPI C/C++ implementation.
/** This is C/C++ implementation of Lua's WSAPI or prospal related to WSGI 2.0,
    for LwIP stack, which is base on tcp callbacks. The interface is like
    Lua's WSAPI, but extend to context pointers, while C don't have coroutines,
    and LwIP could work on more systems, so context switching could not be
    good idea.

    +--------+     +----------------------------------+  |
    |Thread X|     | LwIP Thread                      |  |
    |        |     | +-------------------------------+|  |
    |        |     | |LwIP                           ||  |
    |        |     | |+-------------------+  +-----+ ||  |  +-------------+
    |        |     | || WSAPI HTTP Server <--> Eth <--------> HTTP Client |
    |        |     | |+----------^--------+  +-----+ ||  |  +-------------+
    |        |     | +-----------|-------------------+|  |
    |+------+|     | +-----------v------------+       |  |
    || Data <--------> WSAPI HTTP Application |       |  |
    |+------+|     | +------------------------+       |  |
    +--------+     +----------------------------------+  |
*/

#pragma once

#include "lwip/tcp.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Headers list. Creator is responsible to clean the values.
typedef struct _Header_t {
	const char * key;
	const char * value;
	struct _Header_t * next;
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
	const char* response;		/**< 200 OK etc.*/
	const Header_t* headers;	/**< response header */
	const uint8_t* payload;
	int length;					/**< payload length */
} Message_t;

//! coroutine_fn typedef, which must be returned from application_fn
/*!
   \param arg coroutine argument which could be set in application_fn. This
        could be pointer to struct or class which must be control in
        application_fn and coroutine_fn.
   \return Message contains response, headers, payload and length. If length is
        EOF, all data was sent. Response must be set in first time, headers
        second time and payload could be send moretimes. When response
        or headers in message exists, that will be send. All data in message
        must exists to next coroutine_fn call!
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
coroutine_fn application(Environment_t* env, void** arg);


//! Init (start) the LwIP WSAPI server.
err_t lwsapi_init(void);

#ifdef __cplusplus
}
#endif
