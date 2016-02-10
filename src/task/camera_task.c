#include <peripheral/arducam.h>
#include <task/camera_task.h>
#include <fat_sl.h>
#include <mdriver_spi_sd.h>

/**
 * Initialize the peripherals and state for this task.
 */
uint8_t
camera_task_setup() {
	/* Initialize peripherals for this task. */
	ov5642_init();
	lps331_init();
	hts221_init();

	arducam_init();

	/* Try to mount the SD card. */
	if (fn_initvolume(mmc_spi_initfunc) != F_NO_ERROR) {
		trace_printf("sdcard: failed to mount volume\n");
		return 0;
	}

	return 1; // OK
}


/**
 * Find the first free filename in the format capN.jpg, where N is an integer
 * equal to or greater than 'start'.
 */
uint16_t
getCaptureIndex(char* buffer, uint8_t length, uint16_t start) {
	F_FIND xFindStruct;
	for (int i = start; i < 100; ++i) {
		snprintf(buffer, length, "cap%d.jpg", i);
		if (f_findfirst(buffer, &xFindStruct) == F_ERR_NOTFOUND) {
			return i;
		}
	}
	return 0;
}

/**
 * Capture an image to the SD card every 15 seconds.
 * TODO Implement low power mode for the camera, as it gets very warm during
 * extended operation.
 */
void
camera_task(void * pvParameters) {
	/* Initialize the peripherals and state for this task. */
	if (!camera_task_setup()) {
		trace_printf("camera_task: setup failed\n");
		vTaskDelete(NULL);
		return;
	} else {
		trace_printf("camera_task: started\n");
	}

	const uint8_t BURST_LENGTH = 64;
	uint8_t buffer[BURST_LENGTH], read;
	char filename[16];

	for (int i = 0;; ++i) {
		/* Trigger a new capture. */
		if (arducam_start_capture() != DEVICES_OK) {
			trace_printf("camera: couldn't start capture\n");
			break;
		} else {
			trace_printf("camera: starting capture\n");
		}

		/* Wait for the capture to complete. */
		uint32_t capture_length;
		if (arducam_wait_capture(&capture_length) != DEVICES_OK) {
			trace_printf("camera: capture flag not asserted\n");
			break;
		} else {
			trace_printf("camera: captured image is %d bytes\n", capture_length);
		}

		/* Create an image file on the SD card. */
		getCaptureIndex(filename, 16, i);
		F_FILE* hJpeg = f_open(filename, "w");
		if (hJpeg == NULL) {
			trace_printf("sdcard: open %s failed\n", filename);
		} else {
			trace_printf("sdcard: writing to %s\n", filename);
		}

		/* Read the first burst and discard the dummy byte. */
		read = (capture_length > BURST_LENGTH) ? BURST_LENGTH : capture_length;
		arducam_burst_read(buffer, read);
		capture_length -= read + 1;
		if (f_write(buffer + 1, 1, (read - 1), hJpeg) != (read - 1)) {
			trace_printf("write failed\n");
			return;
		}

		/* Read the remaining bytes. */
		while (capture_length > 0) {
			read = (capture_length > BURST_LENGTH) ? BURST_LENGTH : capture_length;
			arducam_burst_read(buffer, read);
			capture_length -= read;
			if (f_write(buffer, 1, read, hJpeg) != read) {
				trace_printf("write failed\n");
			}
		}

		/* Close the image file. */
		if (f_close(hJpeg) != F_NO_ERROR) {
			trace_printf("close() failed\n");
		}

		trace_printf("capture complete\n");
		vTaskDelay(15000);
	}
}


/*
 *	trace_printf("lps331: %.2f degC %.2f mbar\n",
				lps331_read_temp_C(),
				lps331_read_pres_mbar());

	trace_printf("hts221: %.2f degC %.2f %%rH\n",
			hts221_read_temp_C(),
			hts221_read_hum_rel());

	trace_printf("stlm75: %.2f degC\n",
			stlm75_read_temp_C());
 */
