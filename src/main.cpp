#include <api_mdriver_spi_sd.h>
#include <stdio.h>
#include <stdlib.h>
#include "diag/Trace.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include <fat_sl.h>
#include <skywire.h>
#include <virtual_com.h>
#include "sensors.h"
#include "lps331.h"
#include "hts221.h"
#include "stlm75.h"

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
	sensors_init();
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

void
cameraTask(void* pvParameters) {
	vTaskDelay(2000);

	/* Try to mount the SD card. */
	if (fn_initvolume(mmc_spi_initfunc) != F_NO_ERROR) {
		trace_printf("sdcard: failed to mount volume\n");
		return;
	}

	/* List files on the root of the SD card. */
	F_FIND xFindStruct;
	trace_printf("sdcard: list files in root\n");
	if (f_findfirst( "/*.*", &xFindStruct) == F_NO_ERROR) {
		do {
			trace_printf("%s", xFindStruct.filename);
			if( ( xFindStruct.attr & F_ATTR_DIR ) != 0 ) {
				trace_printf (" -> dir\n");
			} else {
				trace_printf (" -> file %d bytes\n", xFindStruct.filesize);
			}
		} while(f_findnext(&xFindStruct) == F_NO_ERROR );
	}

	for (;;) {
		vTaskDelay(5000);
	}
}

int
main(int argc, char* argv[])
{
	xTaskCreate(sensorsTask, "Sen", (unsigned short)384, (void *)NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(skywireTask, "Sky", (unsigned short)384, (void *)NULL, tskIDLE_PRIORITY, NULL);
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
