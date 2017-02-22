/**
 * HTTP client layer for Skywire.
 *
 * Only public API method is 'http_request()'. This will enqueue a HTTP request
 * and provide a callback after the request is processed. Skywire modem power
 * state is managed transparently. Modem is powered off when the request queue
 * is empty.
 *
 * Author: Mark Lieberman
 */

#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include "common.h"
#include "hayes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constants ---------------------------------------------------------------- */

/* Maximum amount of time to wait for a response. This includes time waiting
   for the response to arrive, as well as time spend reading the body. */
#define HTTP_READ_TIMEOUT 30000

/* Useful HTTP string constants. */
extern const char * METHOD_GET;
extern const char * METHOD_POST;
extern const char * CONTENT_TYPE_APPLICATION_JSON;
extern const char * CONTENT_TYPE_OCTET_STREAM;

/* Typedefs ----------------------------------------------------------------- */

typedef struct _HttpRequest {
	uint16_t handle;
	char * host;
	char * method;
	char * path;
	const char * contentType;
	uint8_t fileUpload;
	char * body;
} HttpRequest;

typedef struct _HttpResponse {
	uint16_t handle;
	uint16_t statusCode;
	uint16_t contentLength;
	uint8_t chunkedEncoding;
	uint8_t * body;
} HttpResponse;

typedef void (*HTTP_RES_CALLBACK)(HttpResponse * res);

typedef struct _RequestNode {
	HttpRequest * req;
	HttpResponse * res;
	HTTP_RES_CALLBACK callback;
	struct _RequestNode * next;
} RequestNode;

/* Functions ---------------------------------------------------------------- */

/* Private functions for Skywire task. */
void http_client_setup(ATDevice * skywireDevice);
void http_client_message_handler(Msg msg);

/* Public HTTP client API functions. */
uint8_t http_request(HttpRequest * req, HTTP_RES_CALLBACK callback);

#ifdef __cplusplus
}
#endif

#endif /* _HTTP_CLIENT_H_ */
