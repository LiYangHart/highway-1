#include <peripheral/skywire.h>
#include <peripheral/virtual_com.h>
#include <peripheral/i2c_spi_bus.h>
#include "task/watchdog_task.h"
#include "task/skywire_task.h"
#include "task/upload_task.h"
#include "task/beacon_task.h"
#include <hayes.h>
#include <stdio.h>
#include <string.h>
#include <fat_sl.h>
#include "diag/Trace.h"

void skywire_task(void * pvParameters);
void skywire_timer_elapsed(TimerHandle_t xTimer);
void skywire_task_setup();
void skywire_task_state_change(Msg msg);
void skywire_task_wake();
void skywire_task_sleep();
void skywire_task_request(Msg msg);
void skywire_task_activate();
void skywire_task_config();
void skywire_task_pdp_enable();
void skywire_task_request_open();
void skywire_task_request_write();
void skywire_task_response_read();
void skywire_task_escape(Msg msg);
void skywire_task_set_state(uint8_t iNewState);

QueueHandle_t xSkywireQueue;
TimerHandle_t xSkywireTimer;

ATDevice dev;
char buffer[256];

uint8_t iState = SKYWIRE_STATE_POWER_OFF;
uint8_t bWakeWhilePoweringOff = pdFALSE;
SkywireRequest* pCurrentRequest = NULL;
F_FILE* xFileUpload = NULL;
uint32_t lRemainingBytes;

/* Content type headers */
const char * CONTENT_TYPE_APPLICATION_JSON = "application/json";
const char * CONTENT_TYPE_OCTET_STREAM     = "octet/stream";

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
	dev.length = 256;

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
			skywire_task_setup();
			break;
		/* Task state machine ----------------------------------------------- */
		case MSG_SKYWIRE_STATE_CHANGE:
			skywire_task_state_change(msg);
			break;
		case MSG_SKYWIRE_WAKE:
			skywire_task_wake();
			break;
		case MSG_SKYWIRE_SLEEP:
			/* TODO Not implemented yet. */
			break;
		case MSG_SKYWIRE_REQUEST:
			skywire_task_request(msg);
			break;
		/* Activation and de-activation process ----------------------------- */
		case MSG_SKYWIRE_ACTIVATE:
			skywire_task_activate();
			break;
		case MSG_SKYWIRE_ACTIVATED:
			skywire_task_set_state(SKYWIRE_STATE_POWER_ON);
			break;
		case MSG_SKYWIRE_CONFIG:
			skywire_task_config();
			break;
		case MSG_SKYWIRE_PDP_ENABLE:
			skywire_task_pdp_enable();
			break;
		/* HTTP transmit process -------------------------------------------- */
		case MSG_SKYWIRE_REQUEST_OPEN:
			skywire_task_request_open();
			break;
		case MSG_SKYWIRE_REQUEST_WRITE:
			skywire_task_request_write();
			break;
		case MSG_SKYWIRE_RESPONSE_READ:
			skywire_task_response_read();
			break;
		/* Other processes -------------------------------------------------- */
		case MSG_SKYWIRE_ESCAPE:
		case MSG_SKYWIRE_ESCAPE_1:
			skywire_task_escape(msg);
			break;
		}
	}
}

// -----------------------------------------------------------------------------

/**
 * Timer callback to initiate state transitions.
 */
void
skywire_timer_elapsed(TimerHandle_t xTimer) {
	/* The message and parameter are is stored in the timer ID. */
	uint32_t iTimer = (uint32_t)pvTimerGetTimerID(xTimer);
	uint16_t m = (iTimer & 0xFF);
	uint32_t p = ((iTimer >> 16) & 0xFF);
	Msg msg = { m, p };
	xQueueSend(xSkywireQueue, &msg, 0);
}

// -----------------------------------------------------------------------------

/**
 * Initialize the peripherals for this task.
 */
void
skywire_task_setup() {
	trace_printf("skywire_task: setup\n");
	vcp_init();
	skywire_init();
}

/**
 * Message handler that is invoked when a state transition occurs.
 */
void
skywire_task_state_change(Msg msg) {
	/* uint8_t iOldState = msg.param1 & 0xFF00 >> 8; */
	switch (msg.param1 & 0xFF) {
	case SKYWIRE_STATE_POWERING_OFF:
		trace_printf("skywire_task: powering off\n");
		break;
	case SKYWIRE_STATE_POWER_OFF:
		trace_printf("skywire_task: power off\n");

		/* Power the modem back on if a wake message arrived during the power
		   down operation. */
		if (bWakeWhilePoweringOff) {
			Msg msg = { MSG_SKYWIRE_WAKE, 0 };
			xQueueSend(xSkywireQueue, &msg, 0);
			bWakeWhilePoweringOff = pdFALSE;
		}
		break;
	case SKYWIRE_STATE_POWERING_ON: {
		trace_printf("skywire_task: powering on\n");

		/* Start the activation process. */
		Msg msg = { MSG_SKYWIRE_ACTIVATE, 0 };
		xQueueSend(xSkywireQueue, &msg, 0);
		break;
	}
	case SKYWIRE_STATE_POWER_ON: {
		trace_printf("skywire_task: power on\n");

		/* Start the connection process. */
		Msg msg = { MSG_SKYWIRE_CONFIG, 0 };
		xQueueSend(xSkywireQueue, &msg, 0);
		break;
	}
	case SKYWIRE_STATE_CONNECTED:
		trace_printf("skywire_task: connected\n");

		if (pCurrentRequest == NULL) {
			/* Pull a new job from the upload task. */
			Msg msg = { MSG_UPLOAD_NEXT, 0 };
			xQueueSend(xUploadQueue, &msg, 0);
		} else {
			/* Start working on the assigned request. */
			Msg msg = { MSG_SKYWIRE_REQUEST_OPEN, 0 };
			xQueueSend(xSkywireQueue, &msg, 0);
		}
		break;
	}
}

/**
 * Message handler that is invoked when the modem is requested to wake.
 */
void
skywire_task_wake() {
	switch (iState) {
	case SKYWIRE_STATE_POWERING_OFF:
		/* Turn the modem back on once the current power off operation is
		   complete. */
		bWakeWhilePoweringOff = pdTRUE;
		break;
	case SKYWIRE_STATE_POWER_OFF: {
		/* Need to power on. */
		skywire_task_set_state(SKYWIRE_STATE_POWERING_ON);
		break;
	}
	case SKYWIRE_STATE_POWERING_ON:
		/* Wake already in progress.
		   Let the state transition handler request a job at power on. */
		break;
	case SKYWIRE_STATE_POWER_ON:
		/* Powered on but not connected. */
		break;
	case SKYWIRE_STATE_CONNECTED:
		/* Already in the desired state. */
		if (pCurrentRequest == NULL) {
			/* Pull the next job from the queue. */
			Msg msg = { MSG_UPLOAD_NEXT, 0 };
			xQueueSend(xUploadQueue, &msg, 0);
		}
		break;
	}
}

/**
 * Message handler that is invoked when the modem is requested to sleep.
 */
void
skywire_task_sleep() {
	/* TODO Not implemented yet. */
	trace_print("skywire_task: sleep\n");
}

/**
 * Message handler that is invoked when a request is assigned to the Skywire.
 */
void
skywire_task_request(Msg msg) {
	SkywireRequest* pRequest = (SkywireRequest*)msg.param1;
	if (pCurrentRequest != NULL) {
		/* Already assigned to a request. */
		trace_printf("skywire_task: already assigned to a request\n");
		return;
	} else {
		/* Accept the request. */
		pCurrentRequest = pRequest;
		trace_printf("skywire_task: accepted a request\n");
		if (iState == SKYWIRE_STATE_CONNECTED) {
			/* Start working on this request immediately. */
			msg.message = MSG_SKYWIRE_REQUEST_OPEN;
			msg.param1 = 0;
			xQueueSend(xSkywireQueue, &msg, 0);
		}
	}
}

/**
 * Execute the modem's power-on sequence.
 */
void
skywire_task_activate() {
	trace_printf("skywire_task: activating...\n");

	/* This method is called when the state changes to powering on. However,
	 * when things go wrong we restart the modem and the current state may not
	 * be powering on - so fall back to the correct state. */
	iState = SKYWIRE_STATE_POWERING_ON;

	/* Execute the activation sequence. */
	/* This will block for about 2 seconds. */
	skywire_activate();

	/* The Skywire will not respond for at least 15 seconds. */
	xTimerChangePeriod(xSkywireTimer, 15000, 0);
	vTimerSetTimerID(xSkywireTimer, (void*)MSG_SKYWIRE_ACTIVATED);
	xTimerStart(xSkywireTimer, 0);
}

/**
 * Configure the modem after power-on.
 */
void
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
	return;

error:
	trace_printf("skywire_task: configure failed\n");

	/* Start the activation workflow from the beginning. */
	msg.message = MSG_SKYWIRE_ESCAPE;
	msg.param1 = MSG_SKYWIRE_ACTIVATE;
	xQueueSend(xSkywireQueue, &msg, 0);
}

/**
 * Activate a PDP context such that IP connections can be created.
 */
void
skywire_task_pdp_enable() {
	trace_printf("skywire_task: PDP context enable...\n");

	/**
	 * Check the state of PDP context 1.
	 */
	if (   hayes_at(&dev, "AT#SGACT?\r\n")                 != HAYES_OK
		|| hayes_res(&dev, pred_ends_with, "OK\r\n", 1000) != HAYES_OK) {
		goto error;
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

	/* Start periodically sending the SD card contents to the server. */
	skywire_task_set_state(SKYWIRE_STATE_CONNECTED);

	return;

error:
	trace_printf("skywire_task: PDP enable failed\n");

	/* Start the activation workflow from the beginning. */
	xTimerChangePeriod(xSkywireTimer, 1000, 0);
	vTimerSetTimerID(xSkywireTimer, (void*)MSG_SKYWIRE_ACTIVATE);
	xTimerStart(xSkywireTimer, 0);
}

/**
 * Start a HTTP request and emit the HTTP header.
 */
void
skywire_task_request_open() {
	if (pCurrentRequest->bFileUpload && !spi_take()) {
		/* Need to access to the SD card. */
		xTimerChangePeriod(xSkywireTimer, 1000, 0);
		vTimerSetTimerID(xSkywireTimer, (void*)MSG_SKYWIRE_REQUEST_OPEN);
		xTimerStart(xSkywireTimer, 0);
		return;
	}

	trace_printf("skywire_task: HTTP request start\n");

	/* Open a socket data connection to the server. */
	char sCommand[64];
	snprintf(sCommand, 64, "AT#SD=1,0,80,\"%s\"\r\n", pCurrentRequest->sHost);
	if (   hayes_at(&dev, sCommand)                              != HAYES_OK
		|| hayes_res(&dev, pred_ends_with, "CONNECT\r\n", 10000) != HAYES_OK) {
		trace_printf("skywire_task: open socket data failed\n");
		goto error;
	}

	/* Output the HTTP header; POST with chunked encoding. */
	char sHeader[256];
	snprintf(sHeader, 256, \
			"%s %s HTTP/1.1\r\nHost: %s\r\nContent-Type: %s\r\n" \
			"Transfer-Encoding: chunked\r\nConnection: close\r\n\r\n",
			pCurrentRequest->sMethod,
			pCurrentRequest->sPath,
			pCurrentRequest->sHost,
			pCurrentRequest->sContentType);
	if (hayes_at(&dev, sHeader) != HAYES_OK) {
		trace_printf("skywire_task: output HTTP header failed\n");
		goto error;
	}

	/* Start writing the body. */
	xTimerChangePeriod(xSkywireTimer, 100, 0);
	vTimerSetTimerID(xSkywireTimer, (void*)MSG_SKYWIRE_REQUEST_WRITE);
	xTimerStart(xSkywireTimer, 0);
	return;

error:
	/* Drop out of connected state. */

	/* Try to activate the modem again. */
	xTimerChangePeriod(xSkywireTimer, 1000, 0);
	vTimerSetTimerID(xSkywireTimer, (void*)MSG_SKYWIRE_ACTIVATE);
	xTimerStart(xSkywireTimer, 0);
}

/**
 * Write a chunk of HTTP data. This method is called repeated until all of the
 * data has been written.
 */
void
skywire_task_request_write() {
	if (pCurrentRequest->bFileUpload) {
		/* Write a chunk of file */

		if (xFileUpload == NULL) {
			/* Open the file. */
			xFileUpload = f_open(pCurrentRequest->sBody, "r");
			lRemainingBytes = f_filelength(pCurrentRequest->sBody);

			/* Emit a chunk with the length of the file. */
			trace_printf("skywire_task: writing %ld bytes\n", lRemainingBytes);
			snprintf(buffer, 512, "%lx\r\n", lRemainingBytes);
			if (hayes_at(&dev, buffer) != HAYES_OK) {
				goto error;
			}

			/* Queue another write operation. */
			xTimerChangePeriod(xSkywireTimer, 50, 0);
			vTimerSetTimerID(xSkywireTimer, (void*)MSG_SKYWIRE_REQUEST_WRITE);
			xTimerStart(xSkywireTimer, 0);
		} else
		if (lRemainingBytes > 0) {
			/* Write some more data from the file to the modem. */
			uint16_t read = f_read(buffer, 1, 128, xFileUpload);
			lRemainingBytes -= read;
			if (hayes_write(&dev, buffer, 0, read) != HAYES_OK) {
				goto error;
			}

			/* Queue another write operation. */
			xTimerChangePeriod(xSkywireTimer, 50, 0);
			vTimerSetTimerID(xSkywireTimer, (void*)MSG_SKYWIRE_REQUEST_WRITE);
			xTimerStart(xSkywireTimer, 0);
		} else {
			/* Write a terminating chunk to signal the end of the body. */
			if (hayes_at(&dev, "\r\n0\r\n\r\n") != HAYES_OK) {
				goto error;
			}

			/* Close the file. */
			if (f_close(xFileUpload) != F_NO_ERROR) {
				goto error;
			}
			xFileUpload = NULL;

			/* Release the SD card if this is a file upload. */
			if (pCurrentRequest->bFileUpload) {
				spi_give();
			}

			/* Begin reading the response from the server. */
			xTimerChangePeriod(xSkywireTimer, 50, 0);
			vTimerSetTimerID(xSkywireTimer, (void*)MSG_SKYWIRE_RESPONSE_READ);
			xTimerStart(xSkywireTimer, 0);
		}
	} else {
		/* TODO Not implemented yet.  */
	}

	return;

error:
	/* Close any open file handles. */
	if (xFileUpload != NULL) {
		f_close(xFileUpload);
		xFileUpload = NULL;
	}

	/* Release the SD card if this is a file upload. */
	if (pCurrentRequest->bFileUpload) {
		spi_give();
	}

	/* Try to activate the modem again. */
	xTimerChangePeriod(xSkywireTimer, 100, 0);
	vTimerSetTimerID(xSkywireTimer, (void*)MSG_SKYWIRE_ACTIVATE);
	xTimerStart(xSkywireTimer, 0);
}

/**
 * Read from the HTTP response buffer. This method is called repeatedly until
 * the connection is closed by the remote server.
 */
void
skywire_task_response_read() {
	uint16_t iStatusCode;
	char szStatus[64];

	/* Read the first line of the response. */
	if (hayes_res(&dev, pred_ends_with, "\r\n", 5000) != HAYES_OK) {
		goto error;
	}

	/* Expect an OK valid response status. */
	if (sscanf(dev.buffer, "HTTP/1.1 %ld %64s", &iStatusCode, szStatus) != 2) {
		goto error;
	}

	trace_printf("skywire_task: response %d %s\n", iStatusCode, szStatus);

	/* Copy the response body. */
	if (hayes_res(&dev, pred_ends_with, "\r\n\r\n", 2000) != HAYES_OK) {

	}

	/* Expect the HTTP connection to be closed. */
	if (hayes_res(&dev, pred_ends_with, "NO CARRIER\r\n", 2000) != HAYES_OK) {
		goto error;
	}

	/* Successfully processed this request. */
	trace_printf("skywire_task: request complete\n", iStatusCode, szStatus);
	pCurrentRequest = NULL;

	Msg msg = {  MSG_UPLOAD_DONE, 0 };
	xQueueSend(xUploadQueue, &msg, 0);

	return;

error:
	/* Try to activate the modem again. */
	xTimerChangePeriod(xSkywireTimer, 100, 0);
	vTimerSetTimerID(xSkywireTimer, (void*)MSG_SKYWIRE_ACTIVATE);
	xTimerStart(xSkywireTimer, 0);
}

/**
 * Generate an escape sequence. A message with the value in msg.param1 will be
 * enqueued after the escape sequence has been emitted.
 */
void
skywire_task_escape(Msg msg) {
	switch (msg.message) {
	case MSG_SKYWIRE_ESCAPE:
		/* Put the callback message in the timer ID high bytes. */
		vTimerSetTimerID(xSkywireTimer, (void*)(MSG_SKYWIRE_ESCAPE_1 |
				((msg.param1 & 0xFF) << 16)));
		break;
	case MSG_SKYWIRE_ESCAPE_1:
		hayes_at(&dev, "+++\r\n");

		/* Queue the callback message after the escape sequence. */
		vTimerSetTimerID(xSkywireTimer, (void*)msg.param1);
		break;
	}

	/* Need a 1 second delay. */
	xTimerChangePeriod(xSkywireTimer, 1000, 0);
	xTimerStart(xSkywireTimer, 0);
}

// -----------------------------------------------------------------------------

/**
 * Setter to update the state machine state.
 */
void
skywire_task_set_state(uint8_t iNewState) {
	/* Make the state transition parameter. */
	uint32_t iStateTransition = (iState << 8) | iNewState;

	/* Update the state. */
	iState = iNewState;

	/* Send a state transition message. */
	Msg msg = { MSG_SKYWIRE_STATE_CHANGE, iStateTransition };
	xQueueSend(xSkywireQueue, &msg, 0);
}
