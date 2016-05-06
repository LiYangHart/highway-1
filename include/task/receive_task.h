/**
 * Beacon task
 *
 * Author: Mark Lieberman, Matthew Mayhew
 */

#ifndef _RECEIVE_TASK_H_
#define _RECEIVE_TASK_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

extern QueueHandle_t xSLUpdatesQueue;

#define RECEIVE_TASK_NAME "REAC"
#define RECEIVE_TASK_STACK_SIZE 1024

void receive_task(void * pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* _RECEIVE_TASK_H_ */
