/**
 * Messages for IPC
 */

#ifndef _MSG_H_
#define _MSG_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Msg {
	uint16_t message;
	uint32_t param1;
} Msg;

/* Watchdog messages */
#define MSG_IWDG_REFRESH               0x01
#define MSG_IWDG_PING                  0x02
#define MSG_IWDG_PONG                  0x03

/* Camera task messages */
#define MSG_CAMERA_SETUP               0x10
#define MSG_CAMERA_MOUNT               0x11
#define MSG_CAMERA_SAMPLE              0x12
#define MSG_CAMERA_PHOTO               0x13

/* Skywire task messages */
#define MSG_SKYWIRE_SETUP              0x20
#define MSG_SKYWIRE_ACTIVATE           0x21
#define MSG_SKYWIRE_CONFIG             0x22
#define MSG_SKYWIRE_PDP_ENABLE         0x23
#define MSG_SKYWIRE_XMIT               0x24

#ifdef __cplusplus
}
#endif

#endif /* _MSG_H_ */
