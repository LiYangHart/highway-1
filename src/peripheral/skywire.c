#include <peripheral/dma_serial.h>
#include <peripheral/skywire.h>
#include <string.h>

uint8_t rx_buffer[128];
DMA_SerialHandle skywire = {
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
skywire_handle() {
	return &skywire.huart;
}

/**
 * Set the pin state of the Skywire ON_OFF pin.
 *
 * The pin is configured by HAL_UART_Init() in stm32f4xx_hal_msp.c.
 */
void
skywire_en(GPIO_PinState pinState) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, pinState);
}

/**
 * Set the pin state of the Skywire RTS pin.
 *
 * The pin is configured by HAL_UART_Init() in stm32f4xx_hal_msp.c.
 */
void
skywire_rts(GPIO_PinState pinState) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, pinState);
}

/**
 * Initialize the UART to communicate with the Skywire modem.
 */
void
skywire_init() {
	/* Configure the UART for the Skywire modem. */
	__HAL_RCC_USART1_CLK_ENABLE();
	skywire.huart.Instance = USART1;
	skywire.huart.State = HAL_UART_STATE_RESET;
	skywire.huart.Init.BaudRate = 115200;
	skywire.huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	skywire.huart.Init.Mode = UART_MODE_TX_RX;
	skywire.huart.Init.WordLength = UART_WORDLENGTH_8B;
	skywire.huart.Init.Parity = UART_PARITY_NONE;
	skywire.huart.Init.StopBits = UART_STOPBITS_1;
	skywire.huart.Init.OverSampling = UART_OVERSAMPLING_16;
	HAL_UART_Init(&skywire.huart);

	/* Configure a DMA channel to service the UART. */
	__HAL_RCC_DMA2_CLK_ENABLE();
	skywire.hdma.Instance = DMA2_Stream5;
	skywire.hdma.State = HAL_DMA_STATE_RESET;
	skywire.hdma.Init.Channel = DMA_CHANNEL_4;
	skywire.hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
	skywire.hdma.Init.PeriphInc = DMA_PINC_DISABLE;
	skywire.hdma.Init.MemInc = DMA_MINC_ENABLE;
	skywire.hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	skywire.hdma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	skywire.hdma.Init.Mode = DMA_CIRCULAR;
	skywire.hdma.Init.Priority = DMA_PRIORITY_LOW;
	skywire.hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	skywire.hdma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
	skywire.hdma.Init.MemBurst = DMA_MBURST_SINGLE;
	skywire.hdma.Init.PeriphBurst = DMA_PBURST_SINGLE;
	HAL_DMA_Init(&skywire.hdma);

	__HAL_LINKDMA(&skywire.huart, hdmarx, skywire.hdma);

	/* Start the receive process. */
	HAL_UART_Receive_DMA(&skywire.huart, skywire.rx_buffer, skywire.rx_length);
}

/**
 * Perform the activation function for the Skywire modem.
 *
 * This requires a pulse of 1s < HOLD_TIME < 2s to be applied to ON_OFF.
 * The modem may not respond to commands for up to 15 seconds.
 */
void
skywire_activate() {
	skywire_rts(GPIO_PIN_RESET);

	/* The ON/activation pulse shall have timing: 1s < HOLD_TIME < 2s. */
	skywire_en(GPIO_PIN_RESET);
	vTaskDelay(1500 / portTICK_PERIOD_MS);
	skywire_en(GPIO_PIN_SET);
	vTaskDelay(1500 / portTICK_PERIOD_MS);
	skywire_en(GPIO_PIN_RESET);
}

uint8_t
skywire_count() {
	return dma_serial_count(&skywire);
}

uint8_t
skywire_getc() {
	return dma_serial_getc(&skywire);
}

uint8_t
skywire_read(uint8_t* buffer, uint8_t position, uint8_t length) {
	return dma_serial_read(&skywire, buffer, position, length);
}

Skywire_StatusTypeDef
skywire_write(uint8_t* buffer, uint8_t start, uint8_t length) {
	return HAL_UART_Transmit(skywire_handle(), buffer + start, length, 10000);
}

