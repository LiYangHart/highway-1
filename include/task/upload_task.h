/**
 * This task manages the queue of files to upload. It periodically scans the
 * /upload/ directory on the SD card to find files.
 *
 * Author: Mark Lieberman
 */

#ifndef _UPLOAD_TASK_H_
#define _UPLOAD_TASK_H_

#include "common.h"
#include "http_client.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constants ---------------------------------------------------------------- */

/* Task name and stack size. */
#define UPLOAD_TASK_NAME               "UPLD"
#define UPLOAD_TASK_STACK_SIZE         1024

/* Hostname of the remote server. */
#define SERVER_HOSTNAME                "a2a93497.ngrok.io"

/* Interval on which to scan the SD card for new files. */
#define UPLOAD_SCAN_INTERVAL           30000

/* Typedefs ----------------------------------------------------------------- */

typedef struct _RequestQueueItem {
	HttpRequest * req;
	HttpResponse * res;
	uint8_t sent;
	struct _RequestQueueItem* next;
} RequestQueueItem;

/* Variables ---------------------------------------------------------------- */

extern QueueHandle_t xUploadQueue;

/* Functions ---------------------------------------------------------------- */

void upload_tell(uint16_t message, uint32_t param1);
uint8_t upload_task_create();

#ifdef __cplusplus
}
#endif

#endif /* _UPLOAD_TASK_H_ */

