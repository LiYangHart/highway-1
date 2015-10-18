/**
 * Simple interface to I2C sensors on the Skywire developer board.
 *
 * Author: Mark Lieberman
 */

#ifndef _SENSORS_H_
#define _SENSORS_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	SENSORS_OK      = HAL_OK,
	SENSORS_ERROR   = HAL_ERROR,
	SENSORS_TIMEOUT = HAL_TIMEOUT
} Sensors_StatusTypeDef;

void sensors_init();
Sensors_StatusTypeDef i2c_read(uint8_t device, uint8_t address, uint8_t* data, uint8_t length);
Sensors_StatusTypeDef i2c_write(uint8_t device, uint8_t address, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif /* _SENSORS_H_ */

