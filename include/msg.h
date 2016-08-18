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


/* Power task messages */
#define MSG_POWER_SETUP                0x30

/* Camera task messages */
#define MSG_CAMERA_SETUP               0x10
#define MSG_CAMERA_MOUNT_SD            0x11
#define MSG_CAMERA_CLEAN_SD            0x12
#define MSG_CAMERA_SAMPLE              0x13
#define MSG_CAMERA_PHOTO               0x14
#define MSG_CAMERA_WRITE_SAMPLES       0x15

/**
 * Skywire task messages -------------------------------------------------------
 */
#define MSG_SKYWIRE_BASE               0x20
#define MSG_SKYWIRE_STATE_CHANGE       (MSG_SKYWIRE_BASE + 1)

#define MSG_SKYWIRE_SETUP              (MSG_SKYWIRE_BASE + 2)
#define MSG_SKYWIRE_ACTIVATE           (MSG_SKYWIRE_BASE + 3)
#define MSG_SKYWIRE_ACTIVATED          (MSG_SKYWIRE_BASE + 4)
#define MSG_SKYWIRE_CONFIG             (MSG_SKYWIRE_BASE + 5)
#define MSG_SKYWIRE_PDP_ENABLE         (MSG_SKYWIRE_BASE + 6)
#define MSG_SKYWIRE_XMIT_START         (MSG_SKYWIRE_BASE + 7)
#define MSG_SKYWIRE_XMIT_BUFFER        (MSG_SKYWIRE_BASE + 8)
#define MSG_SKYWIRE_XMIT_DONE          (MSG_SKYWIRE_BASE + 9)

/**
 * Wake up the Skywire modem and prepare to transmit.
 */
#define MSG_SKYWIRE_WAKE               (MSG_SKYWIRE_BASE + 10)

/**
 * Power off the modem to enter a low power state.
 */
#define MSG_SKYWIRE_SLEEP              (MSG_SKYWIRE_BASE + 11)

/**
 * Send this message to generate an escape sequence. If param1 is non-zero, a
 * message with that value will be queued after the sequence has been emitted.
 */
#define MSG_SKYWIRE_ESCAPE             (MSG_SKYWIRE_BASE + 17)
#define MSG_SKYWIRE_ESCAPE_1           (MSG_SKYWIRE_BASE + 18)

/**
 * Assign a file to transmit. Message param1 shall be the address of a
 * UploadQueueItem struct with information about the file to transmit.
 */
#define MSG_SKYWIRE_REQUEST            (MSG_SKYWIRE_BASE + 12)

/**
 * States in the HTTP request process.
 */
#define MSG_SKYWIRE_REQUEST_OPEN       (MSG_SKYWIRE_BASE + 13)
#define MSG_SKYWIRE_REQUEST_WRITE      (MSG_SKYWIRE_BASE + 14)
#define MSG_SKYWIRE_RESPONSE_READ      (MSG_SKYWIRE_BASE + 15)
#define MSG_SKYWIRE_REQUEST_CLOSE      (MSG_SKYWIRE_BASE + 16)

/**
 * Upload task messages --------------------------------------------------------
 */
#define MSG_UPLOAD_BASE                0x30
#define MSG_UPLOAD_SETUP               (MSG_UPLOAD_BASE + 1)

/**
 * Start a scan of the /upload/ directory on the SD card.
 */
#define MSG_UPLOAD_SCAN                (MSG_UPLOAD_BASE + 2)

/**
 * Delete sent items in the upload queue from the SD card.
 */
#define MSG_UPLOAD_DELETE              (MSG_UPLOAD_BASE + 3)

/**
 * Message requesting the next file to transmit.
 */
#define MSG_UPLOAD_NEXT                (MSG_UPLOAD_BASE + 4)

/**
 * Message telling the upload task that a file was sent.
 */
#define MSG_UPLOAD_DONE                (MSG_UPLOAD_BASE + 5)

#ifdef __cplusplus
}
#endif

#endif /* _MSG_H_ */
