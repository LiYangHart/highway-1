#include <peripheral/i2c_spi_bus.h>
#include <stdio.h>
#include <stdlib.h>

#include "diag/Trace.h"

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include "FreeRTOS.h"
#include "task.h"

#include <task/skywire_task.h>
#include <task/beacon_task.h>
#include <task/camera_task.h>
#include <task/receive_task.h>

/* Enable or disable tasks for development. */
#define SKYWIRE_TASK 0
#define BEACON_TASK 1
#define CAMERA_TASK 0
#define RECEIVE_TASK 0


void
setup_task(void * pvParameters) {
	/* Initialize the I2C bus. */
	trace_printf("initialize I2C bus\n");
	i2c_bus_init();

	/* Initialize the SPI bus. */
	trace_printf("initialize SPI bus\n");
	spi_bus_init();

	#if BEACON_TASK
	trace_printf("starting beacon task\n");
	xTaskCreate(beacon_task,
			BEACON_TASK_NAME,
			BEACON_TASK_STACK_SIZE,
			(void *)NULL,
			tskIDLE_PRIORITY,
			NULL);
	#endif

	#if CAMERA_TASK
	trace_printf("starting camera task\n");
	xTaskCreate(camera_task,
				CAMERA_TASK_NAME,
				CAMERA_TASK_STACK_SIZE,
				(void *)NULL,
				tskIDLE_PRIORITY,
				NULL);
	#endif

	#if SKYWIRE_TASK
	trace_printf("starting skywire task\n");
	xTaskCreate(skywire_task,
			SKYWIRE_TASK_NAME,
			SKYWIRE_TASK_STACK_SIZE,
			(void *)NULL,
			tskIDLE_PRIORITY,
			NULL);
	#endif

	#if RECEIVE_TASK
	trace_printf("starting receiver task\n");
	xTaskCreate(receive_task,
			RECEIVE_TASK_NAME,
			RECEIVE_TASK_STACK_SIZE,
			(void *)NULL,
			tskIDLE_PRIORITY,
			NULL);
	#endif

	/* Delete the setup task. */
	vTaskDelete(NULL);
	trace_printf("Setup complete\n");
}

int
main(int argc, char* argv[])
{
	xTaskCreate(setup_task, "Setup", 384, (void *)NULL, tskIDLE_PRIORITY, NULL);
	vTaskStartScheduler();
	for (;;) {}
}

// -----------------------------------------------------------------------------

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
	for (;;);
}

extern "C" void
vApplicationStackOverflowHook(TaskHandle_t pxTask, char* pcTaskName)
{
	taskDISABLE_INTERRUPTS();
	for (;;);
}
