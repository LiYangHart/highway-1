#include "peripheral/i2c_spi_bus.h"
#include "task/skywire_task.h"
#include "http_client.h"

const char * METHOD_GET                    = "GET";
const char * METHOD_POST                   = "POST";
const char * CONTENT_TYPE_APPLICATION_JSON = "application/json";
const char * CONTENT_TYPE_OCTET_STREAM     = "octet/stream";

static ATDevice * dev;
static RequestNode * queue = NULL;
static F_FILE * fileHandle = NULL;
static uint32_t remainingBytes;
static TickType_t resStart = 0;

/* Function prototypes */
void http_request_open();
void http_request_write();
void http_response_read_status();
void http_response_read_header();
void http_response_read_body();
void http_request_over();

/**
 * Setup the HTTP client.
 */
void
http_client_setup(ATDevice * device) {
	dev = device;
}

/**
 * Message loop for the Skywire/HTTP implementation.
 */
void
http_client_message_handler(Msg msg) {
	switch (msg.message) {
	/* Hook into Skywire state change notifications. */
	case MSG_SKYWIRE_STATE_CHANGE:
		if ((msg.param1 & 0xFF) == SKYWIRE_STATE_CONNECTED) {
			/* Start a HTTP request if none are in progress. */
			if (queue != NULL) {
				skywire_tell(MSG_HTTP_REQUEST_OPEN, 0);
			}
		}
		break;
	case MSG_HTTP_REQUEST_OPEN:
		http_request_open();
		break;
	case MSG_HTTP_REQUEST_WRITE:
		http_request_write();
		break;
	case MSG_HTTP_RESPONSE_READ_STATUS:
		http_response_read_status();
		break;
	case MSG_HTTP_RESPONSE_READ_HEADER:
		http_response_read_header();
		break;
	case MSG_HTTP_RESPONSE_READ_BODY:
		http_response_read_body();
		break;
	case MSG_HTTP_REQUEST_OVER:
		http_request_over();
		break;
	}
}

/**
 * Start a HTTP request and emit the HTTP header.
 */
void
http_request_open() {
	HttpRequest * req = queue->req;

	/* Need to access to the SD card for file uploads. */
	if (req->fileUpload && !spi_take()) {
		skywire_tell(MSG_HTTP_REQUEST_OPEN, 0);
		return;
	}

	/* Open a socket data connection to the host. */
	if (!skywire_socket_open(req->host)) {
		goto error;
	}

	trace_printf("skywire_task: HTTP request start\n");

	/* Output the HTTP header. */
	snprintf((char *)dev->buffer, dev->length, \
			"%s %s HTTP/1.1\r\nHost: %s\r\nContent-Type: %s\r\n" \
			"Transfer-Encoding: chunked\r\nConnection: close\r\n\r\n",
			req->method,
			req->path,
			req->host,
			req->contentType);
	if (hayes_at(dev, (char *)dev->buffer) != HAYES_OK) {
		trace_printf("skywire_task: output HTTP header failed\n");
		goto error;
	}

	/* Start writing the body. */
	skywire_tell_delay(MSG_HTTP_REQUEST_WRITE, 0, 100);
	return;

error:
	/* Power cycle the modem to get back into a good state. */
	if (req->fileUpload) { spi_give(); }
	skywire_reactivate();
}

/**
 * Write a chunk of HTTP data. This method will invoke itself repeatedly until
 * all request content is written. Throughput is limited by write buffer size
 * and invocation rate.
 */
void
http_request_write() {
	HttpRequest * req = queue->req;

	if (req->fileUpload) {
		if (fileHandle == NULL) {
			/* Open the file. */
			fileHandle = f_open(req->body, "r");
			remainingBytes = f_filelength(req->body);

			/* Emit a chunk with the length of the file. */
			trace_printf("skywire_task: writing %ld bytes\n", remainingBytes);
			snprintf(dev->buffer, SKYWIRE_BUFFER_LENGTH, "%lx\r\n", remainingBytes);
			if (hayes_at(dev, (char *)dev->buffer) != HAYES_OK) {
				goto error;
			}

			/* Queue another write operation. */
			skywire_tell_delay(MSG_HTTP_REQUEST_WRITE, 0, 50);
		} else
		if (remainingBytes > 0) {
			/* Write some more data from the file to the modem. */
			uint16_t read = f_read(dev->buffer, 1, 128, fileHandle);
			remainingBytes -= read;
			if (hayes_write(dev, dev->buffer, 0, read) != HAYES_OK) {
				goto error;
			}

			/* Queue another write operation. */
			skywire_tell_delay(MSG_HTTP_REQUEST_WRITE, 0, 50);
		} else {
			/* Write a terminating chunk to signal the end of the body. */
			if (hayes_at(dev, "\r\n0\r\n\r\n") != HAYES_OK) {
				goto error;
			}

			/* Close the file and release the SPI bus. */
			f_close(fileHandle); fileHandle = NULL; spi_give();

			/* Begin reading the response from the server. */
			resStart = xTaskGetTickCount();
			skywire_tell_delay(MSG_HTTP_RESPONSE_READ_STATUS, 0, 50);
		}
	} else {
		/* TODO Not implemented yet.  */
	}

	return;

error:
	/* Power cycle the modem to get back into a good state. */
	if (fileHandle != NULL) {
		f_close(fileHandle);
		fileHandle = NULL;
		remainingBytes = 0;
	}
	if (req->fileUpload) {
		spi_give();
	}
	skywire_reactivate();
}

/**
 * Read the status line from the HTTP response. Invoked repeatedly until the
 * status is received or the response timeout has elapsed.
 */
void
http_response_read_status() {
	/* Read the first line of the response. */
	if (hayes_res(dev, pred_ends_with, "\r\n", 100) != HAYES_OK) {
		goto check_timeout;
	}

	/* Expect a valid HTTP status line. */
	uint16_t statusCode;
	if (sscanf(dev->buffer, "HTTP/1.1 %d", &statusCode) != 1) {
		goto error;
	}

	/* Allocate a response for this request. */
	queue->res = (HttpResponse *)pvPortMalloc(sizeof(HttpResponse));
	if (queue->res == NULL) {
		/* TODO Not enough memory. */
	}

	/* Initialize the response. */
	queue->res->contentLength = 0;
	queue->res->body = NULL;

	/* Begin parsing the headers in the response. */
	resStart = xTaskGetTickCount();
	skywire_tell_delay(MSG_HTTP_RESPONSE_READ_HEADER, 0, 50);
	return;

check_timeout:
	/* Keep trying until the timeout elapses. */
	if ((xTaskGetTickCount() - resStart) < HTTP_READ_TIMEOUT) {
		skywire_tell_delay(MSG_HTTP_RESPONSE_READ_STATUS, 0, 50);
		return;
	}
	/* Timeout has elapsed - fall through to error. */
error:
	/* TODO Smarter error handling of failed requests. */
	skywire_reactivate();
}

/**
 * Read headers from the HTTP response header. Invoked repeatedly until all
 * headers are received or the response timeout has elapsed.
 */
void
http_response_read_header() {
	/* Read the headers and get the content length. */
	if (hayes_res(dev, pred_ends_with, "\r\n", 100) != HAYES_OK) {
		goto check_timeout;
	}

	if ((dev->buffer[0] == '\r') && (dev->buffer[1] == '\n')) {
		/* Reached the end of the response headers. */
		resStart = xTaskGetTickCount();
		skywire_tell(MSG_HTTP_RESPONSE_READ_BODY, 0);
		return;
	}

	HttpResponse * res = queue->res;

	/* Extract values from any header in which we are interested. */
	sscanf(dev->buffer, "Content-Length: %d", &res->contentLength);

	if (strncmp(dev->buffer, "Transfer-Encoding: chunked", dev->read) == 0) {
		res->chunkedEncoding = 1;
	}

	skywire_tell(MSG_HTTP_RESPONSE_READ_HEADER, 0);
	return;

check_timeout:
	/* Keep trying until the timeout elapses. */
	if ((xTaskGetTickCount() - resStart) < HTTP_READ_TIMEOUT) {
		skywire_tell(MSG_HTTP_RESPONSE_READ_HEADER, 0);
		return;
	} else {
		/* Timeout has elapsed - fall through to error. */
		/* TODO Smarter error handling of failed requests. */
		skywire_reactivate();
	}
}

/**
 * Read the body from the HTTP response. Invoked repeatedly until the entire
 * body is received or the response timeout has elapsed.
 */
void
http_response_read_body() {
	HttpResponse * res = queue->res;

	if (res->chunkedEncoding) {
		/* TODO Support chunked transfer encoding. */
	} else
	if (res->contentLength > 0) {
		/* Read the expected number of bytes from the response. */
		if (remainingBytes <= 0) {
			remainingBytes = res->contentLength;

			/* Allocate a buffer for the response body. */
			/* TODO Limit memory allocated for the response. */
			res->body = (uint8_t *)pvPortMalloc(remainingBytes + 1);
			if (res->body == NULL) {
				/* TODO Not enough memory. */
				return;
			} else {
				/* Null terminate the response. */
				res->body[remainingBytes] = '\0';
			}

			skywire_tell(MSG_HTTP_RESPONSE_READ_BODY, 0);
		} else {
			/* Cheating a little - we know the DMA buffer for the Skywire is
			   512 characters. Don't choose a limit too high or we risk losing
			   characters. */
			uint16_t limit = 256;
			if (remainingBytes < 256) {
				limit = remainingBytes;
			}
			uint16_t count = dev->api.count();
			if (count < limit) {
				goto check_timeout;
			} else {
				/* Write into the response body buffer. */
				uint8_t * pos = res->body + (res->contentLength - remainingBytes);
				for (uint16_t i = 0; i < limit; ++i) {
					*pos = dev->api.get_char();
					pos++;
				}

				remainingBytes -= limit;

				if (remainingBytes > 0) {
					/* More bytes to receive. */
					skywire_tell(MSG_HTTP_RESPONSE_READ_BODY, 0);
				} else {
					/* Finished reading the body. */
					skywire_tell(MSG_HTTP_REQUEST_OVER, 0);
				}
			}
		}
	} else {
		/* Content length is zero.*/
		skywire_tell(MSG_HTTP_REQUEST_OVER, 0);
	}

	return;
check_timeout:
	/* Keep trying until the timeout elapses. */
	if ((xTaskGetTickCount() - resStart) < HTTP_READ_TIMEOUT) {
		skywire_tell(MSG_HTTP_RESPONSE_READ_BODY, 0);
		return;
	} else {
		/* Timeout has elapsed - fall through to error. */
		/* TODO Smarter error handling of failed requests. */
		skywire_reactivate();
	}
}

/**
 * Finish processing the HTTP request.
 */
void
http_request_over() {
	if (hayes_res(dev, pred_ends_with, "NO CARRIER\r\n", 2000) != HAYES_OK) {
		goto error;
	}

	RequestNode * head = queue;
	queue = queue->next;

	head->callback(head->res);
	vPortFree(head);

	if (queue != NULL) {
		skywire_tell(MSG_HTTP_REQUEST_OPEN, 0);
	}

	return;

error:
	skywire_reactivate();
}

/* -------------------------------------------------------------------------- */

/**
 * Make an HTTP request. The request is placed in a queue, and the callback
 * is invoked when the request has been performed.
 */
uint8_t
http_request(HttpRequest * req, HTTP_RES_CALLBACK callback) {
	RequestNode * newNode = (RequestNode *)pvPortMalloc(sizeof(RequestNode));
	if (newNode != NULL) {
		newNode->req = req;
		newNode->callback = callback;
		newNode->next = NULL;

		/* Add this request to the queue. */
		if (queue != NULL) {
			RequestNode * node = queue;
			while (node->next != NULL) {
				node = node->next;
			}
			node->next = newNode;
		} else {
			queue = newNode;
		}

		if (!skywire_connected()) {
			/* Need to connect to the network. */
			skywire_tell(MSG_SKYWIRE_WAKE, 0);
		} else
		if (req == NULL) {
			/* Client is idle. Start the request. */
			skywire_tell(MSG_HTTP_REQUEST_OPEN, 0);
		}

		return 1;
	}
	return 0;
}
