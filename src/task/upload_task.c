#include <peripheral/i2c_spi_bus.h>
#include <task/upload_task.h>
#include <task/watchdog_task.h>
#include <task/skywire_task.h>
#include <fat_sl.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <string.h>
#include "diag/Trace.h"

void upload_task(void * pvParameters);
void upload_timer_elapsed(TimerHandle_t xTimer);
void upload_timer_scan(TimerHandle_t xTimer);
void upload_scan_files();
void upload_delete_sent();
void upload_assign_next();
uint8_t upload_is_queued(char* filename);
RequestQueueItem* upload_enqueue(char* filename);
RequestQueueItem* upload_dequeue();

QueueHandle_t xUploadQueue;
TimerHandle_t xUploadTimer;
TimerHandle_t xScanTimer;
RequestQueueItem* pQueueHead = NULL;
RequestQueueItem* pAssigned = NULL;
uint8_t iQueueSize = 0;

/**
 * Create the upload task.
 */
uint8_t
upload_task_create() {
	/* Create a message loop for this task. */
	xUploadQueue = xQueueCreate(10, sizeof(Msg));
	if (xUploadQueue == NULL) goto error;

	/* Timer used to schedule delayed state transitions. */
	xUploadTimer = xTimerCreate("UPL", 1000, pdFALSE, (void*)0, upload_timer_elapsed);
	if (xUploadTimer == NULL) goto error;

	/* Timer used to schedule scans of the /upload/ directory. */
	xScanTimer = xTimerCreate("UPL_SCAN", UPLOAD_SCAN_INTERVAL, pdTRUE, (void*)0, upload_timer_scan);
	if (xScanTimer == NULL) goto error;
	xTimerStart(xScanTimer, 0);

	/* Create the task. */
	if (xTaskCreate(upload_task, UPLOAD_TASK_NAME, UPLOAD_TASK_STACK_SIZE,
		(void *)NULL, tskIDLE_PRIORITY, NULL) != pdPASS) goto error;

	/* Initialize the queue. */
	Msg msg;
	msg.message = MSG_UPLOAD_SCAN;
	xQueueSend(xUploadQueue, &msg, 0);

	trace_printf("upload_task: create task success\n");
	return 1;

error:
	trace_printf("upload_task: setup failed\n");
	return 0;
}

/**
 * Record power consumption data.
 * Activity is recorded to POWER.LOG to be picked up by the Skywire task.
 */
void
upload_task(void * pvParameters __attribute__((unused))) {
	for (;;) {
		Msg msg;
		/* Block until messages are received. */
		if (xQueueReceive(xUploadQueue, &msg, portMAX_DELAY) != pdTRUE) {
			continue;
		}
		switch (msg.message) {
		case MSG_IWDG_PING:
			/* Respond to the ping from the watchdog task. */
			msg.message = MSG_IWDG_PONG;
			xQueueSend(xWatchdogQueue, &msg, 0);
			break;
		case MSG_UPLOAD_SCAN:
			upload_scan_files();
			break;
		case MSG_UPLOAD_DELETE:
			upload_delete_sent();
			break;
		case MSG_UPLOAD_NEXT:
			/* Assign the first unsent item. */
			upload_assign_next();
			break;
		case MSG_UPLOAD_DONE:
			/* Flag the assigned item as sent. */
			pAssigned->bSent = pdTRUE;
			pAssigned = NULL;
			upload_delete_sent();
			break;
		}
	}
}

// -----------------------------------------------------------------------------

/**
 * Timer callback to initiate state transitions.
 */
void
upload_timer_elapsed(TimerHandle_t xTimer) {
	Msg msg;
	/* The message to send is stored in the timer ID. */
	msg.message = (uint32_t)pvTimerGetTimerID(xTimer);
	msg.param1 = 0;
	xQueueSend(xUploadQueue, &msg, 0);
}

/**
 * Timer callback to initiate a scan of the /upload/ directory.
 */
void
upload_timer_scan(TimerHandle_t xTimer __attribute__((unused))) {
	Msg msg = { MSG_UPLOAD_SCAN, 0 };
	xQueueSend(xUploadQueue, &msg, 0);
}

// -----------------------------------------------------------------------------

/**
 * Look for files on the SD card to upload.
 */
void
upload_scan_files() {
	if (!spi_take()) {
		/* Keep trying to obtain access to the SPI bus. */
		xTimerChangePeriod(xUploadTimer, 100, 0);
		vTimerSetTimerID(xUploadTimer, (void*)MSG_UPLOAD_SCAN);
		xTimerStart(xUploadTimer, 0);
		return;
	}

	trace_printf("upload_task: scanning files\n");

	F_FIND xFindStruct;
	if (f_findfirst("/upload/*.*", &xFindStruct) == F_NO_ERROR) {
		do {
			/* Add the file if it is not queued. */
			uint8_t bDir = (xFindStruct.attr & F_ATTR_DIR) == F_ATTR_DIR;
			if (!bDir && !upload_is_queued(xFindStruct.filename)) {
				trace_printf("upload_task: found %s\n", xFindStruct.filename);
				upload_enqueue(xFindStruct.filename);
			}
		} while (f_findnext(&xFindStruct) == F_NO_ERROR);
	}

	spi_give();

	if (iQueueSize > 0) {
		/* Wake the Skywire to process the queue. */
		Msg msg = { MSG_SKYWIRE_WAKE, 0 };
		xQueueSend(xSkywireQueue, &msg, 0);
	} else {
		/* Sleep the Skywire to conserve power. */
		Msg msg = { MSG_SKYWIRE_SLEEP, 0 };
		xQueueSend(xSkywireQueue, &msg, 0);
	}
}

/**
 * Delete files that have been transmitted.
 */
void
upload_delete_sent() {
	if (!spi_take()) {
		/* Keep trying to obtain access to the SPI bus. */
		xTimerChangePeriod(xUploadTimer, 100, 0);
		vTimerSetTimerID(xUploadTimer, (void*)MSG_UPLOAD_DELETE);
		xTimerStart(xUploadTimer, 0);
		return;
	}

	while ((pQueueHead != NULL) && pQueueHead->bSent) {
		/* Delete the file and release memory. */
		RequestQueueItem* pItem = upload_dequeue();
		trace_printf("upload_task: deleting %s...", pItem->sContent);
		f_delete(pItem->sContent);
		vPortFree(pItem);
		trace_printf("done\n");
	}

	spi_give();
}

/**
 * Assign the first unsent item to the Skywire task.
 */
void
upload_assign_next() {
	if (pAssigned != NULL) {
		trace_printf("upload_task: already assigned\n");
	}

	RequestQueueItem* item = pQueueHead;
	while (item != NULL) {
		if (item->bSent == pdFALSE) {
			/* Pass the address of the item to the Skywire task. */
			pAssigned = item;
			Msg msg = { MSG_SKYWIRE_REQUEST, (uint32_t)pAssigned };
			xQueueSend(xSkywireQueue, &msg, 0);
			trace_printf("upload_task: assigned %s\n", pAssigned->sContent);
			return;
		}
	}
}

// -----------------------------------------------------------------------------

/**
 * True if a file is queued, otherwise false.
 */
uint8_t
upload_is_queued(char* filename) {
	RequestQueueItem* pItem = pQueueHead;
	while (pItem != NULL) {
		if (strncmp(pItem->sContent, filename, 64) == 0) {
			return pdTRUE;
		}
		pItem = pItem->pNext;
	}
	return pdFALSE;
}

/**
 * Add a file to the upload queue.
 */
RequestQueueItem*
upload_enqueue(char* filename) {
	RequestQueueItem *pEnqueue, *pItem;
	pEnqueue = (RequestQueueItem*)pvPortMalloc(sizeof(RequestQueueItem));
	if (pEnqueue == NULL) {
		return NULL;
	} else {
		/* Setup the queued item. */
		pEnqueue->bSent = pdFALSE;
		pEnqueue->pNext = NULL;
		strncpy(pEnqueue->sContent, filename, 64);

		/* Build the Skywire HTTP request. */
		pEnqueue->xRequest.iHandle = 0;
		pEnqueue->xRequest.bFileUpload = pdTRUE;
		strncpy(pEnqueue->xRequest.sMethod, "POST", 16);
		strncpy(pEnqueue->xRequest.sHost, "6d5bc316.ngrok.io", 64);
		strncpy(pEnqueue->xRequest.sPath, "/data", 16);
		pEnqueue->xRequest.sContentType = CONTENT_TYPE_OCTET_STREAM;
		pEnqueue->xRequest.sBody = pEnqueue->sContent;

		if (pQueueHead != NULL) {
			pItem = pQueueHead;
			while (pItem->pNext != NULL) {
				pItem = pItem->pNext;
			}
			pItem->pNext = pEnqueue;
		} else {
			pQueueHead = pEnqueue;
		}

		iQueueSize++;
		return pEnqueue;
	}
}

/**
 * Remove a file from the head of the queue.
 */
RequestQueueItem*
upload_dequeue() {
	RequestQueueItem* head = pQueueHead;
	pQueueHead = pQueueHead->pNext;
	iQueueSize--;
	return head;
}
