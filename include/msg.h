/**
 * Messages for IPC
 */

#ifndef _MSG_H_
#define _MSG_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Msg {
	uint16_t message;
	uint32_t param1;
} Msg;

#define MSG_TIMER_ID(msg, param) (void *)((msg & 0xFFF) | ((param & 0xF) << 24))
#define MSG_TIMER_MESSAGE(timerId) (void *)(timerId & 0xFFF)
#define MSG_TIMER_PARAM(timerId) (void *)((timerId & 0xF000) >> 24)

#define MSG_SYSTEM_BASE                0x100

/**
 * Message sent when the SD card is mounted.
 */
#define MSG_SDCARD_MOUNTED             (MSG_SYSTEM_BASE + 1)

/**
 * Watchdog messages -----------------------------------------------------------
 */
#define MSG_IWDG_BASE                  0x1
#define MSG_IWDG_REFRESH               (MSG_IWDG_BASE + 1)
#define MSG_IWDG_PING                  (MSG_IWDG_BASE + 2)
#define MSG_IWDG_PONG                  (MSG_IWDG_BASE + 3)

/**
 * Power task messages ---------------------------------------------------------
 */
#define MSG_POWER_BASE                 0x200
#define MSG_POWER_SETUP                (MSG_POWER_BASE + 1)

/**
 * Upload task messages --------------------------------------------------------
 */
#define MSG_UPLOAD_BASE                0x300
#define MSG_UPLOAD_SETUP               (MSG_UPLOAD_BASE + 1)
#define MSG_UPLOAD_SDCARD_MOUNT        (MSG_UPLOAD_BASE + 2)

/**
 * Start a scan of the /upload/ directory on the SD card.
 */
#define MSG_UPLOAD_SCAN                (MSG_UPLOAD_BASE + 4)

/**
 * Delete sent items in the upload queue from the SD card.
 */
#define MSG_UPLOAD_DELETE              (MSG_UPLOAD_BASE + 5)

/**
 * Message requesting the next file to transmit.
 */
#define MSG_UPLOAD_NEXT                (MSG_UPLOAD_BASE + 6)

/**
 * Message telling the upload task that a file was sent.
 */
#define MSG_UPLOAD_DONE                (MSG_UPLOAD_BASE + 7)


/**
 * Camera task messages --------------------------------------------------------
 */
#define MSG_CAMERA_BASE                0x400
#define MSG_CAMERA_SETUP               (MSG_CAMERA_BASE + 1)
#define MSG_CAMERA_MOUNT_SD            (MSG_CAMERA_BASE + 2)
#define MSG_CAMERA_CLEAN_SD            (MSG_CAMERA_BASE + 3)
#define MSG_CAMERA_SAMPLE              (MSG_CAMERA_BASE + 4)
#define MSG_CAMERA_PHOTO               (MSG_CAMERA_BASE + 5)
#define MSG_CAMERA_WRITE_SAMPLES       (MSG_CAMERA_BASE + 6)

/**
 * Skywire task messages -------------------------------------------------------
 */
#define MSG_SKYWIRE_BASE               0x500
#define MSG_SKYWIRE_SETUP              (MSG_SKYWIRE_BASE + 1)

/**
 * Trigger an activation sequence.
 */
#define MSG_SKYWIRE_ACTIVATE           (MSG_SKYWIRE_BASE + 2)
#define MSG_SKYWIRE_DEACTIVATE         (MSG_SKYWIRE_BASE + 3)
#define MSG_SKYWIRE_CONFIG             (MSG_SKYWIRE_BASE + 4)
#define MSG_SKYWIRE_PDP_ENABLE         (MSG_SKYWIRE_BASE + 5)

/**
 * Emit an escape sequence: 1s idle -> +++ -> 1s idle.
 * Parameter 1 is the address of a callback function to invoke when the escape
 * sequence is finished.
 */
#define MSG_SKYWIRE_ESCAPE             (MSG_SKYWIRE_BASE + 6)
#define MSG_SKYWIRE_RESERVED           (MSG_SKYWIRE_BASE + 7)

/**
 * Change the Skywire power state.
 */
#define MSG_SKYWIRE_WAKE               (MSG_SKYWIRE_BASE + 8)
#define MSG_SKYWIRE_SLEEP              (MSG_SKYWIRE_BASE + 9)

/**
 * Message sent when the state changes. The low bytes of the parameter are the
 * new state, and the high bytes are the old state.
 */
#define MSG_SKYWIRE_STATE_CHANGE       (MSG_SKYWIRE_BASE + 40)

/**
 * Skywire/HTTP client messages.
 */
#define MSG_HTTP_REQUEST_OPEN          (MSG_SKYWIRE_BASE + 50)
#define MSG_HTTP_REQUEST_WRITE         (MSG_SKYWIRE_BASE + 51)
#define MSG_HTTP_RESPONSE_READ_STATUS  (MSG_SKYWIRE_BASE + 52)
#define MSG_HTTP_RESPONSE_READ_HEADER  (MSG_SKYWIRE_BASE + 53)
#define MSG_HTTP_RESPONSE_READ_BODY    (MSG_SKYWIRE_BASE + 54)
#define MSG_HTTP_REQUEST_OVER          (MSG_SKYWIRE_BASE + 55)

#ifdef __cplusplus
}
#endif

#endif /* _MSG_H_ */
