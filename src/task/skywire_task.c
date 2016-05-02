#include <peripheral/skywire.h>
#include <peripheral/virtual_com.h>
#include <peripheral/i2c_spi_bus.h>
#include "task/watchdog_task.h"
#include "task/skywire_task.h"
#include "task/beacon_task.h"
#include <hayes.h>
#include <stdio.h>
#include <string.h>
#include <fat_sl.h>
#include "diag/Trace.h"

/* Function prototypes. */
void skywire_task(void * pvParameters);
void skywire_timer_elapsed(TimerHandle_t xTimer);
uint8_t skywire_task_setup();
uint8_t skywire_task_activate();
uint8_t skywire_task_config();
uint8_t skywire_task_pdp_enable();
uint8_t skywire_task_transmit();

/* Variables and handles. */
QueueHandle_t xSkywireQueue;
TimerHandle_t xSkywireTimer;
ATDevice dev;
char buffer[512];

/**
 * Create the Skywire task.
 */
uint8_t
skywire_task_create() {
	/* Create a message loop for this task. */
	xSkywireQueue = xQueueCreate(10, sizeof(Msg));
	if (xSkywireQueue == NULL) goto error;

	/* Timer used to schedule delayed state transitions. */
	xSkywireTimer = xTimerCreate("Skywire", 1000, pdFALSE, (void*)0,
			skywire_timer_elapsed);
	if (xSkywireTimer == NULL) goto error;

	/* Create the task. */
	if (xTaskCreate(skywire_task, SKYWIRE_TASK_NAME, SKYWIRE_TASK_STACK_SIZE,
		(void *)NULL, tskIDLE_PRIORITY, NULL) != pdPASS) goto error;

	/* Prepare an ATDevice to interface with the Skywire modem. */
	dev.api.count = skywire_count;
	dev.api.getc = skywire_getc;
	dev.api.write = skywire_write;
	dev.buffer = buffer;
	dev.length = 512;

	/* Setup the Skywire hardware. */
	Msg msg;
	msg.message = MSG_SKYWIRE_SETUP;
	xQueueSend(xSkywireQueue, &msg, 0);

	trace_printf("skywire_task: create task success\n");
	return 1;

error:
	trace_printf("skywire_task: create task failed\n");
	return 0;
}

/**
 * Timer callback to initiate state transitions.
 */
void
skywire_timer_elapsed(TimerHandle_t xTimer) {
	Msg msg;
	/* The message to send is stored in the timer ID. */
	msg.message = (uint32_t)pvTimerGetTimerID(xTimer);
	msg.param1 = 0;
	xQueueSend(xSkywireQueue, &msg, 0);
}

/**
 * Communicate with the server.
 */
void
skywire_task(void* pvParameters __attribute__((unused))) {
	for (;;) {
		Msg msg;
		/* Block until messages are received. */
		if (xQueueReceive(xSkywireQueue, &msg, portMAX_DELAY) != pdTRUE) {
			continue;
		}
		switch (msg.message) {
		case MSG_IWDG_PING:
			/* Respond to the ping from the watchdog task. */
			msg.message = MSG_IWDG_PONG;
			xQueueSend(xWatchdogQueue, &msg, 0);
			break;
		case MSG_SKYWIRE_SETUP:
			/* Setup the peripherals used by this task. */
			skywire_task_setup();
			break;
		case MSG_SKYWIRE_ACTIVATE:
			/* Activate/power-up the modem. */
			skywire_task_activate();
			break;
		case MSG_SKYWIRE_CONFIG:
			skywire_task_config();
			break;
		case MSG_SKYWIRE_PDP_ENABLE:
			/* Connect to the cellular network. */
			skywire_task_pdp_enable();
			break;
		case MSG_SKYWIRE_XMIT:
			skywire_task_transmit();
			break;
		}
	}
}

// -----------------------------------------------------------------------------

/**
 * Initialize the peripherals for this task.
 */
uint8_t
skywire_task_setup() {
	trace_printf("skywire_task: setup\n");

	vcp_init();
	skywire_init();

	/* Try to activate the Skywire modem. */
	Msg msg;
	msg.message = MSG_SKYWIRE_ACTIVATE;
	xQueueSend(xSkywireQueue, &msg, 0);

	return 1;
}

/**
 * Execute the modem's power-on sequence.
 */
uint8_t
skywire_task_activate() {
	trace_printf("skywire_task: activating...\n");

	/* Execute the activation sequence. */
	/* This will block for about 2 seconds. */
	skywire_activate();

	/* The Skywire will not respond for at least 15 seconds. */
	xTimerChangePeriod(xSkywireTimer, 15000, 0);
	vTimerSetTimerID(xSkywireTimer, MSG_SKYWIRE_CONFIG);
	xTimerStart(xSkywireTimer, 0);

	return 1;
}

/**
 * Configure the modem after power-on.
 */
uint8_t
skywire_task_config() {
	trace_printf("skywire_task: configuring...\n");

	/**
	 * Disable echo.
	 * Disable flow control.
	 */
	if (   hayes_at(&dev, "ATE0\r\n")                      != HAYES_OK
		|| hayes_res(&dev, pred_ends_with, "OK\r\n", 1000) != HAYES_OK
		|| hayes_at(&dev, "AT&K=0\r\n")                    != HAYES_OK
		|| hayes_res(&dev, pred_ends_with, "OK\r\n", 1000) != HAYES_OK) {
		trace_printf("skywire_task: stage1 failed\n");
		goto error;
	}

	/* Connect to the cellular network. */
	Msg msg;
	msg.message = MSG_SKYWIRE_PDP_ENABLE;
	xQueueSend(xSkywireQueue, &msg, 0);

	trace_printf("skywire_task: configure success\n");
	return 0;

error:
	trace_printf("skywire_task: configure failed\n");

	/* Start the activation workflow from the beginning. */
	xTimerChangePeriod(xSkywireTimer, 15000, 0);
	vTimerSetTimerID(xSkywireTimer, MSG_SKYWIRE_ACTIVATE);
	xTimerStart(xSkywireTimer, 0);
	return 1;
}

/**
 * Activate a PDP context such that IP connections can be created.
 */
uint8_t
skywire_task_pdp_enable() {
	trace_printf("skywire_task: PDP context enable...\n");

	/**
	 * Check the state of PDP context 1.
	 */
	if (   hayes_at(&dev, "AT#SGACT?\r\n")                 != HAYES_OK
		|| hayes_res(&dev, pred_ends_with, "OK\r\n", 1000) != HAYES_OK) {
		return 0;
	} else {
		int cid, stat;
		char* line = NULL;
		while ((line = tokenize_res(&dev, line)) != NULL) {
			/* Look for the state of PDP context 1. */
			if ((sscanf(line, "#SGACT: %d,%d", &cid, &stat) == 2) && (cid == 1)) {
				if (stat == 1) {
					/* Context is already active. */
					goto success;
				} else {
					/* Proceed to activation step. */
					break;
				}
			}
		}
	}

	/**
	 * Input the APN for the Rogers network.
	 * Activate PDP context 1.
	 */
	if (   hayes_at(&dev, "AT+CGDCONT=1,\"IP\",\"internet.com\"\r\n") != HAYES_OK
		|| hayes_res(&dev, pred_ends_with, "OK\r\n", 1000)            != HAYES_OK
		|| hayes_at(&dev, "AT#SGACT=1,1\r\n")                         != HAYES_OK
		|| hayes_res(&dev, pred_ends_with, "OK\r\n", 10000)           != HAYES_OK) {
		goto error;
	}

success:
	trace_printf("skywire_task: PDP enable success\n");
	return 1;

error:
	trace_printf("skywire_task: PDP enable failed\n");

	/* Start the activation workflow from the beginning. */
	xTimerChangePeriod(xSkywireTimer, 15000, 0);
	vTimerSetTimerID(xSkywireTimer, MSG_SKYWIRE_ACTIVATE);
	xTimerStart(xSkywireTimer, 0);
	return 0;
}

/**
 * Transmit the SD card contents to the server.
 */
uint8_t
skywire_task_transmit() {
	trace_printf("skywire_task: transmit\n");

	xTimerChangePeriod(xSkywireTimer, 20000, 0);
	vTimerSetTimerID(xSkywireTimer, MSG_SKYWIRE_XMIT);
	xTimerStart(xSkywireTimer, 0);
	return 1;
}

// POST manifest ---------------------------------------------------------------

/**
 * Add a file to the manifest.
 */
uint8_t
add_to_manifest(Attachment** head, Attachment** tail, char* file) {
	/* Get the size of the attachment. */
	uint32_t length;
	if ((length = f_filelength(file)) == 0) {
		return 0;
	}

	/* Allocate an entry for the manifest. */
	Attachment* attachment;
	attachment = (Attachment*)pvPortMalloc(sizeof(Attachment));
	attachment->length = length;
	attachment->next = NULL;
	strncpy(attachment->name, file, 64);

	/* Add the attachment to the manifest. */
	if (*head == NULL) {
		*head = attachment;
		*tail = *head;
	} else {
		(*tail)->next = attachment;
		*tail = attachment;
	}

	return 1;
}

/**
 * Construct a manifest of files to be sent to the server.
 * The manifest will contain data.log and any attached images.
 */
uint8_t
get_manifest(Attachment** manifest) {
	F_FILE* pxLog;
	if ((pxLog = f_open("data.log", "r")) == NULL) {
		trace_printf("skywire_task: failed to open data.log\n");
		return 0;
	}

	/* Add data.log to the start of the manifest. */
	Attachment* tail = NULL;
	if (!add_to_manifest(manifest, &tail, "data.log")) {
		goto error;
	}

	/* Parse the data log and look for attachments. */
	char line[128];
	while (f_eof(pxLog) == 0) {
		for (uint8_t i = 0; i < 128; ++i) {
			char c = f_getc(pxLog);
			if (c == '\n') {
				line[i] = '\0';
				break;
			} else {
				line[i] = c;
			}
		}

		if (memcmp(line, "FILE:", 5) == 0) {
			int tick;
			char file[64];
			if (sscanf((line + 5), "{\"tick\":%d,\"file\":\"%[^\"]\"}", &tick, file) == 2) {
				if (!add_to_manifest(manifest, &tail, file)) {
					goto error;
				}
			}

		}
	}

	f_close(pxLog);
	return 1;

error:
	if (pxLog != NULL) { f_close(pxLog); }
	return 0;
}

/**
 * Delete all files in the manifest.
 * Free all memory associated with the manifest.
 */
void
free_manifest(Attachment* manifest, uint8_t deleteFiles) {
	Attachment* next = NULL;
	while (manifest != NULL) {
		next = manifest->next;

		/* Delete the file from the SD card. */
		if (deleteFiles && (f_delete(manifest->name) == F_NO_ERROR)) {
			trace_printf("skywire_task: removed %s\n", manifest->name);
		}

		/* Free memory associated with this attachment. */
		vPortFree(manifest);

		manifest = next;
	}
}

/**
 * Use a POST to upload the manifest to the server.
 * Write slowly until we can support software flow control.
 *
 * Each HTTP chunk contains one attachment.
 * The first 32 bytes of the chunk encode the attachment length and filename.
 * Therefore, each HTTP chunk length is the attachment length plus 32 bytes.
 */
uint8_t
post_manifest(ATDevice* dev, Attachment* manifest) {
	if (   hayes_at(dev, "AT#SD=1,0,80,\"" NGROK_TUNNEL "\"\r\n") != HAYES_OK
		|| hayes_res(dev, pred_ends_with, "CONNECT\r\n", 10000)   != HAYES_OK) {
		trace_printf("skywire_task: open socket data failed\n");
		return 0;
	}

	trace_printf("skywire_task: starting POST\n");

	/* Output the HTTP header - specify POST with chunked encoding. */
	hayes_at(dev,
			"POST /data HTTP/1.1\r\n" \
			"Host: " NGROK_TUNNEL "\r\n" \
			"Content-Type: application/octet-stream\r\n" \
			"Transfer-Encoding: chunked\r\n" \
			"Connection: close\r\n\r\n"
	);

	while (manifest != NULL) {
		// Write file
		F_FILE* pxFile = f_open(manifest->name, "r");
		if (pxFile == NULL) {
			trace_printf("failed to open file");
			continue;
		}

		/* Write the start of the HTTP chunk. */
		int length = manifest->length;
		snprintf(dev->buffer, 32, "%x\r\n", 32 +length);
		hayes_at(dev, dev->buffer);

		/* Write the attachment header. */
		memset(dev->buffer, '\0', 32);
		snprintf(dev->buffer, 32, "%s,%d\r\n", manifest->name, length);
		hayes_write(dev, (uint8_t*)dev->buffer, 0, 32);

		/* Write the attachment. */
		uint32_t written = 0;
		while (written < manifest->length) {
			uint32_t read = f_read(dev->buffer, 1, 128, pxFile);
			written += read;
			hayes_write(dev, (uint8_t*)dev->buffer, 0, read);
		}

		f_close(pxFile);
		pxFile = NULL;

		/* Write the end of the HTTP chunk. */
		hayes_at(dev, "\r\n");
		manifest = manifest->next;
	}

	/* Write the trailing HTTP chunk and termination. */
	hayes_at(dev, "0\r\n\r\n");
	return 1;
}

// Task ------------------------------------------------------------------------

uint8_t
parse_response(ATDevice* dev, uint8_t* speedLimit) {
	/**
	 * Expect a 200 OK response.
	 * Skip the headers by looking for "\r\n\r\n".
	 * Look for no carrier when the connection is closed.
	 */
	if (   hayes_res(dev, pred_ends_with, "200 OK\r\n", 20000)    != HAYES_OK
		|| hayes_res(dev, pred_ends_with, "\r\n\r\n", 10000)       != HAYES_OK
		|| hayes_res(dev, pred_ends_with, "NO CARRIER\r\n", 10000) != HAYES_OK) {
		trace_printf("skywire_task: response not as expected\n");
		return 0;
	}

	int sl = 0;
	char* line = NULL;
	while ((line = tokenize_res(dev, line)) != NULL) {
		if (sscanf(line, "SL=%d,EOM", &sl) == 1) {
			//adding line to see if incoming SL value is good
			if (sl >= 1 && sl <= 255){
				*speedLimit = sl;
				return 1;
			}
		}
	}
	*speedLimit = 100;
	return 0;
}

