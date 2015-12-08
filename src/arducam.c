#include "arducam.h"
#include "devices.h"
#include "diag/Trace.h"

/* ArduChip ----------------------------------------------------------------- */

/**
 * Read the ArduChip version.
 */
uint8_t
arducam_chip() {
	uint8_t version = 0;

	/* Emit a few clock cycles so the ArduChip can get ready. */
	spi_release(SLAVE_ARDUCAM);
	spi_read8(&hspi, 0x0, &version, 1);
	spi_read8(&hspi, 0x0, &version, 1);
	spi_read8(&hspi, 0x0, &version, 1);
	spi_read8(&hspi, 0x0, &version, 1);

	/* Read the ArduChip version register. */
	version = 0;
	spi_select(SLAVE_ARDUCAM);
	if (spi_read8(&hspi, ARDUCHIP_VERSION, &version, 1) != DEVICES_OK) {
		spi_release(SLAVE_ARDUCAM);
		return 0;
	}

	spi_release(SLAVE_ARDUCAM);
	uint8_t x;
	spi_read8(&hspi, 0x0, &x, 1);
	return version;
}

/**
 * Configure the ArduChip for FIFO mode with single image capture.
 */
Devices_StatusTypeDef
arducam_setup() {
	uint8_t flags;


	spi_release(SLAVE_ARDUCAM);
	vTaskDelay(50);
	spi_select(SLAVE_ARDUCAM);

	/* Configure the chip to use FIFO mode with active-low VSYNC */
	flags = SITR_FIFO_MASK | SITR_VSYNC_MASK;
	if (spi_write8_8(&hspi, ARDUCHIP_SITR, flags) != DEVICES_OK) {
		spi_release(SLAVE_ARDUCAM);
		return DEVICES_ERROR;
	}

	spi_release(SLAVE_ARDUCAM);
	vTaskDelay(50);
	spi_select(SLAVE_ARDUCAM);

	/* Configure the chip to capture one frame. */
	if (spi_write8_8(&hspi, ARDUCHIP_CCR, CCR_FRAMES(1)) != DEVICES_OK) {
		spi_release(SLAVE_ARDUCAM);
		return DEVICES_ERROR;
	}

	spi_release(SLAVE_ARDUCAM);
	return DEVICES_OK;
}

/**
 * Initiate a JPEG image capture.
 */
uint8_t
arducam_start_capture() {
	uint8_t flags;

	spi_select(SLAVE_ARDUCAM);

	/* Reset the FIFO pointers and FIFO write done flag. */
	flags = FIFO_CLEAR_MASK | FIFO_RDPTR_RST_MASK | FIFO_WRPTR_RST_MASK;
	if (spi_write8_8(&hspi, ARDUCHIP_FIFO_CR, flags) != DEVICES_OK) {
		spi_release(SLAVE_ARDUCAM);
		return DEVICES_ERROR;
	}

	/* Initiate a capture. */
	if (spi_write8_8(&hspi, ARDUCHIP_FIFO_CR, FIFO_START_MASK) != DEVICES_OK) {
		spi_release(SLAVE_ARDUCAM);
		return DEVICES_ERROR;
	}

	spi_release(SLAVE_ARDUCAM);
	return DEVICES_OK;
}

/**
 * Wait for the capture flag to be asserted and return the size of the data
 * written to the FIFO buffer.
 */
Devices_StatusTypeDef
arducam_wait_capture(uint32_t* capture_length) {
	uint8_t value;

	spi_select(SLAVE_ARDUCAM);
	spi_read8(&hspi, ARDUCHIP_SITR, &value, 1);
	trace_printf("sitr: %02x\n", value);
	spi_release(SLAVE_ARDUCAM);
	vTaskDelay(50);
	spi_select(SLAVE_ARDUCAM);
	spi_read8(&hspi, ARDUCHIP_CCR, &value, 1);
	trace_printf("ccr: %02x\n", value);
	spi_release(SLAVE_ARDUCAM);

	/* Poll the FIFO write done flag. */
	for(;;) {
		spi_select(SLAVE_ARDUCAM);

		if (spi_read8(&hspi, ARDUCHIP_STATUS, &value, 1) != DEVICES_OK) {
			spi_release(SLAVE_ARDUCAM);
			return DEVICES_ERROR;
		}

		/* Break when FIFO done is asserted. */
		if ((value & STATUS_FIFO_DONE_MASK) == STATUS_FIFO_DONE_MASK) {
			break;
		}

		spi_release(SLAVE_ARDUCAM);
		vTaskDelay(100);
	}

	/* Get the FIFO write size. */
	if (spi_read8(&hspi, ARDUCHIP_FIFO_WRITE_0, &value, 1) != DEVICES_OK) {
		spi_release(SLAVE_ARDUCAM);
		return DEVICES_ERROR;
	}
	*capture_length = value;

	if (spi_read8(&hspi, ARDUCHIP_FIFO_WRITE_1, &value, 1) != DEVICES_OK) {
		spi_release(SLAVE_ARDUCAM);
		return DEVICES_ERROR;
	}
	*capture_length |= (value << 8);

	if (spi_read8(&hspi, ARDUCHIP_FIFO_WRITE_2, &value, 1) != DEVICES_OK) {
		spi_release(SLAVE_ARDUCAM);
		return DEVICES_ERROR;
	}
	*capture_length |= ((value & 0x7) << 16);

	spi_release(SLAVE_ARDUCAM);
	return DEVICES_OK;
}

/**
 * Read from the FIFO buffer.
 * Returns the number of bytes written to 'buffer'.
 */
Devices_StatusTypeDef
arducam_read_capture(uint8_t* buffer, uint16_t length) {
	/* Slow read implementation - one byte per SPI transaction. */
	for (int i = 0; i < length; ++i) {
		spi_select(SLAVE_ARDUCAM);

		if (spi_read8(&hspi, ARDUCHIP_SINGLE_READ, buffer, 1) != DEVICES_OK) {
			spi_release(SLAVE_ARDUCAM);
			return DEVICES_ERROR;
		}

		spi_release(SLAVE_ARDUCAM);

		++buffer;
	}

	return DEVICES_OK;
}

Devices_StatusTypeDef
arducam_burst_read(uint8_t* buffer, uint16_t length) {
	/* Fast read implementation using burst read. */
	spi_select(SLAVE_ARDUCAM);

	if (spi_read8(&hspi, ARDUCHIP_BURST_READ, buffer, 1) != DEVICES_OK) {
		spi_release(SLAVE_ARDUCAM);
		return DEVICES_ERROR;
	}

	if (HAL_SPI_Receive(&hspi, buffer + 1, length - 1, 1024) != HAL_OK) {
		spi_release(SLAVE_ARDUCAM);
		return DEVICES_ERROR;
	}

	spi_release(SLAVE_ARDUCAM);
	return DEVICES_OK;
}

/* OV5642 I2C --------------------------------------------------------------- */

/**
 * Read the OV5642 chip ID.
 */
uint16_t
ov5642_chip() {
	uint8_t high_byte, low_byte;
	if (i2c_read16(OV5642_ADDRESS_W, OV5642_CHIP_ID_HIGH_BYTE, &high_byte, 1) != DEVICES_OK) {
		return 0;
	}

	if (i2c_read16(OV5642_ADDRESS_W, OV5642_CHIP_ID_LOW_BYTE, &low_byte, 1) != DEVICES_OK) {
		return 0;
	}

	return (high_byte << 8) | low_byte;
}

/**
 * Configure the OV5642 for JPEG capture.
 */
Devices_StatusTypeDef
ov5642_setup() {
	// Perform a software reset of the camera sensor.
	if (i2c_write16_8(OV5642_ADDRESS_W, 0x3008, 0x80) != DEVICES_OK) {
		return DEVICES_ERROR;
	}

	vTaskDelay(100);

	// Initialize the registers of the camera sensor.
	if (i2c_array16_8(OV5642_ADDRESS_W, ov5642_dvp_fmt_global_init) != DEVICES_OK) {
		return DEVICES_ERROR;
	}

	vTaskDelay(100);

	if (i2c_array16_8(OV5642_ADDRESS_W, ov5642_dvp_fmt_jpeg_qvga) != DEVICES_OK) {
		return DEVICES_ERROR;
	}

	if (i2c_write16_8(OV5642_ADDRESS_W, 0x4407, 0x0C) != DEVICES_OK) {
		return DEVICES_ERROR;
	}

	return DEVICES_OK;
}

