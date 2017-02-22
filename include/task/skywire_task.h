/**
 * This task provides power management and socket data services for the Skywire
 * modem. See http_client.h for an HTTP client implementation built on top of
 * this task.
 *
 * Author: Mark Lieberman
 */

#ifndef _SKYWIRE_TASK_H_
#define _SKYWIRE_TASK_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constants ---------------------------------------------------------------- */

/* Task name and stack size. */
#define SKYWIRE_TASK_NAME              "HTTP"
#define SKYWIRE_TASK_STACK_SIZE        1024

/* Size of buffer used to read and write commands to and from the Skywire. */
#define SKYWIRE_BUFFER_LENGTH          256

/* Skywire task states */
#define SKYWIRE_STATE_POWER_OFF        0
#define SKYWIRE_STATE_POWER_ON         1
#define SKYWIRE_STATE_POWERING_ON      2
#define SKYWIRE_STATE_POWERING_OFF     3
#define SKYWIRE_STATE_CONNECTED        4

/* Variables ---------------------------------------------------------------- */

extern QueueHandle_t xSkywireQueue;

/* Functions ---------------------------------------------------------------- */

void skywire_tell(uint16_t message, uint32_t param1);
void skywire_tell_delay(uint16_t message, uint32_t param1, TickType_t delay);

uint8_t skywire_task_create();
void skywire_reactivate();

uint8_t skywire_connected();
uint8_t skywire_socket_open(char* address);

#ifdef __cplusplus
}
#endif

#endif /* _SKYWIRE_TASK_H_ */

