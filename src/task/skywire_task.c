#include "hayes.h"
#include "peripheral/skywire.h"
#include "task/watchdog_task.h"
#include "task/skywire_task.h"
#include "http_client.h"

QueueHandle_t xSkywireQueue;
TimerHandle_t xSkywireTimer;

static Msg delayedMsg;
static ATDevice dev;
static uint8_t buffer[SKYWIRE_BUFFER_LENGTH];
static uint8_t state = SKYWIRE_STATE_POWER_OFF;
static uint8_t wakeWhilePoweringOff = 0;

/* Function prototypes */
void skywire_task_message_loop(void * pvParameters);
void skywire_timer_elapsed(TimerHandle_t xTimer);

void skywire_task_setup();
void skywire_task_state_change(Msg msg);
void skywire_task_wake();
void skywire_task_sleep();
void skywire_task_activate(Msg msg);
void skywire_task_deactivate(Msg msg);
void skywire_task_config();
void skywire_task_pdp_enable();
void skywire_task_escape(Msg msg);

void set_state(uint8_t iNewState);

/* Task setup and message loop ---------------------------------------------- */

/**
 * Create the Skywire task.
 */
uint8_t
skywire_task_create() {
	BaseType_t retVal;

	/* Create a message loop for this task. */
	xSkywireQueue = xQueueCreate(10, sizeof(Msg));
	ERROR_ON_NULL(xSkywireQueue)

	/* Timer used to schedule delayed state transitions. */
	xSkywireTimer = xTimerCreate(
			"Skywire",
			1000,
			pdFALSE,
			(void*)0,
			skywire_timer_elapsed);
	ERROR_ON_NULL(xSkywireTimer)

	/* Create the task. */
	retVal = xTaskCreate(
			skywire_task_message_loop,
			SKYWIRE_TASK_NAME,
			SKYWIRE_TASK_STACK_SIZE,
			(void *)NULL,
			tskIDLE_PRIORITY,
			NULL);
	ERROR_ON_FAIL(retVal)

	trace_printf("skywire_task: created task\n");
	return 1;

error:
	trace_printf("skywire_task: create task failed\n");
	return 0;
}

/**
 * Message loop for the Skywire task.
 */
void
skywire_task_message_loop(void* pvParameters __attribute__((unused))) {
	/* Setup the serial communication with the modem. */
	skywire_init();

	/* Prepare an ATDevice to interface with the Skywire modem. */
	dev.api.count = skywire_count;
	dev.api.get_char = skywire_getc;
	dev.api.write = skywire_write;
	dev.buffer = buffer;
	dev.length = SKYWIRE_BUFFER_LENGTH;

	/* Initialize the HTTP client. */
	http_client_setup(&dev);

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
		/* Activation and de-activation process ----------------------------- */
		case MSG_SKYWIRE_ACTIVATE:
			skywire_task_activate(msg);
			break;
		case MSG_SKYWIRE_DEACTIVATE:
			skywire_task_deactivate(msg);
			break;
		case MSG_SKYWIRE_CONFIG:
			skywire_task_config();
			break;
		case MSG_SKYWIRE_PDP_ENABLE:
			skywire_task_pdp_enable();
			break;
		/* Other processes -------------------------------------------------- */
		case MSG_SKYWIRE_ESCAPE:
		case MSG_SKYWIRE_RESERVED:
			skywire_task_escape(msg);
			break;
		}

		/* Allow HTTP client to handle messages as well. */
		http_client_message_handler(msg);
	}
}

/* Timer callbacks ---------------------------------------------------------- */

/**
 * Timer callback to initiate state transitions.
 */
void
skywire_timer_elapsed(TimerHandle_t xTimer __attribute__((unused))) {
	xQueueSend(xSkywireQueue, &delayedMsg, 0);
}

/* Message sending ---------------------------------------------------------- */

/**
 * Send a message to the Skywire task queue.
 */
void
skywire_tell(uint16_t message, uint32_t param1) {
	Msg msg = { message, param1 };
	xQueueSend(xSkywireQueue, &msg, 0);
}

/**
 * Send a message to the Skywire task queue after a delay.
 */
void
skywire_tell_delay(uint16_t message, uint32_t param1, TickType_t delay) {
	delayedMsg.message = message;
	delayedMsg.param1 = param1;
	xTimerChangePeriod(xSkywireTimer, delay, 0);
	xTimerStart(xSkywireTimer, 0);
}

/* Message handlers --------------------------------------------------------- */

/**
 * Initialize the peripherals for this task.
 */
void
skywire_task_setup() {
	trace_printf("skywire_task: setup\n");

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
		if (wakeWhilePoweringOff) {
			Msg msg = { MSG_SKYWIRE_WAKE, 0 };
			xQueueSend(xSkywireQueue, &msg, 0);
			wakeWhilePoweringOff = pdFALSE;
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
		break;
	}
}

/**
 * Message handler that is invoked when the modem is requested to wake.
 */
void
skywire_task_wake() {
	switch (state) {
	case SKYWIRE_STATE_POWERING_OFF:
		/* Turn the modem back on once the current power off operation is
		   complete. */
		wakeWhilePoweringOff = pdTRUE;
		break;
	case SKYWIRE_STATE_POWER_OFF: {
		/* Need to power on. */
		set_state(SKYWIRE_STATE_POWERING_ON);
		break;
	}
	case SKYWIRE_STATE_POWERING_ON:
	case SKYWIRE_STATE_POWER_ON:
	case SKYWIRE_STATE_CONNECTED:
		/* Already waking up or awake. */
		break;
	}
}

/**
 * Message handler that is invoked when the modem is requested to sleep.
 */
void
skywire_task_sleep() {
	/* TODO Not implemented yet. */
	trace_printf("skywire_task: sleep\n");
}

/**
 * Execute the modem's power-on sequence. This requires a 1s < HOLD_TIME < 2s
 * pulse on the Skywire enable pin. Following the pulse, the modem will not
 * respond for up to 15 seconds.
 */
void
skywire_task_activate(Msg msg) {
	switch (msg.param1) {
	case 0:
		trace_printf("skywire_task: activating...\n");
		skywire_en(GPIO_PIN_RESET);
		skywire_tell_delay(MSG_SKYWIRE_ACTIVATE, 1, 1500);
		break;
	case 1:
		skywire_en(GPIO_PIN_SET);
		skywire_tell_delay(MSG_SKYWIRE_ACTIVATE, 2, 1500);
		break;
	case 2:
		skywire_en(GPIO_PIN_RESET);
		skywire_tell_delay(MSG_SKYWIRE_ACTIVATE, 3, 15000);
		break;
	case 3:
		trace_printf("skywire_task: activated\n");
		set_state(SKYWIRE_STATE_POWER_ON);
		break;
	}
}

/**
 *
 */
void
skywire_task_deactivate(Msg msg) {
 /* TODO Not implemented yet */
}

/**
 * Activate and then configure the modem again.
 * Usually called when something goes wrong.
 */
void
skywire_reactivate() {
	state = SKYWIRE_STATE_POWERING_ON;

	xTimerChangePeriod(xSkywireTimer, 1000, 0);
	vTimerSetTimerID(xSkywireTimer, (void*)MSG_SKYWIRE_ACTIVATE);
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
	set_state(SKYWIRE_STATE_CONNECTED);

	return;

error:
	trace_printf("skywire_task: PDP enable failed\n");

	/* Start the activation workflow from the beginning. */
	xTimerChangePeriod(xSkywireTimer, 1000, 0);
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
		vTimerSetTimerID(xSkywireTimer, (void*)(MSG_SKYWIRE_RESERVED |
				((msg.param1 & 0xFF) << 16)));
		break;
	case MSG_SKYWIRE_RESERVED:
		hayes_at(&dev, "+++\r\n");

		/* Queue the callback message after the escape sequence. */
		vTimerSetTimerID(xSkywireTimer, (void*)msg.param1);
		break;
	}

	/* Need a 1 second delay. */
	xTimerChangePeriod(xSkywireTimer, 1000, 0);
	xTimerStart(xSkywireTimer, 0);
}

/* Other functions ---------------------------------------------------------- */

/**
 * Setter to update the state machine state.
 */
void
set_state(uint8_t newState) {
	/* Make the state transition parameter. */
	uint32_t stateTransition = (state << 8) | newState;

	/* Update the state. */
	state = newState;

	/* Send a state transition message. */
	Msg msg = { MSG_SKYWIRE_STATE_CHANGE, stateTransition };
	xQueueSend(xSkywireQueue, &msg, 0);
}

/**
 * True if the Skywire is connected to the network, otherwise false.
 */
uint8_t
skywire_connected() {
	return (state == SKYWIRE_STATE_CONNECTED);
}

/**
 * Open a TCP socket connection to 'ipOrHost'.
 */
uint8_t
skywire_socket_open(char * ipOrHost) {
	snprintf(dev.buffer, 64, "AT#SD=1,0,80,\"%s\"\r\n", ipOrHost);
	if (   hayes_at(&dev, dev.buffer)                            != HAYES_OK
		|| hayes_res(&dev, pred_ends_with, "CONNECT\r\n", 10000) != HAYES_OK) {
		trace_printf("skywire_task: open socket data failed\n");
		return 0;
	}
	return 1;
}
