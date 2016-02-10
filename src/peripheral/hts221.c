#include <peripheral/hts221.h>
#include <peripheral/i2c_spi_bus.h>
#include "math.h"

HTS221_CalibTypeDef hts221_calib = { 0 };

/**
 * Read the WHO_AM_I register to detect the presence of the sensor.
 * The sensor should respond with HTS221_WHO_I_AM.
 */
uint8_t
hts221_who_am_i() {
	uint8_t value;
	if (i2c_read8(HTS221_ADDRESS_R, HTS221_WHO_AM_I, &value, 1) != DEVICES_OK) {
		return 0;
	}
	return value;
}

/**
 * Power up and calibrate the HTS221 sensor.
 */
uint8_t
hts221_init() {
	if (hts221_who_am_i() != HTS221_WHO_I_AM) {
		goto not_present;
	}

	/* Store the calibration data for the sensor. */
	if (hts221_read_calib(&hts221_calib) != DEVICES_OK) {
		goto not_ready;
	}

	/* Configure the sample rate for the sensor. */
	HTS221_ResConfTypeDef res_conf = { 0 };
	res_conf.AverageHumidity = HTS221_AVGH_512;
	res_conf.AverageTemperature = HTS221_AVGT_256;
	if (hts221_res_conf(&res_conf) != DEVICES_OK) {
		goto not_ready;
	}

	/* Configure the data rate and power status. */
	HTS221_CtrlReg1TypeDef ctrl_reg_1 = { 0 };
	ctrl_reg_1.PowerDown = HTS221_POWER_UP;
	ctrl_reg_1.OutputDataRate = HTS221_RATE_1Hz;
	ctrl_reg_1.BlockDataUpdate = HTS221_BDU_ENABLE;
	if (hts221_setup(&ctrl_reg_1) != DEVICES_OK) {
		goto not_ready;
	}

	trace_printf("hts221: ready\n");
	return 1;

not_present:
	trace_printf("hts221: not present\n");
	return 0;

not_ready:
	trace_printf("hts221: not ready\n");
	return 0;
}

/**
 * Retrieve the calibration data from the sensor.
 * This is required to interpolate the physical units from the sensor values.
 */
Devices_StatusTypeDef
hts221_read_calib(HTS221_CalibTypeDef* calib) {
	/* The calibration registers are in order - read them all at once. */
	uint8_t buffer[16] = { 0 };
	if (i2c_read8(HTS221_ADDRESS_R, I2C_AAI(HTS221_H0_rH_x2), buffer, 16) != DEVICES_OK) {
		return DEVICES_ERROR;
	}

	calib->H0_rH = (float)buffer[0] / 2.0;
	calib->H1_rH = (float)buffer[1] / 2.0;
	calib->T0_degC = (float)(buffer[2] + ((buffer[5] & 0x3) << 8)) / 8.0;
	calib->T1_degC = (float)(buffer[3] + ((buffer[5] & 0xC) << 6)) / 8.0;
	calib->H0_T0_OUT = *(int16_t*)&buffer[6];
	calib->H1_T0_OUT = *(int16_t*)&buffer[10];
	calib->T0_OUT = *(int16_t*)&buffer[12];
	calib->T1_OUT = *(int16_t*)&buffer[14];

	return DEVICES_OK;
}

/**
 * Configure the sensor resolution. (The number of conversions which are
 * averaged to produce a sample.)
 */
Devices_StatusTypeDef
hts221_res_conf(HTS221_ResConfTypeDef* config) {
	uint8_t av_conf =
			  config->AverageHumidity
			| config->AverageTemperature;

	return i2c_write8_8(HTS221_ADDRESS_W, HTS221_AV_CONF, av_conf);
}

/**
 * Configure the power state, data rate, etc. of the sensor.
 * The sensor needs to be powered up before reading can be taken.
 */
Devices_StatusTypeDef
hts221_setup(HTS221_CtrlReg1TypeDef* config) {
	uint8_t ctrl_reg_1 =
			  config->PowerDown
			| config->BlockDataUpdate
			| config->OutputDataRate;

	return i2c_write8_8(HTS221_ADDRESS_W, HTS221_CTRL_REG_1, ctrl_reg_1);
}

/**
 * Read the temperature register from the sensor.
 */
int16_t
hts221_read_temp() {
	int16_t buffer = 0;
	if (i2c_read8(HTS221_ADDRESS_R, I2C_AAI(HTS221_TEMP_OUT_L), (uint8_t*)&buffer, 2) != DEVICES_OK) {
		return INT16_MIN;
	}
	return buffer;
}

/**
 * Read the humidity register from the sensor.
 */
int16_t
hts221_read_hum() {
	int16_t buffer = 0;
	if (i2c_read8(HTS221_ADDRESS_R, I2C_AAI(HTS221_HUMIDITY_OUT_L), (uint8_t*)&buffer, 2) != DEVICES_OK) {
		return INT16_MIN;
	}
	return buffer;
}

/**
 * Read the temperature in degrees Celsius.
 */
float
hts221_read_temp_C() {
	int16_t temp = hts221_read_temp();
	if (temp == INT16_MIN) {
		return NAN;
	}

	/* Interpolate the temperature */
	HTS221_CalibTypeDef* c = &hts221_calib;
	return c->T0_degC + (float)(c->T1_degC - c->T0_degC) *
			((float)(temp - c->T0_OUT) / (float)(c->T1_OUT - c->T0_OUT));
}

/**
 * Read the humidity in percent relative humidity (%rH).
 */
float
hts221_read_hum_rel() {
	int16_t hum = hts221_read_hum();
	if (hum == INT16_MIN) {
		return NAN;
	}

	/* Interpolate the humidity */
	HTS221_CalibTypeDef* c = &hts221_calib;
	return c->H0_rH + (float)(c->H1_rH - c->H0_rH) *
			((float)(hum - c->H0_T0_OUT) / (float)(c->H1_T0_OUT - c->H0_T0_OUT));
}
