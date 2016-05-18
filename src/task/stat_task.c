#include <peripheral/i2c_spi_bus.h>
#include <task/stat_task.h>
#include <task/camera_task.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include "diag/Trace.h"
#include <stm32f4xx.h>
#include <fat_sl.h>
#include <mdriver_spi_sd.h>
#include <FreeRTOS.h>

QueueHandle_t xMountQueue;
/**
 * Open a handle to STAT.LOG.
 */
F_FILE*
open_stat_log() {
	/* Open a handle to append to the data log. */
	F_FILE* hLog = f_open("stat.log", "a");
	if (hLog == NULL) {
		trace_printf("stats_task; failed to open stat.log\n");
	}
	return hLog;
}


/**
 * function to write out task stats to SD card
 */
uint8_t
stat_log_write(char *stat_buffer) {
	int length = 0;
	/* Try to obtain access to the SD card. */
	trace_printf("Attempting to write stats to SD card\n");

	F_FILE* pxStat = NULL;
	if (!spi_take()) {
		/* SPI is busy - nothing more to do. */
		return 0;
	}

	/* Open a handle to the data log. */
	if ((pxStat = open_stat_log()) == NULL) {
		goto error;
	}

	//get length of buffer to write
	length = strlen(stat_buffer);

	//attempt to write to log file
	if (f_write(stat_buffer, 1, length, pxStat) != length) {
			trace_printf("stats_task: write of stats to log failed\n");
			goto error;
	}
	//at end of function, release spi and close file before returning
	f_close(pxStat);
	pxStat = NULL;
	spi_give();
	return 1; // OK

	/* Fall through and clean up. */
	error:
		if (pxStat != NULL) { f_close(pxStat); };
		spi_give();
		return 0;
}

/**
 * Task to periodically print out percentage usage of running tasks
 */
void stat_task(void * pvParameters) {
	//need to create buffer to hold returned ASCII data regarding task run percentages
	//size of buffer is approx. 40 bytes per task.  Assuming five tasks, can increase later if needed
	//overshooting a bit as sometimes freeRTOS tasks/functions get included as well
	char stat_buffer[280];
	int card_mounted = 0;
	trace_printf("Setting stat buffer to null \n");

	//run loop to initially set all of buffer to null
	for (int i = 0; i <= 279; i++)
	{
		stat_buffer[i] = '\0';
	}

	/* Create an IPC queue to receive check if SD card has been mounted. */
	xMountQueue = xQueueCreate( 1, sizeof(Cardmount));
	//starting delay to allow camera task to mount SD card
	vTaskDelay(10000);

	//once task has started, run continuously
	for(;;){
		if (card_mounted == 0)
		{
			trace_printf("Checking if SD card mounted\n");
			Cardmount check;
			if (xQueueReceive(xMountQueue, &check, 0) == pdTRUE) {
				trace_printf("Mount check = %d\n", check.mounted);
				card_mounted= (int)check.mounted;
			}
		}
		else
		{
			//periodically, call function in order to copy current run time stats over to buffer
			vTaskGetRunTimeStats(stat_buffer);

			//print out buffer.  Not sure if need to format or how data is presented in buffer by default
			trace_printf("run_time stats: \n");
			trace_printf("%s", stat_buffer);
			trace_printf("\n");

			//instead of writing out to screen, want to try and write to SD card log instead to make life easier
			//call function and pass in statbuffer
			if ((stat_log_write(stat_buffer)) != 1)
			{
				trace_printf("Error writing stats to file\n");
			}
			else
			{
				trace_printf("Stats written okay\n");
			}

			//after printing out buffer, zero again before next go around
			for (int i = 0; i <= 279; i++)
			{
				stat_buffer[i] = '\0';
			}
		}
		vTaskDelay(40000);
	}
}
