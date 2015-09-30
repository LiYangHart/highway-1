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


/**
 * Implement a terminal that
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

		while(skywire_available() > 0) {
			trace_printf("%c", skywire_getchar());
		}

		/* Yield to other tasks. */
		vTaskDelay(1);
	}
}

void
testTask(void* pvParameters) {

}

int
main(int argc, char* argv[])
{
	//xTaskCreate(testTask, "HW", (unsigned short)768, (void *)NULL, tskIDLE_PRIORITY, NULL);
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
