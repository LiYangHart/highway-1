/**
 * Simple virtual COM port API for ST-LINK on UART2.
 *
 * Author: Mark Lieberman
 */

#ifndef _VIRTUAL_COM_H_
#define _VIRTUAL_COM_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>
#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

UART_HandleTypeDef* vcp_handle();
void vcp_init();
uint8_t vcp_count();
uint8_t vcp_getc();
uint8_t vcp_read(uint8_t* buffer, uint8_t position, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif /* _VIRTUAL_COM_H_ */

