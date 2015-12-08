#include "devices.h"
#include "arducam.h"
#include "lps331.h"
#include "hts221.h"
#include "stlm75.h"

SPI_HandleTypeDef hspi;
I2C_HandleTypeDef hi2c;

/* SPI ---------------------------------------------------------------------- */

/**
 * Setup the SPI bus and initialize all sensors on the bus.
 */
void
spi_init() {
	/* Configure the SPI peripheral that interfaces the camera and SD. */
	__HAL_RCC_SPI1_CLK_ENABLE();
	hspi.Instance = SPI1;
	hspi.State = HAL_SPI_STATE_RESET;
	hspi.Init.Mode = SPI_MODE_MASTER;
	hspi.Init.Direction = SPI_DIRECTION_2LINES;
	hspi.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi.Init.NSS = SPI_NSS_SOFT;
	hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
	hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi.Init.CRCPolynomial = 1;
	if (HAL_SPI_Init(&hspi) != HAL_OK) {
		trace_printf("HAL SPI setup failed\n");
		return;
	}

	if (arducam_chip() == ARDUCHIP_5MP) {
		if (arducam_setup() != DEVICES_OK) {
			return;
		}

		trace_printf("camera/arduchip: ready\n");
	}
}

/**
 * Assert the slave select signal for the SPI device.
 */
void
spi_select(Devices_SS slave) {
	switch (slave) {
	case SLAVE_SDCARD:
		HAL_GPIO_WritePin(SDCARD_SS_PORT, SDCARD_SS_PIN, GPIO_PIN_RESET);
		break;
	case SLAVE_ARDUCAM:
		HAL_GPIO_WritePin(ARDUCAM_SS_PORT, ARDUCAM_SS_PIN, GPIO_PIN_RESET);
		break;
	}
}

/**
 * Release the slave select signal for the SPI device.
 */
void
spi_release(Devices_SS slave) {
	switch (slave) {
	case SLAVE_SDCARD:
		HAL_GPIO_WritePin(SDCARD_SS_PORT, SDCARD_SS_PIN, GPIO_PIN_SET);
		break;
	case SLAVE_ARDUCAM:
		HAL_GPIO_WritePin(ARDUCAM_SS_PORT, ARDUCAM_SS_PIN, GPIO_PIN_SET);
		break;
	}
}

/**
 * Read an 8-bit value from an 8-bit register over SPI.
 */
Devices_StatusTypeDef
spi_read8(SPI_HandleTypeDef* hspi, uint8_t address, uint8_t* data, uint8_t length) {
	uint8_t ignored[16];

	if (HAL_SPI_TransmitReceive(hspi, &address, ignored, 1, 1024) != HAL_OK) {
		return DEVICES_ERROR;
	}

	if (HAL_SPI_TransmitReceive(hspi, ignored, data, length, 1024) != HAL_OK) {
		return DEVICES_ERROR;
	}

	return DEVICES_OK;
}

/**
 * Write an 8-bit value to an 8-bit register address over SPI.
 */
Devices_StatusTypeDef
spi_write8_8(SPI_HandleTypeDef* hspi, uint8_t address, uint8_t data) {
	uint8_t buffer[2] = { address | 0x80, data };
	if (HAL_SPI_TransmitReceive(hspi, buffer, buffer, 2, 1024) != HAL_OK) {
		return DEVICES_ERROR;
	}
	return DEVICES_OK;
}

/* I2C ---------------------------------------------------------------------- */

/**
 * Setup the I2C bus and initialize all sensors on the bus.
 */
void
i2c_init() {
	/* Configure the I2C peripheral that interfaces the sensors. */
	__HAL_RCC_I2C1_CLK_ENABLE();
	hi2c.Instance = I2C1;
	hi2c.State = HAL_I2C_STATE_RESET;
	hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c.Init.ClockSpeed = 30000;
	hi2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c.Init.OwnAddress1 = 0;
	hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c.Init.OwnAddress2 = 0;
	hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c) != HAL_OK) {
		trace_printf("HAL I2C setup failed\n");
		return;
	}

	/* Configure the OV5642 camera sensor. */
	if (ov5642_chip() == OV5642_CHIP_ID) {
		if (ov5642_setup() != DEVICES_OK) {
			return;
		}

		trace_printf("camera/ov5642: ready\n");
	}

	/* Configure the LPS331. */
	if (lps331_who_am_i() == LPS331_WHO_I_AM) {
		/* Use the average of 512 pressure and 128 temperature samples. */
		LPS331_ResConfTypeDef lps331_res = { 0 };
		lps331_res.AveragePressure = LPS331_AVGP_512;
		lps331_res.AverageTemperature = LPS331_AVGT_128;
		if (lps331_res_conf(&lps331_res) != DEVICES_OK) {
			return;
		}

		/* Configure the data rate and power status. */
		LPS331_CtrlReg1TypeDef lps331_ctrl1 = { 0 };
		lps331_ctrl1.PowerDown = LPS331_POWER_UP;
		lps331_ctrl1.OutputDataRate = LPS331_RATE_1_1;
		lps331_ctrl1.BlockDataUpdate = LPS331_BDU_ENABLE;
		if (lps331_setup(&lps331_ctrl1) != DEVICES_OK) {
			return;
		}

		trace_printf("lps331: ready\n");
	}

	/* Configure the HTS221. */
	if (hts221_who_am_i() == HTS221_WHO_I_AM) {
		/* Store the calibration data for the sensor. */
		hts221_read_calib(&hts221_calib);

		/* Configure the sample rate for the sensor. */
		HTS221_ResConfTypeDef res_conf = { 0 };
		res_conf.AverageHumidity = HTS221_AVGH_512;
		res_conf.AverageTemperature = HTS221_AVGT_256;
		if (hts221_res_conf(&res_conf) != DEVICES_OK) {
			return;
		}

		/* Configure the data rate and power status. */
		HTS221_CtrlReg1TypeDef ctrl_reg_1 = { 0 };
		ctrl_reg_1.PowerDown = HTS221_POWER_UP;
		ctrl_reg_1.OutputDataRate = HTS221_RATE_1Hz;
		ctrl_reg_1.BlockDataUpdate = HTS221_BDU_ENABLE;
		if (hts221_setup(&ctrl_reg_1) != DEVICES_OK) {
			return;
		}

		trace_printf("hts221: ready\n");
	}

	/* The STLM75 boots into a ready to use state. However, it can be placed
	   into a lower power state with the following command:
	STLM75_ConfTypeDef conf = { 0 };
	conf.Shutdown = STLM75_POWER_DOWN;
	stlm75_setup(&conf); */
}

/**
 * Helper method to read from an 8-bit address over I2C.
 */
Devices_StatusTypeDef
i2c_read8(uint8_t device, uint8_t address, uint8_t* data, uint8_t length) {
	/* Indicate the start address of the read. */
	if (HAL_I2C_Master_Transmit(&hi2c, device, &address, 1, 1000) != HAL_OK) {
		return DEVICES_ERROR;
	}

	/* Read the desired number of bytes. */
	if (HAL_I2C_Master_Receive(&hi2c, device, data, length, 1000) != HAL_OK) {
		return DEVICES_ERROR;
	}

	return DEVICES_OK;
}

/**
 * Helper method to read from a 16-bit address over I2C.
 */
Devices_StatusTypeDef
i2c_read16(uint8_t device, uint16_t address, uint8_t* data, uint8_t length) {
	/* Indicate the start address of the read. */
	//address = ((address & 0xFF00) >> 8) | ((address & 0xFF) << 8);
	address = __builtin_bswap16(address);
	if (HAL_I2C_Master_Transmit(&hi2c, device, (uint8_t*)&address, 2, 1000) != HAL_OK) {
		return DEVICES_ERROR;
	}

	/* Read the desired number of bytes. */
	if (HAL_I2C_Master_Receive(&hi2c, device, data, length, 1000) != HAL_OK) {
		return DEVICES_ERROR;
	}

	return DEVICES_OK;
}

/**
 * Helper method to write an 8-bit value to a 8-bit address over I2C.
 */
Devices_StatusTypeDef
i2c_write8_8(uint8_t device, uint8_t address, uint8_t data) {
	/* Write the address and data. */
	uint8_t buffer[2] = { address, data };
	if (HAL_I2C_Master_Transmit(&hi2c, device, buffer, 2, 1000) != HAL_OK) {
		return DEVICES_ERROR;
	}
	return DEVICES_OK;
}

/**
 * Helper method to write an 8-bit value to a 16-bit address over I2C.
 */
Devices_StatusTypeDef
i2c_write16_8(uint8_t device, uint16_t address, uint8_t data) {
	/* Write the address and data. */
	uint8_t output[3] = { (address & 0xFF00) >> 8, (address & 0xFF), data };
	if (HAL_I2C_Master_Transmit(&hi2c, device, output, 3, 1000) != HAL_OK) {
		return DEVICES_ERROR;
	}
	return DEVICES_OK;
}

/**
 * Helper method to write an array of 8-bit values to 16-bit addresses.
 */
Devices_StatusTypeDef
i2c_array16_8(uint8_t device, RegisterTuple16_8* array) {
	while(array->address != 0xFFFF) {
		if (i2c_write16_8(device, array->address, array->value) != DEVICES_OK) {
			return DEVICES_ERROR;
		}
		++array;
	}
	return DEVICES_OK;
}
