/**
 * This task manages the queue of files to upload. It periodically scans the
 * /upload/ directory on the SD card to find files. Uploads are treated like
 * jobs in a queue. The Skywire tasks asks for a job and the upload task hands
 * out file to transmit.
 *
 * Author: Mark Lieberman
 */

#ifndef _UPLOAD_TASK_H_
#define _UPLOAD_TASK_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "msg.h"

#include "task/skywire_task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UPLOAD_TASK_NAME "UPLO"
#define UPLOAD_TASK_STACK_SIZE 1024

typedef struct _RequestQueueItem {
	SkywireRequest xRequest;
	uint8_t bSent;
	char sContent[64];
	struct _RequestQueueItem* pNext;
} RequestQueueItem;

extern QueueHandle_t xUploadQueue;

uint8_t upload_task_create();

/* Configurable options */
#define UPLOAD_SCAN_INTERVAL 30000

#ifdef __cplusplus
}
#endif

#endif /* _UPLOAD_TASK_H_ */

