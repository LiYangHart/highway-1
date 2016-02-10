/**
 * Interface layer for I2C and SPI devices used in this project.
 *
 * Author: Mark Lieberman
 */

#ifndef _I2C_SPI_BUS_H_
#define _I2C_SPI_BUS_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Status codes for devices functions.
 * Same as underlying HAL status codes.
 */
typedef enum {
	DEVICES_OK      = HAL_OK,
	DEVICES_ERROR   = HAL_ERROR,
	DEVICES_TIMEOUT = HAL_TIMEOUT
} Devices_StatusTypeDef;

/* SPI and I2C HAL handles */
extern SPI_HandleTypeDef hspi;
extern I2C_HandleTypeDef hi2c;

/* List of SPI slave devices */
typedef enum {
	SLAVE_SDCARD,
	SLAVE_ARDUCAM
} Devices_SS;

/* Chip select pins for slave devices */
#define SDCARD_SS_PORT  GPIOB
#define SDCARD_SS_PIN   GPIO_PIN_10
#define ARDUCAM_SS_PORT GPIOA
#define ARDUCAM_SS_PIN  GPIO_PIN_8

/* A tuple of 16-bit address and 8-bit value.*/
typedef struct {
	uint16_t address;
	uint8_t value;
} RegisterTuple16_8;

/* SPI functions */
void spi_bus_init();
void spi_select(Devices_SS slave);
void spi_release(Devices_SS slave);
Devices_StatusTypeDef spi_read8(SPI_HandleTypeDef* hspi, uint8_t address, uint8_t* data, uint8_t length);
Devices_StatusTypeDef spi_write8_8(SPI_HandleTypeDef* hspi, uint8_t address, uint8_t data);

/* I2C functions */
void i2c_bus_init();
Devices_StatusTypeDef i2c_read8(uint8_t device, uint8_t address, uint8_t* data, uint8_t length);
Devices_StatusTypeDef i2c_read16(uint8_t device, uint16_t address, uint8_t* data, uint8_t length);
Devices_StatusTypeDef i2c_write8_8(uint8_t device, uint8_t address, uint8_t data);
Devices_StatusTypeDef i2c_write16_8(uint8_t device, uint16_t address, uint8_t data);
Devices_StatusTypeDef i2c_array16_8(uint8_t device, RegisterTuple16_8* array);

#ifdef __cplusplus
}
#endif

#endif /* _I2C_SPI_BUS_H_ */

