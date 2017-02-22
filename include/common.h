/**
 * Common include file for this project.
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <timers.h>
#include <task.h>
#include <fat_sl.h>

#include <stdio.h>
#include <string.h>

#include "diag/Trace.h"

#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ERROR_ON_NULL(retVal) if ((retVal) == NULL)   { goto error; }
#define ERROR_ON_FAIL(retVal) if ((retVal) == pdFAIL) { goto error; }

#ifdef __cplusplus
}
#endif

#endif /* _COMMON_H_ */

