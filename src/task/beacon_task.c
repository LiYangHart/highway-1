#include <peripheral/xbee.h>
#include <task/beacon_task.h>
#include "diag/Trace.h"

/**
 * Initialize the peripherals and state for this task.
 */
uint8_t
beacon_task_setup() {
	/* Initialize the UART to communicate with the XBee radio. */
	xbee_init();

	return 1; // OK
}

/**
 * Task to emit beacons which are received by passing cars.
 */
void
beacon_task(void * pvParameters) {
	//transmit used to mark if module should be configured as transmitter or receiver
	int transmit = 1;
	/* Initialize the peripherals and state for this task. */
	if (!beacon_task_setup()) {
		trace_printf("beacon_task: setup failed\n");
		vTaskDelete(NULL);
		return;
	} else {
		trace_printf("beacon_task: started\n");
	}

	if (transmit == 1)
	{
		//with Xbee setup, want to first enter command mode to set proper fields for Xbee operation
		//done by sending three +++ quickly to Xbee, then waiting for one second
		xbee_write((uint8_t*)"+++", 3);

		//after waiting for one delay, check to see if UART responded with "OK"
		vTaskDelay(1000);

		while (xbee_count() > 0){
			trace_printf("%c", xbee_getc());
		}

		//once in command mode, begin setting parameters, look for OK after each one
		//note that current settings are for transmitter, carriage return '\r' needed on end of command strings
		//after setting parameter, field is read back to make sure entered properly

		//checking preamble and network IDS)
		xbee_write((uint8_t*)"ATHP\r", 5);
		vTaskDelay(10);
		trace_printf("preamble: \n");
		while (xbee_count() > 0){
			trace_printf("%c", xbee_getc());
		}

		xbee_write((uint8_t*)"ATID\r", 5);
		vTaskDelay(10);
		trace_printf("network ID: \n");
		while (xbee_count() > 0){
         		trace_printf("%c", xbee_getc());
		}

		//setting power level of Xbee to lowest level to begin
		xbee_write((uint8_t*)"ATPL0\r", 6);
		vTaskDelay(10);
		trace_printf("power level change: \n");
		while (xbee_count() > 0){
             	trace_printf("%c", xbee_getc());
        }

		//set transmitter to not act as node in mesh  network
		xbee_write((uint8_t*)"ATCE2\r", 6);
		vTaskDelay(10);
		trace_printf("node messaging change: \n");
		while (xbee_count() > 0){
		        trace_printf("%c", xbee_getc());
		}

		//setting network hops max to 1, shouldn't be needed but doesn't seem to have a default
		xbee_write((uint8_t*)"ATNH1\r", 6);
		vTaskDelay(10);
		trace_printf("network hop change: \n");
		while (xbee_count() >0){
			trace_printf("%c", xbee_getc());
		}

		//setting destination address for broadcast transmissions, needs to be 0x000000000000FFFF
		xbee_write((uint8_t*)"ATDHFFFFFFFF\r", 13);
		vTaskDelay(10);
		trace_printf("destination address high set: \n");
		while (xbee_count() >0){
			trace_printf("%c", xbee_getc());
		}

		xbee_write((uint8_t*)"ATDL0000FFFF\r", 13);
		vTaskDelay(10);
		trace_printf("destination address low set: \n");
		while (xbee_count() >0){
				trace_printf("%c", xbee_getc());
		}

		//set transmit options for point-to-multipoint, acknlowedgements disabled
		xbee_write((uint8_t*)"ATTO41\r", 7);
		trace_printf("transmission options set: \n");
		vTaskDelay(10);
		while (xbee_count() >0){
				trace_printf("%c", xbee_getc());
		}

		//set node name for transmitter
		xbee_write((uint8_t*)"ATNItransmit\r", 12);
		trace_printf("node name set: \n");
		vTaskDelay(10);
		while (xbee_count() >0){
				trace_printf("%c", xbee_getc());
		}

		//check current baud rate.  Might want to change this in future
		xbee_write((uint8_t*)"ATBD\r", 5);
		trace_printf("Baud rate value: \n");
		vTaskDelay(10);
		while (xbee_count() >0){
				trace_printf("%c", xbee_getc());
		}

		//as most I/O pins are not connected on shield, disabling for now
		xbee_write((uint8_t*)"ATD00\r", 6);
		trace_printf("D0 changed: \n");
		vTaskDelay(10);
		while (xbee_count() >0){
				trace_printf("%c", xbee_getc());
		}

		xbee_write((uint8_t*)"ATD50\r", 6);
		trace_printf("D1 changed: \n");
		vTaskDelay(10);
		while (xbee_count() >0){
				trace_printf("%c", xbee_getc());
		}

		xbee_write((uint8_t*)"ATD70\r", 6);
		trace_printf("D7 changed: \n");
		vTaskDelay(10);
		while (xbee_count() >0){
				trace_printf("%c", xbee_getc());
		}

		xbee_write((uint8_t*)"ATD80\r", 6);
		trace_printf("D8 changed: \n");
		vTaskDelay(10);
		while (xbee_count() > 0){
			trace_printf("%c", xbee_getc());
		}

		xbee_write((uint8_t*)"ATD90\r", 6);
		trace_printf("D9 changed: \n");
		vTaskDelay(10);
		while (xbee_count() > 0){
				trace_printf("%c", xbee_getc());
		}

		xbee_write((uint8_t*)"ATP00\r", 6);
		trace_printf("D10 changed: \n");
		vTaskDelay(10);
		while (xbee_count() > 0){
				trace_printf("%c", xbee_getc());
		}

		//setting pins for pull-up capability
		xbee_write((uint8_t*)"ATPD7FFF\r", 9);
		trace_printf("Pull-up changed: \n");
		vTaskDelay(10);
		while (xbee_count() > 0){
				trace_printf("%c", xbee_getc());
		}

		//setting analog voltage reference
		xbee_write((uint8_t*)"ATAV1\r", 6);
		trace_printf("voltage reference value changed: \n");
		vTaskDelay(10);
		while (xbee_count() > 0){
				trace_printf("%c", xbee_getc());
		}

		//after entering commands to set fields, need to apply changes and exit command mode
		xbee_write((uint8_t*)"ATAC\r", 5);
		vTaskDelay(10);
		xbee_write((uint8_t*)"ATCN\r", 5);
		vTaskDelay(10);

	}


	for (;;) {
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
