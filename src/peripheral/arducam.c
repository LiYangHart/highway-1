#include <peripheral/arducam.h>
#include <peripheral/i2c_spi_bus.h>
#include "diag/Trace.h"
#include "FreeRTOS.h"
#include "task.h"

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
 * Initialize the ArduChip on the ArduCAM module.
 */
uint8_t
arducam_init() {
	if (arducam_chip() != ARDUCHIP_5MP) {
		goto not_present;
	}

	if (arducam_setup() != DEVICES_OK) {
		goto not_ready;
	}

	trace_printf("camera/arduchip: ready\n");
	return 1;

not_present:
	trace_printf("camera/arduchip: not present\n");
	return 0;

not_ready:
	trace_printf("camera/arduchip: not ready\n");
	return 0;

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
 * function to set arducam into low-power mode between taking images
 */

Devices_StatusTypeDef
arducam_low_power_set(){
	uint8_t gpio_value;

	spi_select(SLAVE_ARDUCAM);

	//read gpio register first, then apply mask to set low-power mode
	if (spi_read8(&hspi, ARDUCHIP_GPIO_READ, &gpio_value, 1) != DEVICES_OK){
		spi_release(SLAVE_ARDUCAM);
		trace_printf("Error reading GPIO register \n");
		return DEVICES_ERROR;
	}

	gpio_value = gpio_value | STNDBY_ENABLE_MASK;

	if (spi_write8_8(&hspi, ARDUCHIP_GPIO_WRITE, gpio_value) != DEVICES_OK){
		spi_release(SLAVE_ARDUCAM);
		trace_printf("Error setting GPIO register \n");
		return DEVICES_ERROR;
	}
	spi_release(SLAVE_ARDUCAM);
	return DEVICES_OK;
}

/**
 * function to take Arducam back out of low-power mode
 */

Devices_StatusTypeDef
arducam_low_power_remove(){
	uint8_t gpio_value;

	spi_select(SLAVE_ARDUCAM);

	//read gpio register first, then apply mask to remove low-power mode
	if (spi_read8(&hspi, ARDUCHIP_GPIO_READ, &gpio_value, 1) != DEVICES_OK){
		spi_release(SLAVE_ARDUCAM);
		trace_printf("Error reading GPIO register \n");
		return DEVICES_ERROR;
	}

	gpio_value = gpio_value & !STNDBY_ENABLE_MASK;

	if (spi_write8_8(&hspi, ARDUCHIP_GPIO_WRITE, gpio_value) != DEVICES_OK){
		spi_release(SLAVE_ARDUCAM);
		trace_printf("Error setting GPIO register \n");
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

	/* Poll the FIFO write done flag. */
	for(;;) {
		spi_select(SLAVE_ARDUCAM);

		if (spi_read8(&hspi, ARDUCHIP_STATUS, &value, 1) != DEVICES_OK) {
			spi_release(SLAVE_ARDUCAM);
			return DEVICES_ERROR;
		}

		/* Break when FIFO done is asserted. */
		if ((value & STATUS_FIFO_DONE_MASK) == STATUS_FIFO_DONE_MASK) {
			spi_release(SLAVE_ARDUCAM);
			break;
		}

		spi_release(SLAVE_ARDUCAM);
		vTaskDelay(10);
	}

	/* Get the FIFO write size. */
	spi_select(SLAVE_ARDUCAM);
	if (spi_read8(&hspi, ARDUCHIP_FIFO_WRITE_0, &value, 1) != DEVICES_OK) {
		spi_release(SLAVE_ARDUCAM);
		return DEVICES_ERROR;
	}
	spi_release(SLAVE_ARDUCAM);
	*capture_length = value;

	spi_select(SLAVE_ARDUCAM);
	if (spi_read8(&hspi, ARDUCHIP_FIFO_WRITE_1, &value, 1) != DEVICES_OK) {
		spi_release(SLAVE_ARDUCAM);
		return DEVICES_ERROR;
	}
	spi_release(SLAVE_ARDUCAM);
	*capture_length |= (value << 8);

	spi_select(SLAVE_ARDUCAM);
	if (spi_read8(&hspi, ARDUCHIP_FIFO_WRITE_2, &value, 1) != DEVICES_OK) {
		spi_release(SLAVE_ARDUCAM);
		return DEVICES_ERROR;
	}
	spi_release(SLAVE_ARDUCAM);
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
 * Initialize the CMOS sensor on the ArduCAM module.
 */
uint8_t
ov5642_init() {
	if (ov5642_chip() != OV5642_CHIP_ID) {
		goto not_present;
	}

	/* Configure the CMOS sensor for JPEG capture.
	 * TODO Start in low power mode and change modes later.
	 */
	if (ov5642_setup() != DEVICES_OK) {
		goto not_ready;
	}

	trace_printf("camera/ov5642: ready\n");
	return 1;

not_present:
	trace_printf("camera/ov5642: not present\n");
	return 0;

not_ready:
	trace_printf("camera/ov5642: not ready\n");
	return 0;
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

	if (i2c_array16_8(OV5642_ADDRESS_W, ov5642_res_1080P) != DEVICES_OK) {
		return DEVICES_ERROR;
	}

	if (i2c_write16_8(OV5642_ADDRESS_W, 0x4407, 0x0C) != DEVICES_OK) {
		return DEVICES_ERROR;
	}

	return DEVICES_OK;
}


