#include "virtual_com.h"
#include "dma_serial.h"

uint8_t rx_buffer[128];
DMA_SerialHandle vcp = { { 0 }, { 0 }, rx_buffer, 128, rx_buffer };

UART_HandleTypeDef*
vcp_handle() {
	return &vcp.huart;
}

void
vcp_init() {
	/* Configure the UART for the virtual COM port. */
	__HAL_RCC_USART2_CLK_ENABLE();
	vcp.huart.Instance = USART2;
	vcp.huart.State = HAL_UART_STATE_RESET;
	vcp.huart.Init.BaudRate = 9600;
	vcp.huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	vcp.huart.Init.Mode = UART_MODE_TX_RX;
	vcp.huart.Init.WordLength = UART_WORDLENGTH_8B;
	vcp.huart.Init.Parity = UART_PARITY_NONE;
	vcp.huart.Init.StopBits = UART_STOPBITS_1;
	vcp.huart.Init.OverSampling = UART_OVERSAMPLING_16;
	HAL_UART_Init(&vcp.huart);

	/* Configure a DMA channel to service the UART. */
	__HAL_RCC_DMA1_CLK_ENABLE();
	vcp.hdma.Instance = DMA1_Stream5;
	vcp.hdma.State = HAL_DMA_STATE_RESET;
	vcp.hdma.Init.Channel = DMA_CHANNEL_4;
	vcp.hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
	vcp.hdma.Init.PeriphInc = DMA_PINC_DISABLE;
	vcp.hdma.Init.MemInc = DMA_MINC_ENABLE;
	vcp.hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	vcp.hdma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	vcp.hdma.Init.Mode = DMA_CIRCULAR;
	vcp.hdma.Init.Priority = DMA_PRIORITY_LOW;
	vcp.hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	vcp.hdma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
	vcp.hdma.Init.MemBurst = DMA_MBURST_SINGLE;
	vcp.hdma.Init.PeriphBurst = DMA_PBURST_SINGLE;
	HAL_DMA_Init(&vcp.hdma);

	__HAL_LINKDMA(&vcp.huart, hdmarx, vcp.hdma);

	/* Start the receive process. */
	HAL_UART_Receive_DMA(&vcp.huart, vcp.rx_buffer, vcp.rx_length);
}

uint8_t
vcp_available() {
	return 0;
}

uint8_t
vcp_getchar() {
	return 0;
}
