#include <peripheral/xbee.h>
#include <task/beacon_task.h>
#include <stdio.h>
#include <string.h>
#include <hayes.h>
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

//function to do configuration of Xbee module acting as transmitter
uint8_t
xbee_transmit_setup(ATDevice* xbee_transmit) {
	//with Xbee setup, want to first enter command mode to set proper fields for Xbee operation
	//done by sending three +++ quickly to Xbee, then waiting for one second
	if (hayes_at(xbee_transmit, "+++")								!= HAYES_OK
		|| hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1050)	!= HAYES_OK){
			trace_printf("Failed to enter command mode \n");
			return 0;
	}
	else {
		trace_printf("Entered command mode\n");
	}

	//once in command mode, begin setting parameters, look for OK after each one
	//note that current settings are for transmitter, carriage return '\r' needed on end of command strings
	//setting parameters first, will read back certain critical ones after

	if (hayes_at(xbee_transmit, "ATPL0\r")							!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_transmit, "ATCE2\r")						!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_transmit, "ATNH1\r")						!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_transmit, "ATDH00000000\r")					!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_transmit, "ATDL0000FFFF\r")					!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_transmit, "ATTO41\r")						!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_transmit, "ATD00\r")						!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_transmit, "ATD50\r")						!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_transmit, "ATD70\r")						!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_transmit, "ATD80\r")						!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_transmit, "ATPD7FFF\r")						!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_transmit, "ATAV1\r")						!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1200)		!= HAYES_OK) {
			trace_printf("Sending configuration commands failed \n");
			return 0;
	}

	else{
		trace_printf("Configuration commands run \n");
	}

	//after sending commands out, want to read back a few values
	//if not as expected, send command to correct

	//first, checking preamble and network IDs
	if (hayes_at(xbee_transmit, "ATHP\r")							!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "0\r", 1000)		!= HAYES_OK) {
			trace_printf("Preamble not set correctly, changing \n");
			if (hayes_at(xbee_transmit, "ATHP0\r")							!= HAYES_OK
				||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1000)		!= HAYES_OK) {
				trace_printf("Error setting preamble \n");
				return 0;
			}
	}

	else{
		trace_printf("Preamble okay \n");
	}

	if (hayes_at(xbee_transmit, "ATID\r")								!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "7FFF\r", 1000)		!= HAYES_OK) {
			trace_printf("Network ID not set correctly, changing \n");
			if (hayes_at(xbee_transmit, "ATID7FFF\r")						!= HAYES_OK
				||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1000)		!= HAYES_OK) {
					trace_printf("Error setting network ID \n");
					return 0;
			}
	}

	else{
		trace_printf("Network ID okay \n");
	}

	//checking baud rate
	if (hayes_at(xbee_transmit, "ATBD\r")								!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "3\r", 1000)			!= HAYES_OK) {
			trace_printf("Baud rate not set correctly, changing \n");
			if (hayes_at(xbee_transmit, "ATBD3\r")							!= HAYES_OK
				||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1000)		!= HAYES_OK) {
					trace_printf("Error setting baud rate \n");
					return 0;
			}
	}

	else{
		trace_printf("Baud rate okay \n");
	}

	//double check power and destination address fields
	if (hayes_at(xbee_transmit, "ATPL\r")										!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "0\r", 1200)				!= HAYES_OK ){
		trace_printf("Error in checking power level \n");
		return 0;
	}
	else {
		trace_printf("Power level okay \n");
	}

	if (hayes_at(xbee_transmit, "ATDH\r")									!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "0\r", 1200)		!= HAYES_OK){
		trace_printf("Error with destination high \n");
		return 0;
	}
	else {
		trace_printf("Destination high okay \n");
	}


	if (hayes_at(xbee_transmit, "ATDL\r")									!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "FFFF\r", 1200)		!= HAYES_OK) {
			trace_printf("Error with destination low \n");
			return 0;
	}

	else {
		trace_printf("Parameter check okay \n");
	}

	//after configuring/checking everything, apply changes and exit command mode
	if (hayes_at(xbee_transmit, "ATAC\r")							!= HAYES_OK
		||hayes_res(xbee_transmit, pred_ends_with, "OK\r", 1000)		!= HAYES_OK
		||hayes_at(xbee_transmit, "ATCN\r")							!= HAYES_OK) {
			trace_printf("Error applying changes \n");
			return 0;
	}

	//if this point reached, okay on set-up
	trace_printf("Xbee set-up complete \n");
	return 1;
}


/**
 * Task to emit beacons which are received by passing cars.
 */
void
beacon_task(void * pvParameters) {
	//transmit used to mark if module should be configured as transmitter or receiver
	//int transmit = 1;
	//value used to hold value of set-up, start at '0' then set to '1' if done correctly
	int set_up_okay = 0;
	char buffer[512];

	//AT device for use with hayes commands
	ATDevice xbee_transmit;
	xbee_transmit.api.count = xbee_count;
	xbee_transmit.api.getc = xbee_getc;
	xbee_transmit.api.write = xbee_write;
	xbee_transmit.buffer = buffer;
	xbee_transmit.length = 512;

	//variable to track number of bad transmissions and allowed threshold before a module reset
	int threshold = 10;
	int error_count = 0;

	//speed used to hold speed value to transmit through xbee and set constants for string construction
	int speed = 100;
	int prev_speed = 100;
	int u_limit = 255;
	int l_limit = 1;
	char text_start[5] = "SL: \0";
	char text_end[7] = " km/hr\0";
	char speed_string[4];
	char speed_cat[4];
	char send_string[14];
	char zero_one[2] = "0\0";
	char zero_two[3] = "00\0";
	char dollar[2]= "$\0";

	/* Initialize the peripherals and state for this task. */
	if (!beacon_task_setup()) {
		trace_printf("beacon_task: setup failed\n");
		vTaskDelete(NULL);
		return;
	} else {
		trace_printf("beacon_task: started\n");
	}

	//once task has started, run continuously
	for(;;){
		//configuration for module acting as transmitter

		//first, check to see if configuration has been done properly
		if (set_up_okay == 0){
			//call function to configure transmitting xbee module as well as configuring serial just in case
			xbee_init();

			if(!xbee_transmit_setup(&xbee_transmit)){
				trace_printf("Xbee configuration failed, try again\n");
			}
			else{
				trace_printf("Xbee configured okay \n");
				set_up_okay = 1;
			}
		}

		/*after configuring module, want to periodically output transmission

		/**
		 * The Skywire task will periodically communicate with the MCC.
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


		//convert integer speed value to string and pad if needed
		if (set_up_okay == 1){
			trace_printf("Constructing string \n");

			SLUpdate slUpdate;
			if (xQueueReceive(xSLUpdatesQueue, &slUpdate, 0) == pdTRUE) {
				trace_printf("beacon task: SL = %d\n", slUpdate.limit);
				speed = (int)slUpdate.limit;
			}

			//check that speed limit value is within limits.  If not, use previous
			if (speed >= l_limit && speed <= u_limit) {
				snprintf(speed_string, 4, "%d", speed);
				trace_printf("%s \n", speed_string);
				prev_speed = speed;
			}
			else {
				snprintf(speed_string, 4, "%d", prev_speed);
				trace_printf("%s \n", speed_string);
			}

			speed_cat[0] = '\0';
			send_string[0] = '\0';
			if (strlen(speed_string) == 1){
				trace_printf("Copying string \n");
				strcpy(speed_cat, zero_two);
				trace_printf("%s \n", speed_cat);
				trace_printf("Concatenating \n");
				strcat(speed_cat, speed_string);
			}
			else if (strlen(speed_string) == 2){
				strcpy(speed_cat, zero_one);
				strcat(speed_cat, speed_string);
			}
			else if (strlen(speed_string) == 3){
				strcpy(speed_cat, speed_string);
			}

			trace_printf("concatenated speed value \n");
			trace_printf("%s \n", speed_cat);
			//construct phrase to be sent, '$' sign on end to avoid passing along junk
			strcat(send_string, text_start);
			strcat(send_string, speed_cat);
			strcat(send_string, text_end);
			strcat(send_string, dollar);
			trace_printf("Sending phrase: %s", send_string);
			trace_printf("\n");

			//send string and do check to see if there was error
			if (xbee_write((uint8_t*)send_string, 0, 14) != XBEE_OK){
				//if error, increment count
				trace_printf("Error on writing to Xbee \n");
				error_count++;
			}
			//otherwise, reset error count
			else {
				error_count = 0;
			}

			//if error count exceeds threshold, set back to 0 and flag module to be re-configured
			if (error_count >= threshold) {
				trace_printf("Resetting module configuration \n");
				error_count = 0;
				set_up_okay = 0;
			}
			//incrementing speed value for test
			//speed++;
			/* Run task at ~1Hz for now. */
		}
		vTaskDelay(10000);
	}
}
