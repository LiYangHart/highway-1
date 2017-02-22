/**
 * Simple non-blocking API for DMA ring-buffer receive serial communication.
 *
 * Author: Mark Lieberman
 */

#ifndef _DMA_SERIAL_H_
#define _DMA_SERIAL_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>
#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	UART_HandleTypeDef huart;
	DMA_HandleTypeDef hdma;
	uint8_t* rx_buffer;
	uint16_t rx_length;
	uint8_t const * rx_tail_ptr;
} DMA_SerialHandle;

uint16_t dma_serial_count(DMA_SerialHandle* handle);
uint8_t dma_serial_getc(DMA_SerialHandle* handle);
uint16_t dma_serial_read(DMA_SerialHandle* handle, uint8_t* buffer, uint16_t position, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* _DMA_SERIAL_H_ */

