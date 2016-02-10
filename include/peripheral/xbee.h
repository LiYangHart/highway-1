/**
 * Simple API for XBee radio on UART6.
 *
 * Author: Mark Lieberman
 */

#ifndef _XBEE_H_
#define _XBEE_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>
#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

UART_HandleTypeDef* xbee_handle();
void xbee_init();
uint8_t xbee_count();
uint8_t xbee_getc();
uint8_t xbee_read(uint8_t* buffer, uint8_t position, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif /* _XBEE_H_ */

