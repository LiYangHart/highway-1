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

#define SKYWIRE_STATE_POWER_OFF 0
#define SKYWIRE_STATE_POWER_ON 1
#define SKYWIRE_STATE_POWERING_ON 2
#define SKYWIRE_STATE_POWERING_OFF 3
#define SKYWIRE_STATE_CONNECTED 4

extern QueueHandle_t xSkywireQueue;

#define SKYWIRE_TASK_NAME "SKYW"
#define SKYWIRE_TASK_STACK_SIZE 2586

typedef struct _SkywireRequest {
	uint16_t iHandle;
	uint8_t bFileUpload;
	char sMethod[16];
	char sHost[64];
	char sPath[64];
	const char * sContentType;
	char * sBody;
} SkywireRequest;

extern const char * CONTENT_TYPE_APPLICATION_JSON;
extern const char * CONTENT_TYPE_OCTET_STREAM;

/* Configurable options */
#define NGROK_TUNNEL "4b9cc537.ngrok.io"

uint8_t skywire_task_create();

#ifdef __cplusplus
}
#endif

#endif /* _SKYWIRE_TASK_H_ */

