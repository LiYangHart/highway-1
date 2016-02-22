/**
 * Camera task
 *
 * Author: Mark Lieberman
 */

#ifndef _CAMERA_TASK_H_
#define _CAMERA_TASK_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAMERA_TASK_NAME "CAMR"
#define CAMERA_TASK_STACK_SIZE 2048

void camera_task(void * pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* _CAMERA_TASK_H_ */

