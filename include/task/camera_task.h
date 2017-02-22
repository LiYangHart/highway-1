/**
 * This task operates the camera and environmental sensor package.
 *
 * Author: Mark Lieberman
 */

#ifndef _CAMERA_TASK_H_
#define _CAMERA_TASK_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAMERA_TASK_NAME "CAMR"
#define CAMERA_TASK_STACK_SIZE 2048

#define SAMPLE_BUFFER_SIZE 64

#define CAMERA_SAMPLE_INTERVAL 5000
#define CAMERA_PHOTO_INTERVAL 60000

extern QueueHandle_t xCameraQueue;

typedef struct _Sample {
	TickType_t TickCount;
	float LPS331Temperature;
	float LPS331Pressure;
	float HTS221Temperature;
	float HTS221Humidity;
} Sample;

typedef struct _SampleBuffer {
	Sample Buffer[SAMPLE_BUFFER_SIZE];
	uint8_t Count;
} SampleBuffer;

#define MIN(a,b) ((a)<(b)?(a):(b))

#define SAMPLE_RATE_MS 5000
#define IMAGE_RATE_MS 30000

#define BURST_READ_LENGTH 128

uint8_t camera_task_create();
void camera_tell(uint16_t message, uint32_t param1);

#ifdef __cplusplus
}
#endif

#endif /* _CAMERA_TASK_H_ */

