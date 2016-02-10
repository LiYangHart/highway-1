#include <peripheral/i2c_spi_bus.h>
#include <peripheral/lps331.h>
#include "math.h"
#include "diag/Trace.h"

/**
 * Read the WHO_AM_I register to detect the presence of the sensor.
 * The sensor should respond with LPS331_WHO_I_AM.
 */
uint8_t
lps331_who_am_i() {
	uint8_t value;
	if (i2c_read8(LPS331_ADDRESS, LPS331_WHO_AM_I, &value, 1) != DEVICES_OK) {
		return 0;
	}
	return value;
}

/**
 * Power up and configure the LP2331 sensor.
 */
uint8_t
lps331_init() {
	if (lps331_who_am_i() != LPS331_WHO_I_AM) {
		goto not_present;
	}

	/* Use the average of 512 pressure and 128 temperature samples. */
	LPS331_ResConfTypeDef lps331_res = { 0 };
	lps331_res.AveragePressure = LPS331_AVGP_512;
	lps331_res.AverageTemperature = LPS331_AVGT_128;
	if (lps331_res_conf(&lps331_res) != DEVICES_OK) {
		goto not_ready;
	}

	/* Configure the data rate and power status. */
	LPS331_CtrlReg1TypeDef lps331_ctrl1 = { 0 };
	lps331_ctrl1.PowerDown = LPS331_POWER_UP;
	lps331_ctrl1.OutputDataRate = LPS331_RATE_1_1;
	lps331_ctrl1.BlockDataUpdate = LPS331_BDU_ENABLE;
	if (lps331_setup(&lps331_ctrl1) != DEVICES_OK) {
		goto not_ready;
	}

	trace_printf("lps331: ready\n");
	return 1;

not_present:
	trace_printf("lps331: not present\n");
	return 0;

not_ready:
	trace_printf("lps331: not ready\n");
	return 0;
}

/**
 * Configure the sensor resolution. (The number of conversions which are
 * averaged to produce a sample.)
 */
Devices_StatusTypeDef
lps331_res_conf(LPS331_ResConfTypeDef* config) {
	uint8_t res_conf =
			  config->AveragePressure
			| config->AverageTemperature;

	return i2c_write8_8(LPS331_ADDRESS, LPS331_RES_CONF, res_conf);
}

/**
 * Configure the power state, data rate, etc. of the sensor.
 * The sensor needs to be powered up before reading can be taken.
 */
Devices_StatusTypeDef
lps331_setup(LPS331_CtrlReg1TypeDef* config) {
	uint8_t ctrl_reg_1 =
			  config->PowerDown
			| config->OutputDataRate
			| config->Differential
			| config->BlockDataUpdate
			| config->Delta
			| config->SpiInterfaceMode;

	return i2c_write8_8(LPS331_ADDRESS, LPS331_CTRL_REG_1, ctrl_reg_1);
}

/**
 * Read the temperature register from the sensor.
 */
int16_t
lps331_read_temp() {
	int16_t buffer = 0;
	if (i2c_read8(LPS331_ADDRESS, I2C_AAI(LPS331_TEMP_OUT_L), (uint8_t*)&buffer, 2) != DEVICES_OK) {
		return INT16_MIN;
	}
	return buffer;
}

/**
 * Read the pressure register from the sensor.
 */
int32_t
lps331_read_pres() {
	int32_t buffer = 0;
	if (i2c_read8(LPS331_ADDRESS, I2C_AAI(LPS331_PRESS_OUT_XL), (uint8_t*)&buffer, 3) != DEVICES_OK) {
		return INT32_MIN;
	}
	return buffer;
}

/**
 * Read the temperature in degrees Celsius.
 */
float
lps331_read_temp_C() {
	int16_t value = lps331_read_temp();
	if (value == INT16_MIN) {
		return NAN;
	}
	return 42.5 + ((float)value / 480.0);

}

/**
 * Read the pressure in millibar.
 */
float
lps331_read_pres_mbar() {
	int32_t value = lps331_read_pres();
	if (value == INT32_MIN) {
		return NAN;
	}
	return (float)value / 4096.0;
}
