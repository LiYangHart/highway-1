/**
 * Read/write API for DMA ring buffer serial communication.
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
	uint8_t rx_length;
	uint8_t const * rx_tail_ptr;
} DMA_SerialHandle;

/**
 * Get the number of characters available in the serial buffer.
 */
uint8_t dma_serial_has(DMA_SerialHandle* handle);

/**
 * Get the next character in the serial buffer.
 */
uint8_t dma_serial_getc(DMA_SerialHandle* handle);

#ifdef __cplusplus
}
#endif

#endif /* _DMA_SERIAL_H_ */

