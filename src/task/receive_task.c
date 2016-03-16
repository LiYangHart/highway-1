#include <peripheral/xbee.h>
#include <peripheral/lcd.h>
#include <task/receive_task.h>
#include <stdio.h>
#include <string.h>
#include <hayes.h>
#include "diag/Trace.h"

/**
 * Initialize the peripherals and state for this task.
 */
uint8_t
receive_task_setup() {

	/* Initialize the UART to communicate with the XBee radio. */
	xbee_init();

	return 1; // OK
}

uint8_t
lcd_task_setup() {
		/*initialize LCD to display values coming in from Xbee. */
	lcd_init();
	return 1; // OK
}

//function to do configuration of Xbee module acting as receiver
uint8_t
xbee_receive_setup(ATDevice* xbee_receive) {
	//with Xbee setup, want to first enter command mode to set proper fields for Xbee operation
	//done by sending three +++ quickly to Xbee, then waiting for one second
	if (hayes_at(xbee_receive, "+++")								!= HAYES_OK
		|| hayes_res(xbee_receive, pred_ends_with, "OK\r", 1050)	!= HAYES_OK){
			trace_printf("Failed to enter command mode \n");
			return 0;
	}
	else {
		trace_printf("Entered command mode\n");
	}

	//once in command mode, begin setting parameters, look for OK after each one
	//note that current settings are for transmitter, carriage return '\r' needed on end of command strings
	//setting parameters first, will read back certain critical ones after

	if (hayes_at(xbee_receive, "ATPL0\r")							!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_receive, "ATCE2\r")						!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_receive, "ATNH1\r")						!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_receive, "ATDH00000000\r")					!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_receive, "ATDL0000FFFF\r")					!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_receive, "ATTO41\r")						!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_receive, "ATD00\r")						!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_receive, "ATD50\r")						!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_receive, "ATD70\r")						!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_receive, "ATD80\r")						!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_receive, "ATPD7FFF\r")						!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1200)		!= HAYES_OK
		||hayes_at(xbee_receive, "ATAV1\r")						!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1200)		!= HAYES_OK) {
			trace_printf("Sending configuration commands failed \n");
			return 0;
	}

	else{
		trace_printf("Configuration commands run \n");
	}

	//after sending commands out, want to read back a few values
	//if not as expected, send command to correct

	//first, checking preamble and network IDs
	if (hayes_at(xbee_receive, "ATHP\r")							!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "0\r", 1000)		!= HAYES_OK) {
			trace_printf("Preamble not set correctly, changing \n");
			if (hayes_at(xbee_receive, "ATHP0\r")							!= HAYES_OK
				||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1000)		!= HAYES_OK) {
				trace_printf("Error setting preamble \n");
				return 0;
			}
	}

	else{
		trace_printf("Preamble okay \n");
	}

	if (hayes_at(xbee_receive, "ATID\r")								!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "7FFF\r", 1000)		!= HAYES_OK) {
			trace_printf("Network ID not set correctly, changing \n");
			if (hayes_at(xbee_receive, "ATID7FFF\r")						!= HAYES_OK
				||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1000)		!= HAYES_OK) {
					trace_printf("Error setting network ID \n");
					return 0;
			}
	}

	else{
		trace_printf("Network ID okay \n");
	}

	//checking baud rate
	if (hayes_at(xbee_receive, "ATBD\r")								!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "3\r", 1000)			!= HAYES_OK) {
			trace_printf("Baud rate not set correctly, changing \n");
			if (hayes_at(xbee_receive, "ATBD3\r")							!= HAYES_OK
				||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1000)		!= HAYES_OK) {
					trace_printf("Error setting baud rate \n");
					return 0;
			}
	}

	else{
		trace_printf("Baud rate okay \n");
	}

	//double check power and destination address fields
	if (hayes_at(xbee_receive, "ATPL\r")										!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "0\r", 1200)				!= HAYES_OK ){
		trace_printf("Error in checking power level \n");
		return 0;
	}
	else {
		trace_printf("Power level okay \n");
	}

	if (hayes_at(xbee_receive, "ATDH\r")									!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "0\r", 1200)		!= HAYES_OK){
		trace_printf("Error with destination high \n");
		return 0;
	}
	else {
		trace_printf("Destination high okay \n");
	}


	if (hayes_at(xbee_receive, "ATDL\r")									!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "FFFF\r", 1200)		!= HAYES_OK) {
			trace_printf("Error with destination low \n");
			return 0;
	}

	else {
		trace_printf("Parameter check okay \n");
	}

	//after configuring/checking everything, apply changes and exit command mode
	if (hayes_at(xbee_receive, "ATAC\r")							!= HAYES_OK
		||hayes_res(xbee_receive, pred_ends_with, "OK\r", 1000)		!= HAYES_OK
		||hayes_at(xbee_receive, "ATCN\r")							!= HAYES_OK) {
			trace_printf("Error applying changes \n");
			return 0;
	}

	//if this point reached, okay on set-up
	trace_printf("Xbee set-up complete \n");
	return 1;
}

void
receive_task(void * pvParameters) {
	//value used to hold value of set-up, start at '0' then set to '1' if done correctly
	int set_up_okay = 0;
	char buffer[512];
	int error = 0;
	int threshold = 10;

	char ack = '\x06';
	char lcd_returned = '\x00';
	char rec;
	char received[15];
	char lcd_print[6];
	int i = 0;

	ATDevice xbee_receive;
	xbee_receive.api.count = xbee_count;
	xbee_receive.api.getc = xbee_getc;
	xbee_receive.api.write = xbee_write;
	xbee_receive.buffer = buffer;
	xbee_receive.length = 512;

	/* Initialize the peripherals and state for this task. */
	if (!receive_task_setup()) {
		trace_printf("receiver_task: setup failed\n");
		vTaskDelete(NULL);
		return;
	} else {
		trace_printf("receiver_task: started\n");
	}

	//initialize display LCD
	if (!lcd_task_setup()) {
		trace_printf("LCD startup failed\n");
		vTaskDelete(NULL);
		return;
	}else{
		trace_printf("LCD started\n");
	}

	//clearing splash screen off of LCD.  Doing this first as it sets some properties back to default
	lcd_write((uint8_t*)"\xFF\xD7", 2);
	vTaskDelay(200);
	trace_printf("LCD cleared\n");
	while (lcd_count() > 0){
		lcd_returned = lcd_getc();
		if (lcd_returned == ack){
			trace_printf("Acknowledged\n");
		}
	}

	//after LCD uart has been cleared, send commands to set text properties and print OK on screen
	//set text height and width to 2x that of default
	lcd_write((uint8_t*)"\xFF\x7C\x00\x04", 4);
	vTaskDelay(200);
	trace_printf("Setting text height\n");
	while (lcd_count() > 0){
		lcd_returned = lcd_getc();
		if (lcd_returned == ack){
			trace_printf("Acknowledged\n");
		}
	}

	lcd_write((uint8_t*)"\xFF\x7B\x00\x04", 4);
	vTaskDelay(200);
	trace_printf("Setting text width\n");
	while (lcd_count() > 0){
		lcd_returned = lcd_getc();
		if (lcd_returned == ack){
			trace_printf("Acknowledged\n");
		}
	}

	//set text colour to red
	lcd_write((uint8_t*)"\xFF\x7F\xF8\x00", 4);
	vTaskDelay(200);
	trace_printf("Setting text colour\n");
	while (lcd_count() > 0){
		lcd_returned = lcd_getc();
		if (lcd_returned == ack){
			trace_printf("Acknowledged\n");
		}
	}

	//set cursor position and print text
	lcd_write((uint8_t*)"\xFF\xE4\x00\x01\x00\x01", 6);
	vTaskDelay(200);
	trace_printf("Setting Cursor Position\n");
	while (lcd_count() > 0){
		lcd_returned = lcd_getc();
		if (lcd_returned == ack){
			trace_printf("Acknowledged\n");
		}
	}

	lcd_write((uint8_t*)"\x00\x06\x4F\x4B\x00", 5);
	vTaskDelay(200);
	trace_printf("Printing OK\n");
	while (lcd_count() > 0){
		lcd_returned = lcd_getc();
		if (lcd_returned == ack){
			trace_printf("Acknowledged\n");
		}
	}

	//once task has started, run continuously
	for(;;){
		//configuration for module acting as transmitter

		//first, check to see if configuration has been done properly
		if (set_up_okay == 0){
			//call function to configure transmitting xbee module as well as re-configuring serial just in case
			//if ending back here because not writing to module, figured it was better to reset both
			xbee_init();

			if(!xbee_receive_setup(&xbee_receive)){
				trace_printf("Xbee configuration failed, try again\n");
				//extra delay in place here to make sure command mode times out before trying again
				vTaskDelay(8000);
			}
			else{
				trace_printf("Xbee configured okay \n");
				set_up_okay = 1;
			}
		}

		//after configuring module, want to periodically check for received transmissions


		//convert integer speed value to string and pad if needed
		if (set_up_okay == 1){
			i = 0;
			trace_printf("Setting null\n");
			for (int j = 0; j <= 14; j++){
				received[j] = '\0';
			}

			//if data has come in, print out and copy into received
			if (xbee_count() > 0){
				trace_printf("Received Message: \n");
				while (xbee_count() > 0) {
					rec = xbee_getc();
					trace_printf("%c", rec);
					received[i] = rec;
					i++;
				}
				trace_printf("\n");
				//trace_printf("%s \n", received);
				trace_printf("received_length = %d \n", strlen(received));
				//after receiving characters, check for needed length and termination character.  If match, pull out and print to LCD
				if (strlen(received) == 14 && received[13] == '$'){
					//resetting error variable and then printing speed out to LCD
					error = 0;
					trace_printf("String match\n");
					lcd_print[0] = '\x00';
					lcd_print[1] = '\x06';
					strncpy(&lcd_print[2], &received[4], 3);
					lcd_print[5] = '\0';

					//clear LCD screen and set text height, width and position
					lcd_write((uint8_t*)"\xFF\xD7", 2);
					vTaskDelay(200);

					lcd_write((uint8_t*)"\xFF\x7C\x00\x04", 4);
					vTaskDelay(200);

					lcd_write((uint8_t*)"\xFF\x7B\x00\x04", 4);
					vTaskDelay(200);

					lcd_write((uint8_t*)"\xFF\xE4\x00\x01\x00\x01", 6);
					vTaskDelay(200);

					//print string to LCD
					lcd_write((uint8_t*)lcd_print, 6);
					vTaskDelay(200);
					}

				else{
					trace_printf("Full packet not received \n");
					error++;
				}
			}
		}
		//if sequential bad received values exceed threshold, set for reconfiguration of xbee module
		if (error >= threshold){
			error = 0;
			set_up_okay = 0;
		}
		vTaskDelay(2000);
	}
}
