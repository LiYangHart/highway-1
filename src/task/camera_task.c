#include <peripheral/arducam.h>
#include <peripheral/lps331.h>
#include <peripheral/hts221.h>
#include <task/watchdog_task.h>
#include <task/camera_task.h>

QueueHandle_t xCameraQueue;
TimerHandle_t xCameraTimer;
TimerHandle_t xCameraTimerSample;
TimerHandle_t xCameraTimerPhoto;

static Msg delayedMsg;
uint8_t cameraFound = 0;
SampleBuffer samples;
uint16_t dcimIndex = 1;

/* Function prototypes */
void camera_task_message_loop(void * pvParameters);
void camera_timer_elapsed(TimerHandle_t xTimer);
void camera_timer_sample(TimerHandle_t xTimer);
void camera_timer_photo(TimerHandle_t xTimer);
void camera_tell_delay(uint16_t message, uint32_t, TickType_t delay);

void setup_sensors();
void start_recording();
void take_sample();
void write_samples();
void take_photo();

/* Task setup and message loop ---------------------------------------------- */

/**
 * Create the camera task.
 */
uint8_t
camera_task_create() {
	BaseType_t retVal;

	/* Create a message loop for this task. */
	xCameraQueue = xQueueCreate(10, sizeof(Msg));
	ERROR_ON_NULL(xCameraQueue)

	/* Timer used to schedule delayed state transitions. */
	xCameraTimer = xTimerCreate(
			CAMERA_TASK_NAME "STATE",
			1000,
			pdFALSE,
			(void*)0,
			camera_timer_elapsed);
	ERROR_ON_NULL(xCameraTimer)

	/* Timer used to schedule environmental sampling. */
	xCameraTimerSample = xTimerCreate(
			CAMERA_TASK_NAME "SAMPLE",
			CAMERA_SAMPLE_INTERVAL,
			pdTRUE,
			(void*)0,
			camera_timer_sample);
	ERROR_ON_NULL(xCameraTimerSample)

	/* Timer used to schedule camera photo capture. */
	xCameraTimerPhoto = xTimerCreate(
			CAMERA_TASK_NAME "PHOTO",
			CAMERA_PHOTO_INTERVAL,
			pdTRUE,
			(void*)0,
			camera_timer_photo);
	ERROR_ON_NULL(xCameraTimerPhoto)

	/* Create the task. */
	retVal = xTaskCreate(
			camera_task_message_loop,
			CAMERA_TASK_NAME,
			CAMERA_TASK_STACK_SIZE,
			(void *)NULL,
			tskIDLE_PRIORITY,
			NULL);
	ERROR_ON_FAIL(retVal)

	trace_printf("camera_task: created task\n");
	return 1;

error:
	trace_printf("camera_task: setup failed\n");
	return 0;
}

/**
 * Message loop for the camera task.
 */
void
camera_task_message_loop(void * pvParameters __attribute__((unused))) {
	/* Initialize the sample buffer. */
	samples.Count = 0;

	/* Setup the sensors. */
	camera_tell(MSG_CAMERA_SETUP, 0);

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
			setup_sensors();
			break;
		case MSG_SDCARD_MOUNTED:
			start_recording();
			break;
		case MSG_CAMERA_SAMPLE:
			take_sample();
			break;
		case MSG_CAMERA_WRITE_SAMPLES:
			write_samples();
			break;
		case MSG_CAMERA_PHOTO:
			take_photo();
			break;
		}
	}
}

/* Timer callbacks ---------------------------------------------------------- */

/**
 * Timer callback to initiate state transitions.
 */
void
camera_timer_elapsed(TimerHandle_t xTimer __attribute__((unused))) {
	xQueueSend(xCameraQueue, &delayedMsg, 0);
}

/**
 * Timer callback to initiate a sample.
 */
void
camera_timer_sample(TimerHandle_t xTimer __attribute__((unused))) {
	camera_tell(MSG_CAMERA_SAMPLE, 0);
}

/**
 * Timer callback to initiate a photo.
 */
void
camera_timer_photo(TimerHandle_t xTimer __attribute__((unused))) {
	camera_tell(MSG_CAMERA_PHOTO, 0);
}

/* Message sending ---------------------------------------------------------- */

/**
 * Send a message to the upload task queue.
 */
void
camera_tell(uint16_t message, uint32_t param1) {
	Msg msg = { message, param1 };
	xQueueSend(xCameraQueue, &msg, 0);
}

/**
 * Send a message to the upload task queue after a delay.
 */
void
camera_tell_delay(uint16_t message, uint32_t param1, TickType_t delay) {
	delayedMsg.message = message;
	delayedMsg.param1 = param1;
	xTimerChangePeriod(xCameraTimer, delay, 0);
	xTimerStart(xCameraTimer, 0);
}

/* Message handlers --------------------------------------------------------- */

/**
 * Initialize the peripherals for this task.
 */
void
setup_sensors() {
	if (!spi_take()) {
		/* Keep trying to obtain access to the SPI bus. */
		camera_tell_delay(MSG_CAMERA_SETUP, 0, 100);
		return;
	}

	trace_printf("camera_task: initialize sensors\n");

	/* Initialize I2C peripherals for this task. */
	ov5642_init();
	lps331_init();
	hts221_init();

	cameraFound = arducam_init();
	if (cameraFound) {
		trace_printf("camera_task: camera installed\n");
	} else {
		trace_printf("camera_task: camera not found\n");
	}

	spi_give();
}

/**
 * Start taking photos and recording samples.
 */
void
start_recording() {
	trace_printf("camera_task: start recording\n");

	xTimerStart(xCameraTimerSample, 0);
	xTimerStart(xCameraTimerPhoto, 0);
}

/**
 * Capture a sample in memory.
 */
void
take_sample() {
	/* Ensure that there is room in the sample buffer. */
	if (samples.Count < SAMPLE_BUFFER_SIZE) {
		/* Record a sample from each sensor. */
		Sample* sample = &samples.Buffer[samples.Count];
		sample->TickCount = xTaskGetTickCount();
		sample->LPS331Temperature = lps331_read_temp_C();
		sample->LPS331Pressure = lps331_read_pres_mbar();
		sample->HTS221Temperature = hts221_read_temp_C();
		sample->HTS221Humidity = hts221_read_hum_rel();
		samples.Count++;

		trace_printf("camera_task: | %2.2f C | %4.0f mbar | %2.2f C | %2.2f %%rHum |\n",
				sample->LPS331Temperature,
				sample->LPS331Pressure,
				sample->HTS221Temperature,
				sample->HTS221Humidity);
	}

	/* Try to persist the samples. */
	Msg msg = { MSG_CAMERA_WRITE_SAMPLES, 0 };
	xQueueSend(xCameraQueue, &msg, 0);
}

/**
 * Open a handle to DATA.LOG.
 */
F_FILE*
open_log() {
	/* Open a handle to append to the data log. */
	F_FILE* hLog = f_open("data.log", "a");
	if (hLog == NULL) {
		trace_printf("camera_task; failed to open data.log\n");
	}
	return hLog;
}

/**
 * Write the sample buffer to DATA.log.
 */
void
write_samples() {
	F_FILE* pxLog = NULL;

	if (!spi_take()) {
		/* SPI bus is not available. */
		return;
	}

	/* Open a handle to the data log. */
	if ((pxLog = open_log()) == NULL) {
		goto error;
	}

	/* Write the samples to the data log. */
	for (uint8_t i = 0; i < samples.Count; ++i) {
		char* format =
				"DATA:{\"tick\":%d,"
				  "\"lps331\":{\"temp\":%.2f,\"pres\":%.2f},"
				  "\"hts221\":{\"temp\":%.2f,\"hum\":%.2f}"
				"}\n";

		char buffer[128];
		Sample* sample = &samples.Buffer[i];
		int16_t wrote = snprintf(buffer, 128, format,
				sample->TickCount,
				sample->LPS331Temperature,
				sample->LPS331Pressure,
				sample->HTS221Temperature,
				sample->HTS221Humidity);

		f_write(buffer, 1, wrote, pxLog);
	}
	f_close(pxLog);
	pxLog = NULL;

	/* Reset the sample buffer. */
	samples.Count = 0;

	/* Fall through and clean up. */
error:
	if (pxLog != NULL) { f_close(pxLog); };
	spi_give();
}

/**
 * Find the next image name for a JPG capture.
 */
uint16_t
next_image_name(char* buffer, uint8_t length) {
	F_FIND xFindStruct;
	for (int i = dcimIndex; i < (dcimIndex + 100); ++i) {
		snprintf(buffer, length, "dcim%d.jpg", i);
		if (f_findfirst(buffer, &xFindStruct) == F_ERR_NOTFOUND) {
			dcimIndex = i + 1;
			return i;
		}
	}
	return 0;
}

/**
 * Capture an image from the camera to the SD card.
 */
void
take_photo() {
	F_FILE *pxJpg = NULL, *pxLog = NULL;

	if (!spi_take()) {
		/* Keep trying to obtain access to the SPI bus. */
		xTimerChangePeriod(xCameraTimer, 100, 0);
		vTimerSetTimerID(xCameraTimer, MSG_CAMERA_PHOTO);
		xTimerStart(xCameraTimer, 0);
		return;
	}

	/* Wake the camera from low power state. */
	if (arducam_standby_clear() != DEVICES_OK) {
		trace_printf("camera_task: wake camera failed\n");
		goto error;
	}

	/* Trigger a new capture. */
	uint32_t remainingBytes;
	if (   arducam_start_capture()               != DEVICES_OK
		|| arducam_wait_capture(&remainingBytes) != DEVICES_OK) {
		trace_printf("camera_task: image capture failed\n");
		goto error;
	}

	/* Choose a file name for this image. */
	char jpgName[32];
	if (   !next_image_name(jpgName, 32)
		|| (pxJpg = f_open(jpgName, "w")) == NULL) {
		trace_printf("camera_task: open JPG failed\n");
		goto error;
	} else {
		trace_printf("camera_task: write to %s\n", jpgName);
	}

	/* Copy the JPG image from the camera flash to the SD card. */
	uint8_t buffer[BURST_READ_LENGTH];
	uint8_t length = MIN(remainingBytes, BURST_READ_LENGTH);
	arducam_burst_read(buffer, length);
	remainingBytes -= length + 1;
	if (f_write(buffer + 1, 1, (length - 1), pxJpg) != (length - 1)) {
		trace_printf("camera_task: write to JPG failed\n");
		goto error;
	}
	for (uint16_t i = 0; remainingBytes > 0; ++i) {
		uint8_t length = MIN(remainingBytes, BURST_READ_LENGTH);
		arducam_burst_read(buffer, length);
		remainingBytes -= length;
		if (f_write(buffer, 1, length, pxJpg) != length) {
			trace_printf("camera_task: write to JPG failed\n");
			goto error;
		}
	}
	f_close(pxJpg);
	pxJpg = NULL;

	/* Open a handle to the data log. */
	if ((pxLog = open_log()) == NULL) {
		goto error;
	}

	/* Write the JPG name to the log. */
	TickType_t tickTime = xTaskGetTickCount();
	int8_t wrote = snprintf(buffer, BURST_READ_LENGTH,
			"FILE:{\"tick\":%d,\"file\":\"%s\"}\n",
			tickTime, jpgName);
	f_write(buffer, 1, wrote, pxLog);

	f_close(pxLog);
	pxLog = NULL;

	trace_printf("camera_task: capture complete\n");

	/* Put the camera from low power state. */
	if (arducam_standby_set() != DEVICES_OK) {
		trace_printf("camera_task: sleep camera failed\n");
		goto error;
	}

	/* Fall through and clean up. */
error:
	if (pxJpg != NULL) { f_close(pxJpg); };
	if (pxLog != NULL) { f_close(pxLog); };
	spi_give();
}

/* Other functions ---------------------------------------------------------- */



