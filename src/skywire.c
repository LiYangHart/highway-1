#include "skywire.h"
#include "dma_serial.h"

UART_HandleTypeDef huart_skywire;
DMA_HandleTypeDef hdma_skywire;

uint8_t rx_buffer[RX_BUFFER_LEN];
uint8_t const * rx_tail_ptr;

UART_HandleTypeDef*
skywire_handle() {
	return &huart_skywire;
}

void
skywire_en(GPIO_PinState pinState) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, pinState);
}

void
skywire_rts(GPIO_PinState pinState) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, pinState);
}

void
skywire_init() {
	/* Configure the UART for the Skywire modem. */
	__HAL_RCC_USART1_CLK_ENABLE();
	huart_skywire.Instance = USART1;
	huart_skywire.State = HAL_UART_STATE_RESET;
	huart_skywire.Init.BaudRate = 115200;
	huart_skywire.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart_skywire.Init.Mode = UART_MODE_TX_RX;
	huart_skywire.Init.WordLength = UART_WORDLENGTH_8B;
	huart_skywire.Init.Parity = UART_PARITY_NONE;
	huart_skywire.Init.StopBits = UART_STOPBITS_1;
	huart_skywire.Init.OverSampling = UART_OVERSAMPLING_16;
	HAL_UART_Init(&huart_skywire);

	/* Configure a DMA channel to service the UART. */
	__HAL_RCC_DMA2_CLK_ENABLE();
	hdma_skywire.Instance = DMA2_Stream5;
	hdma_skywire.State = HAL_DMA_STATE_RESET;
	hdma_skywire.Init.Channel = DMA_CHANNEL_4;
	hdma_skywire.Init.Direction = DMA_PERIPH_TO_MEMORY;
	hdma_skywire.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_skywire.Init.MemInc = DMA_MINC_ENABLE;
	hdma_skywire.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_skywire.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	hdma_skywire.Init.Mode = DMA_CIRCULAR;
	hdma_skywire.Init.Priority = DMA_PRIORITY_LOW;
	hdma_skywire.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	hdma_skywire.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
	hdma_skywire.Init.MemBurst = DMA_MBURST_SINGLE;
	hdma_skywire.Init.PeriphBurst = DMA_PBURST_SINGLE;
	HAL_DMA_Init(&hdma_skywire);

	__HAL_LINKDMA(&huart_skywire, hdmarx, hdma_skywire);

	/* Start the receive process. */
	HAL_UART_Receive_DMA(&huart_skywire, rx_buffer, RX_BUFFER_LEN);
	rx_tail_ptr = rx_buffer;
}

void
skywire_activate() {
	skywire_rts(GPIO_PIN_RESET);

	/* The ON/activation pulse shall have timing: 1s < HOLD_TIME < 2s. */
	skywire_en(GPIO_PIN_RESET);
	vTaskDelay(1500 / portTICK_PERIOD_MS);
	skywire_en(GPIO_PIN_SET);
	vTaskDelay(1500 / portTICK_PERIOD_MS);
	skywire_en(GPIO_PIN_RESET);

	/* The modem will not respond to any AT commands for 15 seconds. */
	vTaskDelay(16000 / portTICK_PERIOD_MS);
}

uint8_t
skywire_available() {
	uint8_t const * head = rx_buffer + RX_BUFFER_LEN - __HAL_DMA_GET_COUNTER(huart_skywire.hdmarx);
	uint8_t const * tail = rx_tail_ptr;
	if( head >= tail ) {
		return head - tail;
	} else {
		return head - tail + RX_BUFFER_LEN;
	}
}

uint8_t
skywire_getchar() {
	uint8_t const * head = rx_buffer + RX_BUFFER_LEN - __HAL_DMA_GET_COUNTER(huart_skywire.hdmarx);
	uint8_t const * tail = rx_tail_ptr;
	if (head != tail) {
	    char c =  *rx_tail_ptr++;
	    if (rx_tail_ptr >= rx_buffer + RX_BUFFER_LEN) {
	        rx_tail_ptr -= RX_BUFFER_LEN;
	    }
	    return c;
	}
	return 0xFF;
}
