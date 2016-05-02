/**
 * Watchdog task
 *
 * Author: Mark Lieberman
 */

#ifndef _WATCHDOG_TASK_H_
#define _WATCHDOG_TASK_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>
#include <stm32f4xx_hal_iwdg.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WATCHDOG_TASK_NAME "IWDG"
#define WATCHDOG_TASK_STACK_SIZE 1024

extern QueueHandle_t xWatchdogQueue;

/**
 * Watchdog period in 8ms ticks
 * 8ms * 0x3E8 = 8 seconds
 */
#define IWDG_PERIOD 0xFFF

#define IWDG_SKYWIRE_MASK 0x01
#define IWDG_CAMERA_MASK  0x02

#define IWDG_ALL_TASKS_OK 0x03

uint8_t watchdog_task_create();

#ifdef __cplusplus
}
#endif

#endif /* _WATCHDOG_TASK_H_ */

