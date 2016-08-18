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

void camera_task(void * pvParameters);

void camera_timer_elapsed(TimerHandle_t xTimer);
void camera_timer_sample(TimerHandle_t xTimer);
void camera_timer_photo(TimerHandle_t xTimer);

void camera_task_setup();
void camera_task_mount_sd();
void camera_task_clean_sd();
void camera_task_take_sample();
void camera_task_write_samples();
void camera_task_take_photo();

QueueHandle_t xCameraQueue;
TimerHandle_t xCameraTimer;
TimerHandle_t xCameraTimerSample;
TimerHandle_t xCameraTimerPhoto;
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
	xCameraTimer = xTimerCreate("CAM-STA", 1000, pdFALSE, (void*)0,
			camera_timer_elapsed);
	if (xCameraTimer == NULL) goto error;

	/* Timer used to schedule samples every 1 seconds. */
	xCameraTimerSample = xTimerCreate("CAM-SAM", CAMERA_SAMPLE_INTERVAL,
			pdTRUE, (void*)0, camera_timer_sample);
	if (xCameraTimerSample == NULL) goto error;

	/* Timer used to schedule photos every 60 seconds. */
	xCameraTimerPhoto = xTimerCreate("CAM-IMG", CAMERA_PHOTO_INTERVAL,
			pdTRUE, (void*)0, camera_timer_photo);
	if (xCameraTimerPhoto == NULL) goto error;

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
 * Record sensor data and capture images.
 * Activity is recorded to DATA.LOG to be picked up by the Skywire task.
 */
void
camera_task(void * pvParameters __attribute__((unused))) {
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
			camera_task_setup();
			break;
		case MSG_CAMERA_MOUNT_SD:
			/* Mount the volume on the SD card. */
			camera_task_mount_sd();
			break;
		case MSG_CAMERA_CLEAN_SD:
			/* Delete all files on the SD card. */
			camera_task_clean_sd();
			break;
		case MSG_CAMERA_SAMPLE:
			/* Capture a sample in memory. */
			camera_task_take_sample();
			break;
		case MSG_CAMERA_WRITE_SAMPLES:
			/* Write samples to the SD card. */
			camera_task_write_samples();
			break;
		case MSG_CAMERA_PHOTO:
			/* Capture a photo to the SD card. */
			camera_task_take_photo();
			break;
		}
	}
}

// -----------------------------------------------------------------------------

/**
 * Timer callback to initiate state transitions.
 */
void
camera_timer_elapsed(TimerHandle_t xTimer) {
	Msg msg;
	/* The message to send is stored in the timer ID. */
	msg.message = (uint32_t)pvTimerGetTimerID(xTimer);
	msg.param1 = 0;
	xQueueSend(xCameraQueue, &msg, 0);
}

/**
 * Timer callback to initiate a sample.
 */
void
camera_timer_sample(TimerHandle_t xTimer __attribute__((unused))) {
	Msg msg = { MSG_CAMERA_SAMPLE, 0 };
	xQueueSend(xCameraQueue, &msg, 0);
}

/**
 * Timer callback to initiate a photo.
 */
void
camera_timer_photo(TimerHandle_t xTimer __attribute__((unused))) {
	Msg msg = { MSG_CAMERA_PHOTO, 0 };
	xQueueSend(xCameraQueue, &msg, 0);
}

// -----------------------------------------------------------------------------

/**
 * Initialize the peripherals for this task.
 */
void
camera_task_setup() {
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
	msg.message = MSG_CAMERA_MOUNT_SD;
	xQueueSend(xCameraQueue, &msg, 0);
}

/**
 * Mount the volume on the SD card.
 */
void
camera_task_mount_sd() {
	spi_take();
	/* Initialize the FAT volume on the SD card. */
	if (fn_initvolume(mmc_spi_initfunc) != F_NO_ERROR) {
		goto error;
	}
	spi_give();

#ifdef CAMERA_CLEAN_SD
	/* Delete all files on the SD card. */
	Msg msg = { MSG_CAMERA_CLEAN_SD, 0 };
	xQueueSend(xCameraQueue, &msg, 0);
#else
	/* Enable the timers for samples and photos. */
	xTimerStart(xCameraTimerSample, 0);
	xTimerStart(xCameraTimerPhoto, 0);
#endif

	return;

error:
	trace_printf("camera_task: failed to mount volume\n");
	spi_give();

	/* Retry in one second. */
	xTimerChangePeriod(xCameraTimer, 1000, 0);
	vTimerSetTimerID(xCameraTimer, (void*)MSG_CAMERA_MOUNT_SD);
	xTimerStart(xCameraTimer, 0);
}

/**
 * Delete all files on the SD card.
 */
void
camera_task_clean_sd() {
	if (!spi_take()) {
		/* Keep trying to obtain access to the SPI bus. */
		xTimerChangePeriod(xCameraTimer, 1000, 0);
		vTimerSetTimerID(xCameraTimer, (void*)MSG_CAMERA_CLEAN_SD);
		xTimerStart(xCameraTimer, 0);
	}

	trace_printf("camera_task: cleaning SD card\n");
	f_delete("data.log");
	F_FIND xFindStruct;
	if (f_findfirst("*.JPG", &xFindStruct) == F_NO_ERROR) {
		do {
			f_delete(xFindStruct.filename);
		} while (f_findnext(&xFindStruct) == F_NO_ERROR);
	}

	if (f_findfirst("upload", &xFindStruct) != F_NO_ERROR) {
		f_mkdir("upload");
	}

	f_chdir("upload");

	spi_give();

	/* Enable the timers for samples and photos. */
	xTimerStart(xCameraTimerSample, 0);
	xTimerStart(xCameraTimerPhoto, 0);
}

/**
 * Capture a sample in memory.
 */
void
camera_task_take_sample() {
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
camera_task_write_samples() {
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
camera_task_take_photo() {
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

