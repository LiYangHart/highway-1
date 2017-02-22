#include "common.h"
#include <task/upload_task.h>
#include <task/watchdog_task.h>
#include <peripheral/i2c_spi_bus.h>
#include "mdriver_spi_sd.h"
#include "http_client.h"

QueueHandle_t xUploadQueue;
TimerHandle_t xUploadTimer;
TimerHandle_t xScanTimer;

static Msg delayedMsg;
static uint16_t handle = 0;
RequestQueueItem* uploadQueue = NULL;
uint8_t iQueueSize = 0;

/* Function prototypes */
void upload_task_message_loop(void * pvParameters);
void upload_timer_elapsed(TimerHandle_t xTimer);
void upload_timer_scan(TimerHandle_t xTimer);
void upload_tell_delay(uint16_t message, uint32_t param1, TickType_t delay);

void mount_sdcard();
void scan_files();
void delete_sent();
void res_callback();

RequestQueueItem* stub_file_upload(char * filename);
uint8_t upload_is_queued(char * filename);
void upload_enqueue(RequestQueueItem * item);
void upload_remove(RequestQueueItem * item);

/* Task setup and message loop ---------------------------------------------- */

/**
 * Create the upload task.
 */
uint8_t
upload_task_create() {
	BaseType_t retVal;

	/* Create a message loop for this task. */
	xUploadQueue = xQueueCreate(10, sizeof(Msg));
	ERROR_ON_NULL(xUploadQueue);

	/* Timer used to schedule delayed state transitions. */
	xUploadTimer = xTimerCreate(
			"UPL",
			1000,
			pdFALSE,
			(void*)0,
			upload_timer_elapsed);
	ERROR_ON_NULL(xUploadTimer)

	/* Timer used to schedule scans of the /upload/ directory. */
	xScanTimer = xTimerCreate(
			"UPL_SCAN",
			UPLOAD_SCAN_INTERVAL,
			pdTRUE,
			(void*)0,
			upload_timer_scan);
	ERROR_ON_NULL(xScanTimer)
	xTimerStart(xScanTimer, 0);

	/* Create the task. */
	retVal = xTaskCreate(
			upload_task_message_loop,
			UPLOAD_TASK_NAME,
			UPLOAD_TASK_STACK_SIZE,
			(void *)NULL,
			tskIDLE_PRIORITY, NULL);
	ERROR_ON_FAIL(retVal)

	trace_printf("upload_task: created task\n");
	return 1;

error:
	trace_printf("upload_task: setup failed\n");
	return 0;
}

/**
 * Message loop for the upload task.
 */
void
upload_task_message_loop(void * pvParameters __attribute__((unused))) {
	/* Mount the SD-card. */
	upload_tell(MSG_UPLOAD_SDCARD_MOUNT, 0);

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
		case MSG_UPLOAD_SDCARD_MOUNT:
			mount_sdcard();
			break;
		case MSG_UPLOAD_SCAN:
			scan_files();
			break;
		case MSG_UPLOAD_DELETE:
			delete_sent();
			break;
		}
	}
}

/* Timer callbacks ---------------------------------------------------------- */

/**
 * Timer callback to initiate state transitions.
 */
void
upload_timer_elapsed(TimerHandle_t xTimer __attribute__((unused))) {
	xQueueSend(xUploadQueue, &delayedMsg, 0);
}

/**
 * Timer callback to initiate a scan of the /upload directory.
 */
void
upload_timer_scan(TimerHandle_t xTimer __attribute__((unused))) {
	upload_tell(MSG_UPLOAD_SCAN, 0);
}

/* Message sending ---------------------------------------------------------- */

/**
 * Send a message to the upload task queue.
 */
void
upload_tell(uint16_t message, uint32_t param1) {
	Msg msg = { message, param1 };
	xQueueSend(xUploadQueue, &msg, 0);
}

/**
 * Send a message to the upload task queue after a delay.
 */
void
upload_tell_delay(uint16_t message, uint32_t param1, TickType_t delay) {
	delayedMsg.message = message;
	delayedMsg.param1 = param1;
	xTimerChangePeriod(xUploadTimer, delay, 0);
	xTimerStart(xUploadTimer, 0);
}

/* Message handlers --------------------------------------------------------- */

/**
 * Mount the volume on the SD card.
 */
void
mount_sdcard() {
	if (!spi_take()) {
		/* Keep trying to obtain access to the SPI bus. */
		upload_tell_delay(MSG_UPLOAD_SDCARD_MOUNT, 0, 100);
		return;
	}

	/* Initialize the FAT volume on the SD card. */
	if (fn_initvolume(mmc_spi_initfunc) != F_NO_ERROR) {
		goto error;
	}

	/* Ensure that /upload exists on the SD card. */
	F_FIND xFindStruct;
	switch (f_findfirst("upload", &xFindStruct)) {
	case F_ERR_NOTFOUND:
		if (f_mkdir("upload") != F_NO_ERROR) {
			trace_printf("upload_task: mkdir 'upload' failed\n");
		}
		break;
	case F_NO_ERROR:
		/* Already exists. */
		break;
	default:
		/* Unexpected error. */
		goto error;
	}

	//f_chdir("upload");

	spi_give();
	trace_printf("upload_task: SD-card mounted\n");

	/* Tell the camera task that the SD is available. */
	camera_tell(MSG_SDCARD_MOUNTED, 0);
	return;

error:
	spi_give();
	trace_printf("upload_task: failed to mount SD-card\n");
	upload_tell_delay(MSG_UPLOAD_SDCARD_MOUNT, 0, 1000);
}

/**
 * Look for files on the SD card.
 */
void
scan_files() {
	if (!spi_take()) {
		/* Keep trying to obtain access to the SPI bus. */
		upload_tell_delay(MSG_UPLOAD_SCAN, 0, 100);
		return;
	}

	trace_printf("upload_task: scanning files\n");

	F_FIND xFindStruct;
	if (f_findfirst("/upload/*.*", &xFindStruct) == F_NO_ERROR) {
		do {
			/* Add the file if it is not queued. */
			uint8_t isDir = (xFindStruct.attr & F_ATTR_DIR) == F_ATTR_DIR;
			if (!isDir && !upload_is_queued(xFindStruct.filename)) {
				RequestQueueItem * item = stub_file_upload(xFindStruct.filename);
				upload_enqueue(item);
				http_request(item->req, res_callback);
			}
		} while (f_findnext(&xFindStruct) == F_NO_ERROR);
	}

	spi_give();
}

/**
 * Delete files that have been sent.
 */
void
delete_sent() {
	if (!spi_take()) {
		/* Keep trying to obtain access to the SPI bus. */
		upload_tell_delay(MSG_UPLOAD_DELETE, 0, 100);
		return;
	}

	RequestQueueItem * item = uploadQueue;
	while (item != NULL) {
		if (item->res != NULL) {
			upload_remove(item);

			/* Delete the uploaded file. */
			if (item->req->fileUpload) {
				if (f_delete(item->req->body) != F_NO_ERROR) {
					trace_printf("upload_task: failed to delete %s\n",
							item->req->body);
				}
			}

			/* Free memory used by the request. */
			vPortFree(item->req->path);
			vPortFree(item->req->body);

			/* Free memory used by the response. */
			vPortFree(item->res->body);
			vPortFree(item->res);

			/* Free the list item. */
			vPortFree(item);
		}
		item = item->next;
	}

	spi_give();
}

/* Other functions ---------------------------------------------------------- */

/**
 * Callback method that is invoked when an HTTP request is complete.
 */
void
res_callback(HttpResponse * res) {
	RequestQueueItem* item = uploadQueue;
	while (item != NULL) {
		if (item->req->handle == res->handle) {
			item->res = res;
			upload_tell(MSG_UPLOAD_DELETE, 0);
			break;
		}
		item = item->next;
	}
}

/**
 * Create a new request item for a file upload.
 */
RequestQueueItem*
stub_file_upload(char * filename) {
	RequestQueueItem * item = (RequestQueueItem *)pvPortMalloc(sizeof(RequestQueueItem));
	ERROR_ON_NULL(item);

	item->req = (HttpRequest *)pvPortMalloc(sizeof(HttpRequest));
	ERROR_ON_NULL(item->req);

	item->sent = 0;
	item->next = NULL;

	item->req->handle = ++handle;
	item->req->method = (char *)METHOD_POST;
	item->req->host = SERVER_HOSTNAME;
	item->req->contentType = CONTENT_TYPE_OCTET_STREAM;
	item->req->fileUpload = 1;

	item->req->path = NULL;
	item->req->body = NULL;
	item->req->path = (char *)pvPortMalloc(F_MAXPATH);
	item->req->body = (char *)pvPortMalloc(F_MAXPATH);
	ERROR_ON_NULL(item->req->path);
	ERROR_ON_NULL(item->req->body);

	snprintf(item->req->path, F_MAXPATH, "/data?file=%s", filename);
	snprintf(item->req->body, F_MAXPATH, "/upload/%s", filename);

	return item;

error:
	vPortFree(item->req->body);
	vPortFree(item->req->path);
	vPortFree(item->req);
	vPortFree(item);

	return NULL;
}

/**
 * True if a file is queued, otherwise false.
 */
uint8_t
upload_is_queued(char * filename) {
	RequestQueueItem* item = uploadQueue;
	while (item != NULL) {
		if (strncmp(item->req->body, filename, F_MAXPATH) == 0) {
			return 1;
		}
		item = item->next;
	}
	return 0;
}

/**
 * Add an item to the upload queue.
 */
void
upload_enqueue(RequestQueueItem * adding) {
	if (uploadQueue == NULL) {
		uploadQueue = adding;
	} else {
		RequestQueueItem * item = uploadQueue;
		while (item->next != NULL) {
			item = item->next;
		}
		item->next = adding;
	}
}

/**
 * Remove an item from the upload queue.
 */
void
upload_remove(RequestQueueItem * removing) {
	if (uploadQueue == removing) {
		uploadQueue = removing->next;
	} else {
		RequestQueueItem * item = uploadQueue;
		while ((item->next != removing) && (item->next != NULL)) {
			item = item->next;
		}
		if (item->next == removing) {
			item->next = removing->next;
		}
	}
}
