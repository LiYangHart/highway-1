/**
 * Simple AT-command API for Skywire modem on UART1.
 *
 * Author: Mark Lieberman
 */

#ifndef _SKYWIRE_H_
#define _SKYWIRE_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>
#include "FreeRTOS.h"

#define SKYWIRE_DMA_BUFFER_LENGTH 512

typedef enum {
	SKYWIRE_OK      = HAL_OK,
	SKYWIRE_ERROR   = HAL_ERROR,
	SKYWIRE_TIMEOUT = HAL_TIMEOUT
} Skywire_StatusTypeDef;

#ifdef __cplusplus
extern "C" {
#endif

UART_HandleTypeDef* skywire_handle();
void skywire_en(GPIO_PinState pinState);
void skywire_rts(GPIO_PinState pinState);
void skywire_init();
uint16_t skywire_count();
uint8_t skywire_getc();
uint16_t skywire_read(uint8_t* buffer, uint16_t position, uint16_t length);

Skywire_StatusTypeDef skywire_write(uint8_t* buffer, uint16_t start, uint16_t length);
Skywire_StatusTypeDef skywire_at(char* buffer);

#ifdef __cplusplus
}
#endif

#endif /* _SKYWIRE_H_ */

