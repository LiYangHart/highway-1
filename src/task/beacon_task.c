#include <peripheral/xbee.h>
#include <task/beacon_task.h>
#include "diag/Trace.h"

QueueHandle_t xSLUpdatesQueue;

/**
 * Initialize the peripherals and state for this task.
 */
uint8_t
beacon_task_setup() {
	/* Create an IPC queue to receive updates from the Skywire task. */
	xSLUpdatesQueue = xQueueCreate( 10, sizeof(SLUpdate));

	/* Initialize the UART to communicate with the XBee radio. */
	xbee_init();

	return 1; // OK
}

/**
 * Task to emit beacons which are received by passing cars.
 */
void
beacon_task(void * pvParameters) {
	/* Initialize the peripherals and state for this task. */
	if (!beacon_task_setup()) {
		trace_printf("beacon_task: setup failed\n");
		vTaskDelete(NULL);
		return;
	} else {
		trace_printf("beacon_task: started\n");
	}

	for (;;) {
		SLUpdate slUpdate;
		if (xQueueReceive(xSLUpdatesQueue, &slUpdate, 0) == pdTRUE) {
			trace_printf("beacon task: SL = %d\n", slUpdate.limit);
		}

		/**
		 * TODO The Skywire task will periodically communicate with the MCC.
		 * When updated SL values are received, they will be communicated to
		 * this task via IPC mailbox.
		 */

		/**
		 * See: xbee.h and dma_serial.h
		 * xbee_count() - number of bytes available from XBee UART
		 * xbee_getc()  - get one byte form XBee UART
		 * xbee_read()  - read bytes into a buffer, non-blocking, **not implemented yet**
		 * xbee_write() - blocking write - uses HAL_UART_Transmit
		 */

		/* Echo received bytes from UART6 to the console. */
		while (xbee_count() > 0) {
			trace_printf("%c", xbee_getc());
		}

		/* Run task at ~1Hz for now. */
		vTaskDelay(1000);
	}
}
