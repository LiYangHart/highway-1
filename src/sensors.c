#include "sensors.h"
#include "lps331.h"
#include "hts221.h"
#include "stlm75.h"

I2C_HandleTypeDef hi2c_sensors;

/**
 * Setup the I2C bus and initialize all sensors on the bus.
 */
void
sensors_init() {

	/* Configure the I2C peripheral that interfaces the sensors. */
	__HAL_RCC_I2C1_CLK_ENABLE();
	hi2c_sensors.Instance = I2C1;
	hi2c_sensors.State = HAL_I2C_STATE_RESET;
	hi2c_sensors.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c_sensors.Init.ClockSpeed = 100000;
	hi2c_sensors.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c_sensors.Init.OwnAddress1 = 0;
	hi2c_sensors.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c_sensors.Init.OwnAddress2 = 0;
	hi2c_sensors.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c_sensors.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c_sensors) != HAL_OK) {
		trace_printf("HAL I2C setup failed");
		return;
	}

	/* Configure the LPS331. */
	if (lps331_who_am_i() == LPS331_WHO_I_AM) {
		/* Use the average of 512 pressure and 128 temperature samples. */
		LPS331_ResConfTypeDef lps331_res = { 0 };
		lps331_res.AveragePressure = LPS331_AVGP_512;
		lps331_res.AverageTemperature = LPS331_AVGT_128;
		if (lps331_res_conf(&lps331_res) != SENSORS_OK) {
			return;
		}

		/* Configure the data rate and power status. */
		LPS331_CtrlReg1TypeDef lps331_ctrl1 = { 0 };
		lps331_ctrl1.PowerDown = LPS331_POWER_UP;
		lps331_ctrl1.OutputDataRate = LPS331_RATE_1_1;
		lps331_ctrl1.BlockDataUpdate = LPS331_BDU_ENABLE;
		if (lps331_setup(&lps331_ctrl1) != SENSORS_OK) {
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
		if (hts221_res_conf(&res_conf) != SENSORS_OK) {
			return;
		}

		/* Configure the data rate and power status. */
		HTS221_CtrlReg1TypeDef ctrl_reg_1 = { 0 };
		ctrl_reg_1.PowerDown = HTS221_POWER_UP;
		ctrl_reg_1.OutputDataRate = HTS221_RATE_1Hz;
		ctrl_reg_1.BlockDataUpdate = HTS221_BDU_ENABLE;
		if (hts221_setup(&ctrl_reg_1) != SENSORS_OK) {
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
 * Helper method to read from I2C slave devices.
 */
Sensors_StatusTypeDef
i2c_read(uint8_t device, uint8_t address, uint8_t* data, uint8_t length) {
	/* Indicate the start address of the read. */
	if (HAL_I2C_Master_Transmit(&hi2c_sensors, device, &address, 1, 1000) != HAL_OK) {
		return SENSORS_ERROR;
	}

	/* Read the desired number of bytes. */
	if (HAL_I2C_Master_Receive(&hi2c_sensors, device, data, length, 1000) != HAL_OK) {
		return SENSORS_ERROR;
	}

	return SENSORS_OK;
}

/**
 * Helper method to write to I2C slave devices.
 */
Sensors_StatusTypeDef
i2c_write(uint8_t device, uint8_t address, uint8_t data) {
	uint8_t output[2];
	output[0] = address;
	output[1] = data;

	/* Write the address and data. */
	if (HAL_I2C_Master_Transmit(&hi2c_sensors, device, output, 2, 1000) != HAL_OK) {
		return SENSORS_ERROR;
	}
	return SENSORS_OK;
}
