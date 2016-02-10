#include <peripheral/skywire.h>
#include <peripheral/virtual_com.h>
#include <task/skywire_task.h>

/**
 * Initialize the peripherals and state for this task.
 */
uint8_t
skywire_task_setup() {
	vcp_init();
	skywire_init();

	/* Execute the reset sequence - this takes about 15 seconds but is
	   required to enable the Skywire modem. */
	skywire_activate();

	return 1; // OK
}

/**
 * The Skywire demo task is a simple bridge that connects the virtual COM port
 * to the Skywire UART. Connect to the virtual COM port from a PC and input
 * commands directly to the Skywire modem.
 */
void
skywire_task(void* pvParameters) {
	/* Initialize the peripherals and state for this task. */
	if (!skywire_task_setup()) {
		trace_printf("skywire_task: setup failed\n");
		vTaskDelete(NULL);
		return;
	} else {
		trace_printf("skywire_task: started\n");
	}

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
