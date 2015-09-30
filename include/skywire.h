/**
 * Skywire cellular modem library.
 *
 * Author: Mark Lieberman
 */

#ifndef _SKYWIRE_H_
#define _SKYWIRE_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>
#include "FreeRTOS.h"

/* Buffer length for RX DMA ring buffer. */
#define RX_BUFFER_LEN 128

typedef enum {
	SKYWIRE_OK      = HAL_OK,
	SKYWIRE_ERROR   = HAL_ERROR,
	SKYWIRE_TIMEOUT = HAL_TIMEOUT
} Skywire_StatusTypeDef;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get a reference to the HAL UART instance.
 */
UART_HandleTypeDef* skywire_handle();

/**
 * Set the pin state of the Skywire ON_OFF pin.
 *
 * The pin is configured by HAL_UART_Init() in stm32f4xx_hal_msp.c.
 */
void skywire_en(GPIO_PinState pinState);

/**
 * Set the pin state of the Skywire RTS pin.
 *
 * The pin is configured by HAL_UART_Init() in stm32f4xx_hal_msp.c.
 */
void skywire_rts(GPIO_PinState pinState);

/**
 * Initialize the UART to communicate with the Skywire modem.
 */
void skywire_init();

/**
 * Perform the activation function for the Skywire modem.
 *
 * This requires a pulse of 1s < HOLD_TIME < 2s to be applied to ON_OFF.
 * The modem may not respond to commands for up to 15 seconds.
 */
void skywire_activate();

/**
 * Get the number of characters available in the Skywire buffer.
 */
uint8_t skywire_available();

/**
 * Get the next character in the Skywire buffer.
 */
uint8_t skywire_getchar();

#ifdef __cplusplus
}
#endif

#endif /* _SKYWIRE_H_ */

