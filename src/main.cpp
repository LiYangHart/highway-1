#include <devices.h>
#include <stdio.h>
#include <stdlib.h>
#include "diag/Trace.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include <fat_sl.h>
#include <api_mdriver_spi_sd.h>

#include "skywire.h"
#include "virtual_com.h"
#include "arducam.h"
#include "lps331.h"
#include "hts221.h"
#include "stlm75.h"

#include <stdio.h>      /* printf */
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */



/**
 * The Skywire demo task is a simple bridge that connects the virtual COM port
 * to the Skywire UART. Connect to the virtual COM port from a PC and input
 * commands directly to the Skywire modem.
 */
void
skywireTask(void* pvParameters) {
	/* Configure the UART(s) now that the HAL is initialized and the clock is
	   running at full speed. */
	vcp_init();
	skywire_init();

	/* Execute the reset sequence - this takes about 15 seconds but is
	   required to enable the Skywire modem. */
	skywire_activate();

	/* Implement a simple terminal to relay commands to the Skywire modem. */
	trace_printf("Starting Skywire terminal\n");

	HAL_UART_Transmit(vcp_handle(), (uint8_t*)"Started\n", 8, 1000);

	while(1) {

		uint8_t b;
		while (vcp_count() > 0) {
			b = vcp_getc();

			/* Echo characters input to the terminal. */
			HAL_UART_Transmit(vcp_handle(), &b, 1, 1000);

			if (b == '\n') {
				b = '\r';
				HAL_UART_Transmit(skywire_handle(), &b, 1, 0);
				b = '\n';
				HAL_UART_Transmit(skywire_handle(), &b, 1, 0);
			} else {
				HAL_UART_Transmit(skywire_handle(), &b, 1, 0);
			}
		}

		while(skywire_count() > 0) {
			b = skywire_getc();
			HAL_UART_Transmit(vcp_handle(), &b, 1, 1000);
		}

		/* Yield to other tasks. */
		vTaskDelay(1);
	}
}

/**
 * The sensor demo task prints the temperature, pressure and humidity on the
 * debug channel every five seconds.
 */
void
sensorsTask(void* pvParameters) {
	i2c_init();
	for(;;) {
		vTaskDelay(5000);

		trace_printf("lps331: %.2f degC %.2f mbar\n",
				lps331_read_temp_C(),
				lps331_read_pres_mbar());

		trace_printf("hts221: %.2f degC %.2f %%rH\n",
				hts221_read_temp_C(),
				hts221_read_hum_rel());

		trace_printf("stlm75: %.2f degC\n",
				stlm75_read_temp_C());
	}
}

/**
 * Find the first free filename in the format capN.jpg, where N is an integer
 * equal to or greater than 'start'.
 */
uint16_t getCaptureIndex(char* buffer, uint8_t length, uint16_t start) {
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
cameraTask(void* pvParameters) {
	const int BURST_LENGTH = 64;
	uint8_t buffer[BURST_LENGTH], read;
	char filename[16];

	spi_init();

	vTaskDelay(2000);

	i2c_init();

	/* Try to mount the SD card. */
	if (fn_initvolume(mmc_spi_initfunc) != F_NO_ERROR) {
		trace_printf("sdcard: failed to mount volume\n");
		return;
	}

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

int
main(int argc, char* argv[])
{
	//xTaskCreate(sensorsTask, "Sen", (unsigned short)384, (void *)NULL, tskIDLE_PRIORITY, NULL);
	//xTaskCreate(skywireTask, "Sky", (unsigned short)384, (void *)NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(cameraTask,  "Cam", (unsigned short)1024, (void *)NULL, tskIDLE_PRIORITY, NULL);
	vTaskStartScheduler();
	for (;;) {}
}

extern "C" void
vApplicationTickHook( void )
{
	/* Have this method call HAL_IncTick(). However, HAL_Delay() would
	   then be tick-based rather than millisecond based. Alternatively,
	   implement HAL_GetTick to use FreeRTOS timing. */
	/*HAL_IncTick();*/
}

uint32_t HAL_GetTick(void)
{
  return xTaskGetTickCount();
}

extern "C" void
vApplicationIdleHook( void )
{

}

extern "C" void
vApplicationMallocFailedHook( void )
{
	taskDISABLE_INTERRUPTS();
	for( ;; );
}

extern "C" void
vApplicationStackOverflowHook(TaskHandle_t pxTask, char* pcTaskName)
{
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
