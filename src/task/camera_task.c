#include <peripheral/arducam.h>
#include <peripheral/lps331.h>
#include <peripheral/hts221.h>
#include <task/camera_task.h>
#include <task/watchdog_task.h>
#include <fat_sl.h>
#include <mdriver_spi_sd.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <string.h>

void camera_timer_elapsed(TimerHandle_t xTimer);
void camera_task(void * pvParameters);

QueueHandle_t xCameraQueue;
TimerHandle_t xCameraTimer;
uint8_t arduCamInstalled = 0;
SampleBuffer samples;
uint16_t dcimIndex = 1;
uint8_t gpio_regval = 0;


/**
 * Create the camera task.
 */
uint8_t
camera_task_create() {
	/* Create a message loop for this task. */
	xCameraQueue = xQueueCreate(10, sizeof(Msg));
	if (xCameraQueue == NULL) goto error;

	/* Timer used to schedule delayed state transitions. */
	xCameraTimer = xTimerCreate("Camera", 1000, pdFALSE, (void*)0,
			camera_timer_elapsed);
	if (xCameraTimer == NULL) goto error;

	/* Create the task. */
	if (xTaskCreate(camera_task, CAMERA_TASK_NAME, CAMERA_TASK_STACK_SIZE,
		(void *)NULL, tskIDLE_PRIORITY, NULL) != pdPASS) goto error;

	/* Setup the camera hardware. */
	Msg msg;
	msg.message = MSG_CAMERA_SETUP;
	xQueueSend(xCameraQueue, &msg, 0);
	return 1;

error:
	trace_printf("camera_task: setup failed\n");
	return 0;
}

/**
 * Timer callback to initiate state transitions.
 */
void
camera_timer_elapsed(TimerHandle_t xTimer) {
	Msg msg;
	/* The message to send is stored in the timer ID. */
	msg.message = (uint16_t)pvTimerGetTimerID(xTimer);
	msg.param1 = 0;
	xQueueSend(xCameraQueue, &msg, 0);
}

/**
 * Record sensor data and capture images.
 * Activity is recorded to DATA.LOG to be picked up by the Skywire task.
 */
void
camera_task(void * pvParameters) {
	/* Initialize the sample buffer. */
	samples.Count = 0;

	for (;;) {
		Msg msg;
		/* Block until messages are received. */
		if (xQueueReceive(xCameraQueue, &msg, portMAX_DELAY) != pdTRUE) {
			continue;
		}
		switch (msg.message) {
		case MSG_IWDG_PING:
			/* Respond to the ping from the watchdog task. */
			msg.message = MSG_IWDG_PONG;
			xQueueSend(xWatchdogQueue, &msg, 0);
			break;
		case MSG_CAMERA_SETUP:
			/* Setup the sensors and camera. */
			camera_device_setup();
			break;
		case MSG_CAMERA_MOUNT:
			/* Mount the SD card. */
			camera_mount_sd();
			break;
		case MSG_CAMERA_SAMPLE:
			/* Capture a sample to the SD card. */
			capture_sample();
			break;
		case MSG_CAMERA_PHOTO:
			/* Capture a photo to the SD card. */
			capture_photo();
			break;
		}
	}
}

// -----------------------------------------------------------------------------

/**
 * Initialize the peripherals for this task.
 */
void
camera_device_setup() {
	/* Initialize I2C peripherals for this task. */
	ov5642_init();
	lps331_init();
	hts221_init();

	/* Intialize SPI peripherals for this task. */
	spi_take();
	arduCamInstalled = arducam_init();
	spi_give();

	/* Try to mount the SD card. */
	Msg msg;
	msg.message = MSG_CAMERA_MOUNT;
	xQueueSend(xCameraQueue, &msg, 0);
}

/**
 * Try to mount the SD card.
 */
void
camera_mount_sd() {
	spi_take();

	if (fn_initvolume(mmc_spi_initfunc) != F_NO_ERROR) {
		trace_printf("sdcard: failed to mount volume\n");
		goto error;
	} else {
		trace_printf("sdcard: volume mounted\n");
	}

	/* Start taking samples. */
	Msg msg;
	msg.message = MSG_CAMERA_SAMPLE;
	xQueueSend(xCameraQueue, &msg, 0);
	return;

error:
	spi_give();

	/* Retry in one second. */
	xTimerChangePeriod(xCameraTimer, 1000, 0);
	vTimerSetTimerID(xCameraTimer, MSG_CAMERA_MOUNT);
	xTimerStart(xCameraTimer, 0);
}

/**
 * Capture a sample reading.
 */
void
capture_sample() {
	spi_take();
	trace_printf("Taking a sample\n");
	spi_give();

	samples.Count++;

	if (samples.Count > 30) {
		/* Take a photo */
		xTimerChangePeriod(xCameraTimer, 1000, 0);
		vTimerSetTimerID(xCameraTimer, MSG_CAMERA_PHOTO);
		xTimerStart(xCameraTimer, 0);
	} else {
		/* Take another samples */
		xTimerChangePeriod(xCameraTimer, 1000, 0);
		vTimerSetTimerID(xCameraTimer, MSG_CAMERA_SAMPLE);
		xTimerStart(xCameraTimer, 0);
	}
	return;

error:
	/* Retry in one second. */
	xTimerChangePeriod(xCameraTimer, 1000, 0);
	vTimerSetTimerID(xCameraTimer, MSG_CAMERA_SAMPLE);
	xTimerStart(xCameraTimer, 0);
}

/**
 * Capture a photo.
 */
void
capture_photo() {
	spi_take();
	trace_printf("Taking a photo\n");
	spi_give();

	samples.Count = 0;

	/* Fall through to take a sample. */
error:
	/* Retry in one second. */
	xTimerChangePeriod(xCameraTimer, 1000, 0);
	vTimerSetTimerID(xCameraTimer, MSG_CAMERA_SAMPLE);
	xTimerStart(xCameraTimer, 0);
}
