/**
 * Media driver implementation for FreeRTOS FAT SL using SPI SD driver for SDHC and SDXC cards.
 *
 * A basic SPI SD driver implementation using HAL SPI device with polled IO.
 *
 * TODO Clock speed increase after card initialization.
 * TODO Implement read and write sector using DMA or interrupt.
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

/**
 * Get a reference to the HAL UART instance.
 */
UART_HandleTypeDef* vcp_handle();

/**
 * Initialize the UART to communicate with the virtual COM port.
 */
void vcp_init();

/**
 * Get the number of characters available in the serial buffer.
 */
uint8_t vcp_available();

/**
 * Get the next character in the serial buffer.
 */
uint8_t vcp_getchar();

#ifdef __cplusplus
}
#endif

#endif /* _VIRTUAL_COM_H_ */

