#include <devices.h>
#include "stlm75.h"
#include "math.h"

/**
 * Configure the power state, thermostat mode, etc. of the sensor.
 * The sensor boots into the powered up state.
 */
Devices_StatusTypeDef
stlm75_setup(STLM75_ConfTypeDef* config) {
	uint8_t conf =
			  config->Shutdown
			| config->Polarity
			| config->ThermostatMode
			| config->FaultTolerance;

	return i2c_write8_8(STLM75_ADDRESS, STLM75_CONF, conf);
}

/**
 * Read the temperature in Celsius as a signed 8-bit value.
 *
 * Note: the temperature is stored with alignment such that the upper byte of
 * the temperature register is effectively the current signed 8-bit temperature
 * in Celsius.
 *
 * This works because shifting right in binary is equivalent to dividing by 2
 * and the temperature register counts the temperature in 0.5 degC increments.
 */
int8_t
stlm75_read_temp() {
	int8_t buffer[2] = { 0 };
	if (i2c_read8(STLM75_ADDRESS, STLM75_TEMP, (uint8_t*)buffer, 2) != DEVICES_OK) {
		return 0xFF;
	}
	return buffer[0];
}

/**
 * Read the temperature in Celsius with 0.5 degree resolution.
 */
float
stlm75_read_temp_C() {
	int16_t buffer = 0;
	if (i2c_read8(STLM75_ADDRESS, STLM75_TEMP, (uint8_t*)&buffer, 2) != DEVICES_OK) {
		return NAN;
	}

	/* Recover the 9-bit temperature value for better resolution. */
	return (float)(__builtin_bswap16(buffer) >> 7) / 2.0;
}
