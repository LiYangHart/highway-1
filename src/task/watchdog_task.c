#include "task/watchdog_task.h"
#include "task/skywire_task.h"
#include "task/camera_task.h"

void watchdog_task(void * pvParameters);
void watchdog_refresh(TimerHandle_t xTimer);

QueueHandle_t xWatchdogQueue;
TimerHandle_t xWatchdogTimer;
IWDG_HandleTypeDef hiwdg;
uint8_t taskOkFlags = 0;

/**
 * Create the watchdog task.
 */
uint8_t
watchdog_task_create() {
	/* Configure the independent watchdog. */
	/* (32kHz / 32) = 1ms */
	hiwdg.Instance = IWDG;
	hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
	hiwdg.Init.Reload = IWDG_PERIOD;
	if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
		goto error;
	}

	/* Create a message loop for this task. */
	xWatchdogQueue = xQueueCreate(10, sizeof(Msg));
	if (xWatchdogQueue == NULL) goto error;

	/* Create a timer to periodically check the tasks. */
	xWatchdogTimer = xTimerCreate("WDG", (IWDG_PERIOD / 2), pdTRUE,
		(void *) 0, watchdog_refresh);
	if (xWatchdogTimer == NULL) goto error;

	/* Create the task. */
	if (xTaskCreate(watchdog_task, WATCHDOG_TASK_NAME, WATCHDOG_TASK_STACK_SIZE,
		(void *)NULL, tskIDLE_PRIORITY, NULL) != pdPASS) goto error;

	return 1;

error:
	trace_printf("watchdog_task: setup failed\n");
	return 0;
}

/**
 * Send an event to check the task state and refresh the watchdog.
 */
void
watchdog_refresh(TimerHandle_t xTimer __attribute__((unused))) {
	Msg msg = { MSG_IWDG_REFRESH, 0 };
	xQueueSend(xWatchdogQueue, &msg, 0);
}

/**
 * Watchdog task to monitor the state of the system.
 */
void
watchdog_task(void * pvParameters __attribute__((unused))) {
	/* Set all of the tasks' initial state to OK. */
	taskOkFlags = IWDG_ALL_TASKS_OK;

	/* Start the independent watchdog. */
	if (HAL_IWDG_Start(&hiwdg) != HAL_OK) {
		trace_printf("watchdog_task: start IWDG failed\n");
		vTaskDelete(NULL);
		return;
	}

	/* Start the timer. */
	if (xTimerStart(xWatchdogTimer, 0) != pdTRUE) {
		vTaskDelete(NULL);
		return;
	}

	for (;;) {
		Msg msg;
		/* Block until messages are received. */
		if (xQueueReceive(xWatchdogQueue, &msg, portMAX_DELAY) != pdTRUE) {
			continue;
		}
		switch (msg.message) {
		case MSG_IWDG_PONG:
			/* Set the OK flag for the task. */
			/* Param1 should contain the mask we provided in the ping. */
			taskOkFlags |= msg.param1;
			break;
		case MSG_IWDG_REFRESH:
			/* Refresh the watchdog if all tasks are OK. */
			if (taskOkFlags == IWDG_ALL_TASKS_OK) {
				HAL_IWDG_Refresh(&hiwdg);
			}

			/* Reset the OK flags for all of the tasks. */
			taskOkFlags = 0;

			/* Request a status update from the camera task. */
			msg.message = MSG_IWDG_PING;
			msg.param1 = IWDG_CAMERA_MASK;
			xQueueSend(xCameraQueue, &msg, 0);

			/* Request a status update from the Skywire task. */
			msg.param1 = IWDG_SKYWIRE_MASK;
			xQueueSend(xSkywireQueue, &msg, 0);

			break;
		}
	}
}
