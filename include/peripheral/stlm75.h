/**
 * Simple I2C interface to the STLM75 sensor on the Skywire developer board.
 */

#ifndef _STLM75_H_
#define _STLM75_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include <peripheral/i2c_spi_bus.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Device addresses */
#define STLM75_ADDRESS         0x90

/* Register addresses */
#define STLM75_TEMP            0x90
#define STLM75_CONF            0x91
#define STLM75_T_HYS           0x92
#define STLM75_T_OS            0x93

typedef struct __STLM75_ConfTypeDef {
	uint8_t Shutdown;
	uint8_t ThermostatMode;
	uint8_t Polarity;
	uint8_t FaultTolerance;
} STLM75_ConfTypeDef;

#define STLM75_POWER_UP        0x00
#define STLM75_POWER_DOWN      0x01

#define STLM75_MODE_COMPARE    0x00
#define STLM75_MODE_INTERRUPT  0x02

#define STLM75_POL_ACTIVE_HIGH 0x00
#define STLM75_POL_ACTIVE_LOW  0x03

#define STLM75_FAULT_1         0x00
#define STLM75_FAULT_2         0x08
#define STLM75_FAULT_4         0x10
#define STLM75_FAULT_6         0x18

Devices_StatusTypeDef stlm75_setup(STLM75_ConfTypeDef* config);
int8_t stlm75_read_temp();
float stlm75_read_temp_C();

#ifdef __cplusplus
}
#endif

#endif /* _STLM75_H_ */

