#include <task/power_task.h>
#include <task/watchdog_task.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <string.h>

void power_timer_elapsed(TimerHandle_t xTimer);
void power_task_setup();

QueueHandle_t xPowerQueue;
TimerHandle_t xPowerTimer;

ADC_HandleTypeDef hadc;

/**
 * Create the power task.
 */
uint8_t
power_task_create() {
	/* Create a message loop for this task. */
	xPowerQueue = xQueueCreate(10, sizeof(Msg));
	if (xPowerQueue == NULL) goto error;

	/* Timer used to schedule delayed state transitions. */
	xPowerTimer = xTimerCreate("PWR", 1000, pdFALSE, (void*)0, power_timer_elapsed);
	if (xPowerTimer == NULL) goto error;

	/* Setup the ADC hardware. */
	Msg msg;
	msg.message = MSG_POWER_SETUP;
	xQueueSend(xPowerQueue, &msg, 0);

	return 1;

error:
	trace_printf("power_task: setup failed\n");
	return 0;
}

/**
 * Record power consumption data.
 * Activity is recorded to POWER.LOG to be picked up by the Skywire task.
 */
void
power_task(void * pvParameters __attribute__((unused))) {
	for (;;) {
		Msg msg;
		/* Block until messages are received. */
		if (xQueueReceive(xPowerQueue, &msg, portMAX_DELAY) != pdTRUE) {
			continue;
		}
		switch (msg.message) {
		case MSG_IWDG_PING:
			/* Respond to the ping from the watchdog task. */
			msg.message = MSG_IWDG_PONG;
			xQueueSend(xWatchdogQueue, &msg, 0);
			break;
		case MSG_POWER_SETUP:
			/* Setup the sensors and camera. */
			power_task_setup();
			break;
		}
	}
}

// -----------------------------------------------------------------------------

/**
 * Timer callback to initiate state transitions.
 */
void
power_timer_elapsed(TimerHandle_t xTimer) {
	Msg msg;
	/* The message to send is stored in the timer ID. */
	msg.message = (uint32_t)pvTimerGetTimerID(xTimer);
	msg.param1 = 0;
	xQueueSend(xPowerQueue, &msg, 0);
}

// -----------------------------------------------------------------------------

/**
 * Initialize the peripherals for this task.
 */
void
power_task_setup() {
	// TODO Not implemented yet.
}


