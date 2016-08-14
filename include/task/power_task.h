/**
 * Camera task
 *
 * Author: Mark Lieberman
 */

#ifndef _POWER_TASK_H_
#define _POWER_TASK_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define POWER_TASK_NAME "POWR"
#define POWER_TASK_STACK_SIZE 1024

extern QueueHandle_t xPowerQueue;

uint8_t power_task_create();

#ifdef __cplusplus
}
#endif

#endif /* _POWER_TASK_H_ */

