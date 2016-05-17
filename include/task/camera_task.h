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

typedef struct Cardmount {
	uint8_t mounted;
} Cardmount;

#define MIN(a,b) ((a)<(b)?(a):(b))

#define CAMERA_TASK_NAME "CAMR"
#define CAMERA_TASK_STACK_SIZE 2048

#define SAMPLE_RATE_MS 5000
#define IMAGE_RATE_MS 30000

#define BURST_READ_LENGTH 128

/* Program will delete DATA.LOG and JPG files at boot if defined. */
#define CLEAN_SD_CARD

/*program will format SD card if defined*/
//#define FORMAT_SD_CARD

void camera_task(void * pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* _CAMERA_TASK_H_ */

