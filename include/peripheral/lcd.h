/**
 * Simple API for LCD unit on UART1.
 *
 * Author: Matt Mayhew
 */

#ifndef _LCD_H_
#define _LCD_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>
#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

UART_HandleTypeDef* lcd_handle();
void lcd_init();
uint8_t lcd_count();
uint8_t lcd_getc();
uint8_t lcd_read(uint8_t* buffer, uint8_t position, uint8_t length);
uint8_t lcd_write(uint8_t* buffer, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif /* _LCD_H_ */
