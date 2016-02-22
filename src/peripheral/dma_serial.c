#include <peripheral/dma_serial.h>
#include <math.h>

uint8_t const *
head_ptr(DMA_SerialHandle* handle) {
	return handle->rx_buffer + handle->rx_length -
			__HAL_DMA_GET_COUNTER(handle->huart.hdmarx);
}

/**
 * Get the number of characters available in the serial buffer.
 */
uint8_t
dma_serial_count(DMA_SerialHandle* handle) {
	uint8_t const* head = head_ptr(handle);
	uint8_t	const* tail = handle->rx_tail_ptr;

	if (head >= tail) {
		return (head - tail);
	} else {
		return head - tail + handle->rx_length;
	}
}

/**
 * Get the next character in the serial buffer.
 */
uint8_t
dma_serial_getc(DMA_SerialHandle* handle) {
	uint8_t const* head = head_ptr(handle);
	uint8_t	const* tail = handle->rx_tail_ptr;

	if (head != tail) {
		char c =  *handle->rx_tail_ptr++;
		if (handle->rx_tail_ptr >= handle->rx_buffer + handle->rx_length) {
			handle->rx_tail_ptr -= handle->rx_length;
		}
		return c;
	}
	return 0xFF;
}

/**
 * Read up to 'length' characters from the ring buffer.
 */
uint8_t
dma_serial_read(DMA_SerialHandle* handle, uint8_t* buffer, uint8_t position,
		uint8_t length) {

	// TODO A proper implementation
	uint8_t count = dma_serial_count(handle);
	if (length < count) {
		count = length;
	}

	for (int i = 0; i < count; ++i) {
		buffer[i + position] = dma_serial_getc(handle);
	}

	return count;
}
