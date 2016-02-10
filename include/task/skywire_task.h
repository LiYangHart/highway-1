/**
 * Skywire task
 *
 * Author: Mark Lieberman, Matthew Mayhew
 */

#ifndef _SKYWIRE_TASK_H_
#define _SKYWIRE_TASK_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SKYWIRE_TASK_NAME "SKYW"
#define SKYWIRE_TASK_STACK_SIZE 1024

void skywire_task(void * pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* _SKYWIRE_TASK_H_ */

