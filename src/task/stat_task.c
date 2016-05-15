#include <task/stat_task.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include "diag/Trace.h"


/**
 * Task to periodically print out percentage usage of running tasks
 */
void
stat_task(void * pvParameters) {
	//need to create buffer to hold returned ASCII data regarding task run percentages
	//size of buffer is approx. 40 bytes per task.  Assuming five tasks, can increase later if needed
	char stat_buffer[200];
	trace_printf("Setting stat buffer to null \n");

	//run loop to initially set all of buffer to null
	for (int i = 0; i <= 199; i++)
	{
		stat_buffer[i] = '\0';
	}


	//once task has started, run continuously
	for(;;){
		//periodically, call function in order to copy current run time stats over to buffer
		vTaskGetRunTimeStats(stat_buffer);

		//print out buffer.  Not sure if need to format or how data is presented in buffer by default
		trace_printf("run_time stats: \n");
		trace_printf("%s", stat_buffer);
		trace_printf("\n");

		vTaskDelay(10000);
	}
}
