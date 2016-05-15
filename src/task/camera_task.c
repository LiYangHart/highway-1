#include <peripheral/arducam.h>
#include <peripheral/lps331.h>
#include <peripheral/hts221.h>
#include <task/camera_task.h>
#include <fat_sl.h>
#include <mdriver_spi_sd.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <string.h>

uint8_t arduCamInstalled = 0;
SampleBuffer samples;
uint16_t dcimIndex = 1;
uint8_t gpio_regval = 0;


/**
 * Initialize the peripherals and state for this task.
 */
uint8_t
camera_task_setup() {
	/* Initialize I2C peripherals for this task. */
	ov5642_init();
	lps331_init();
	hts221_init();

	/* Intialize SPI peripherals for this task. */
	spi_take();
	arduCamInstalled = arducam_init();

	/* Try to mount the SD card. */
	if (fn_initvolume(mmc_spi_initfunc) != F_NO_ERROR) {
		trace_printf("sdcard: failed to mount volume\n");
		spi_give();
		return 0;
	}

//#ifdef FORMAT_SD_CARD
	/*format SD card if not formatted*/
/*	trace_printf("Attempting to format SD card \n");
	trace_printf("Assuming FAT32 based on size of card\n");
	if (fn_hardformat(F_FAT32_MEDIA) != F_NO_ERROR)
	{
		trace_printf("Format failed\n");
	}
	else
	{
		trace_printf("Format Success\n");
	}
	while(1)
	{
		trace_printf("Format done\n");
		vTaskDelay(10000);
	}
#endif*/

#ifdef CLEAN_SD_CARD
	/* Delete DATA.LOG and JPG files from the SD card. */
	trace_printf("camera_task: cleaning SD card\n");
	f_delete("DATA.LOG");
	f_delete("file.txt");
	F_FIND xFindStruct;
	if (f_findfirst("*.JPG", &xFindStruct) == F_NO_ERROR) {
		do {
			f_delete(xFindStruct.filename);
		} while (f_findnext(&xFindStruct) == F_NO_ERROR);
	}
#endif


	spi_give();
	return 1; // OK
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
 * Capture a reading of the sensors into the sample buffer.
 * Write the sample buffer to the SD card if access is available.
 */
void
capture_sample(TickType_t* lastReading) {
	trace_printf("reading sensors\n");

	F_FILE* pxLog = NULL;

	/* Record values from each sensor. */
	Sample* sample = &samples.Buffer[samples.Count];
	sample->TickCount = xTaskGetTickCount();
	sample->LPS331Temperature = lps331_read_temp_C();
	sample->LPS331Pressure = lps331_read_pres_mbar();
	sample->HTS221Temperature = hts221_read_temp_C();
	sample->HTS221Humidity = hts221_read_hum_rel();

	/* Successfully collected a sample. */
	*lastReading = sample->TickCount;
	samples.Count++;

	/* Try to obtain access to the SD card. */
	if (!spi_take()) {
		/* SPI is busy - nothing more to do. */
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
capture_image(TickType_t* lastCapture) {
	F_FILE *pxJpg = NULL, *pxLog = NULL;

	if (!spi_take()) {
		/* Need exclusive access to the SPI bus. */
		return;
	}

	trace_printf("reading camera\n");

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

	/* Successfully captured an image. */
	*lastCapture = xTaskGetTickCount();

	/* Fall through and clean up. */
error:
	if (pxJpg != NULL) { f_close(pxJpg); };
	if (pxLog != NULL) { f_close(pxLog); };
	spi_give();
}

/**
 * Record sensor data and capture images.
 * Activity is recorded to DATA.LOG to be picked up by the Skywire task.
 */
void
camera_task(void * pvParameters) {

	int low_pow = 0;

	/* Initialize the peripherals and state for this task. */
	if (!camera_task_setup()) {
		trace_printf("camera_task: setup failed\n");
		vTaskDelete(NULL);
		return;
	} else {
		trace_printf("camera_task: started\n");
	}

	/* Initialize timing for capture sub-tasks. */
	TickType_t lastReading, lastCapture;
	lastReading = lastCapture = xTaskGetTickCount();

	/* Initialize the sample buffer. */
	samples.Count = 0;

	for (;;) {
		TickType_t currentTicks = xTaskGetTickCount();

		if ((currentTicks - lastReading) >= SAMPLE_RATE_MS) {
			capture_sample(&lastReading);
		}

		//point at which image is captured from camera
		if (arduCamInstalled) {
			if ((currentTicks - lastCapture) >= IMAGE_RATE_MS) {
				//if camera is currently in low power mode, exit low power mode before taking image
				if (low_pow == 1){
					if(arducam_low_power_remove() != DEVICES_OK){
						trace_printf("Error removing low power mode \n");
					}
					else{
						low_pow = 0;
					}
				}

				//only try taking image if low power has been properly disabled
				if (low_pow == 0){
					capture_image(&lastCapture);
				}

				//after image has been taken, put camera down into low-power mode
				if (low_pow == 0){
					if(arducam_low_power_set() != DEVICES_OK){
						trace_printf("Error setting low power mode \n");
					}
					else{
						trace_printf("Low power mode set \n");
						low_pow = 1;
					}
				}
			}
			vTaskDelay(100);
		}
	}

}
