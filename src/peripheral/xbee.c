#include <peripheral/dma_serial.h>
#include <peripheral/xbee.h>
#include "FreeRTOS.h"
#include "task.h"

uint8_t rx_buffer[128];
DMA_SerialHandle xbee = {
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
xbee_handle() {
	return &xbee.huart;
}

/**
 * Initialize the UART to communicate with the XBee radio.
 */
void
xbee_init() {
	/* Configure the UART for the Skywire modem. */
	__HAL_RCC_USART6_CLK_ENABLE();
	xbee.huart.Instance = USART6;
	xbee.huart.State = HAL_UART_STATE_RESET;
	xbee.huart.Init.BaudRate = 115200;
	xbee.huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	xbee.huart.Init.Mode = UART_MODE_TX_RX;
	xbee.huart.Init.WordLength = UART_WORDLENGTH_8B;
	xbee.huart.Init.Parity = UART_PARITY_NONE;
	xbee.huart.Init.StopBits = UART_STOPBITS_1;
	xbee.huart.Init.OverSampling = UART_OVERSAMPLING_16;
	HAL_UART_Init(&xbee.huart);

	/* Configure a DMA channel to service the UART. */
	__HAL_RCC_DMA2_CLK_ENABLE();
	xbee.hdma.Instance = DMA2_Stream1;
	xbee.hdma.State = HAL_DMA_STATE_RESET;
	xbee.hdma.Init.Channel = DMA_CHANNEL_5;
	xbee.hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
	xbee.hdma.Init.PeriphInc = DMA_PINC_DISABLE;
	xbee.hdma.Init.MemInc = DMA_MINC_ENABLE;
	xbee.hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	xbee.hdma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	xbee.hdma.Init.Mode = DMA_CIRCULAR;
	xbee.hdma.Init.Priority = DMA_PRIORITY_LOW;
	xbee.hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	xbee.hdma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
	xbee.hdma.Init.MemBurst = DMA_MBURST_SINGLE;
	xbee.hdma.Init.PeriphBurst = DMA_PBURST_SINGLE;
	HAL_DMA_Init(&xbee.hdma);

	__HAL_LINKDMA(&xbee.huart, hdmarx, xbee.hdma);

	/* Start the receive process. */
	HAL_UART_Receive_DMA(&xbee.huart, xbee.rx_buffer, xbee.rx_length);
}

uint8_t
xbee_count() {
	return dma_serial_count(&xbee);
}

uint8_t
xbee_getc() {
	return dma_serial_getc(&xbee);
}

uint8_t
xbee_read(uint8_t* buffer, uint8_t position, uint8_t length) {
	return dma_serial_read(&xbee, buffer, position, length);
}

uint8_t
xbee_write(uint8_t* buffer, uint8_t length) {
	if (HAL_UART_Transmit(xbee_handle(), buffer, length, 1000) == HAL_OK) {
		return length;
	}
	return 0;
}
