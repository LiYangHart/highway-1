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
#include "timers.h"

#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAMERA_TASK_NAME "CAMR"
#define CAMERA_TASK_STACK_SIZE 2048

extern QueueHandle_t xCameraQueue;

typedef struct _Sample {
	TickType_t TickCount;
	float LPS331Temperature;
	float LPS331Pressure;
	float HTS221Temperature;
	float HTS221Humidity;
} Sample;

typedef struct _SampleBuffer {
	Sample Buffer[64];
	uint8_t Count;
} SampleBuffer;

#define MIN(a,b) ((a)<(b)?(a):(b))

#define SAMPLE_RATE_MS 5000
#define IMAGE_RATE_MS 30000

#define BURST_READ_LENGTH 128

/* Program will delete DATA.LOG and JPG files at boot if defined. */
#define CLEAN_SD_CARD

uint8_t camera_task_create();

#ifdef __cplusplus
}
#endif

#endif /* _CAMERA_TASK_H_ */

