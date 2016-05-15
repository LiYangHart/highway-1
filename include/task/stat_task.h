/* Beacon task
 *
 * Author: Mark Lieberman, Matthew Mayhew
 */

#ifndef _STAT_TASK_H_
#define _STAT_TASK_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif


#define STAT_TASK_NAME "STAT"
#define STAT_TASK_STACK_SIZE 1024

void stat_task(void * pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* _BEACON_TASK_H_ */
