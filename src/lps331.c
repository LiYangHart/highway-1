#include "sensors.h"
#include "lps331.h"
#include "math.h"

/**
 * Read the WHO_AM_I register to detect the presence of the sensor.
 * The sensor should respond with LPS331_WHO_I_AM.
 */
uint8_t
lps331_who_am_i() {
	uint8_t value;
	if (i2c_read(LPS331_ADDRESS, LPS331_WHO_AM_I, &value, 1) != SENSORS_OK) {
		return 0;
	}
	return value;
}

/**
 * Configure the sensor resolution. (The number of conversions which are
 * averaged to produce a sample.)
 */
Sensors_StatusTypeDef
lps331_res_conf(LPS331_ResConfTypeDef* config) {
	uint8_t res_conf =
			  config->AveragePressure
			| config->AverageTemperature;

	return i2c_write(LPS331_ADDRESS, LPS331_RES_CONF, res_conf);
}

/**
 * Configure the power state, data rate, etc. of the sensor.
 * The sensor needs to be powered up before reading can be taken.
 */
Sensors_StatusTypeDef
lps331_setup(LPS331_CtrlReg1TypeDef* config) {
	uint8_t ctrl_reg_1 =
			  config->PowerDown
			| config->OutputDataRate
			| config->Differential
			| config->BlockDataUpdate
			| config->Delta
			| config->SpiInterfaceMode;

	return i2c_write(LPS331_ADDRESS, LPS331_CTRL_REG_1, ctrl_reg_1);
}

/**
 * Read the temperature register from the sensor.
 */
int16_t
lps331_read_temp() {
	int16_t buffer = 0;
	if (i2c_read(LPS331_ADDRESS, I2C_AAI(LPS331_TEMP_OUT_L), (uint8_t*)&buffer, 2) != SENSORS_OK) {
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
	if (i2c_read(LPS331_ADDRESS, I2C_AAI(LPS331_PRESS_OUT_XL), (uint8_t*)&buffer, 3) != SENSORS_OK) {
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
