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
		if (HAL_UART_Receive(vcp_handle(), &b, 1, 1000) == HAL_OK) {
			trace_printf("%c", b);

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
			trace_printf("%c", skywire_getc());
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
		trace_printf("lps331: %.2f degC %.2f mbar\n",
				lps331_read_temp_C(),
				lps331_read_pres_mbar());

		trace_printf("hts221: %.2f degC %.2f %%rH\n",
				hts221_read_temp_C(),
				hts221_read_hum_rel());

		trace_printf("stlm75: %.2f degC\n",
				stlm75_read_temp_C());

		vTaskDelay(5000);
	}
}

int
main(int argc, char* argv[])
{
	xTaskCreate(sensorsTask, "HW", (unsigned short)768, (void *)NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(skywireTask, "SW", (unsigned short)384, (void *)NULL, tskIDLE_PRIORITY, NULL);
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
