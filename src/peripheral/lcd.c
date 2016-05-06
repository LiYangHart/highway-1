#include <peripheral/dma_serial.h>
#include <peripheral/lcd.h>
#include "FreeRTOS.h"
#include "task.h"

uint8_t rx_buffer[128];
DMA_SerialHandle lcd = {
	{ 0 },
	{ 0 },
	rx_buffer,
	128,
	rx_buffer
};

/**
 * Get a reference to the HAL UART instance.
 */
UART_HandleTypeDef*
lcd_handle() {
	return &lcd.huart;
}

/**
 * Initialize the UART to communicate with the XBee radio.
 */
void
lcd_init() {
	/* Configure the UART for the LCD. */
	__HAL_RCC_USART1_CLK_ENABLE();
	lcd.huart.Instance = USART1;
	lcd.huart.State = HAL_UART_STATE_RESET;
	lcd.huart.Init.BaudRate = 9600;
	lcd.huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	lcd.huart.Init.Mode = UART_MODE_TX_RX;
	lcd.huart.Init.WordLength = UART_WORDLENGTH_8B;
	lcd.huart.Init.Parity = UART_PARITY_NONE;
	lcd.huart.Init.StopBits = UART_STOPBITS_1;
	lcd.huart.Init.OverSampling = UART_OVERSAMPLING_16;
	HAL_UART_Init(&lcd.huart);

	/* Configure a DMA channel to service the UART. */
	__HAL_RCC_DMA2_CLK_ENABLE();
	lcd.hdma.Instance = DMA2_Stream5;
	lcd.hdma.State = HAL_DMA_STATE_RESET;
	lcd.hdma.Init.Channel = DMA_CHANNEL_4;
	lcd.hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
	lcd.hdma.Init.PeriphInc = DMA_PINC_DISABLE;
	lcd.hdma.Init.MemInc = DMA_MINC_ENABLE;
	lcd.hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	lcd.hdma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	lcd.hdma.Init.Mode = DMA_CIRCULAR;
	lcd.hdma.Init.Priority = DMA_PRIORITY_LOW;
	lcd.hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	lcd.hdma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
	lcd.hdma.Init.MemBurst = DMA_MBURST_SINGLE;
	lcd.hdma.Init.PeriphBurst = DMA_PBURST_SINGLE;
	HAL_DMA_Init(&lcd.hdma);

	__HAL_LINKDMA(&lcd.huart, hdmarx, lcd.hdma);

	/* Start the receive process. */
	HAL_UART_Receive_DMA(&lcd.huart, lcd.rx_buffer, lcd.rx_length);
}

uint8_t
lcd_count() {
	return dma_serial_count(&lcd);
}

uint8_t
lcd_getc() {
	return dma_serial_getc(&lcd);
}

uint8_t
lcd_read(uint8_t* buffer, uint8_t position, uint8_t length) {
	return dma_serial_read(&lcd, buffer, position, length);
}

uint8_t
lcd_write(uint8_t* buffer, uint8_t length) {
	if (HAL_UART_Transmit(lcd_handle(), buffer, length, 1000) == HAL_OK) {
		return length;
	}
	return 0;
}
