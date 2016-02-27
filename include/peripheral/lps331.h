/**
 * Simple I2C interface to the LPS331 sensor on the Skywire developer board.
 */

#ifndef _LPS331_H_
#define _LPS331_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include <peripheral/i2c_spi_bus.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Device address */
#define LPS331_ADDRESS         0xBA

/* Register addresses */
#define LPS331_WHO_AM_I        0x0F
#define LPS331_RES_CONF        0x10
#define LPS331_CTRL_REG_1      0x20
#define LPS331_PRESS_OUT_XL    0x28
#define LPS331_PRESS_OUT_L     0x29
#define LPS331_PRESS_OUT_H     0x2A
#define LPS331_TEMP_OUT_L      0x2B
#define LPS331_TEMP_OUT_H      0x2C

/* Contents of the WHO_AM_I register */
#define LPS331_WHO_I_AM        0xBB

typedef struct {
	uint8_t AveragePressure;
	uint8_t AverageTemperature;
} LPS331_ResConfTypeDef;

#define LPS331_AVGP_1          0x00
#define LPS331_AVGP_2          0x01
#define LPS331_AVGP_4          0x02
#define LPS331_AVGP_8          0x03
#define LPS331_AVGP_16         0x04
#define LPS331_AVGP_32         0x05
#define LPS331_AVGP_64         0x06
#define LPS331_AVGP_128        0x07
#define LPS331_AVGP_256        0x08
#define LPS331_AVGP_384        0x09
#define LPS331_AVGP_512        0x0A

#define LPS331_AVGT_1          0x00
#define LPS331_AVGT_2          0x10
#define LPS331_AVGT_4          0x20
#define LPS331_AVGT_8          0x30
#define LPS331_AVGT_16         0x40
#define LPS331_AVGT_32         0x50
#define LPS331_AVGT_64         0x60
#define LPS331_AVGT_128        0x70

typedef struct __LPS331_CtrlReg1TypeDef {
	uint8_t PowerDown;
	uint8_t OutputDataRate;
	uint8_t Differential;
	uint8_t BlockDataUpdate;
	uint8_t Delta;
	uint8_t SpiInterfaceMode;
} LPS331_CtrlReg1TypeDef;

#define LPS331_POWER_DOWN      0x00
#define LPS331_POWER_UP        0x80

#define LPS331_ONE_SHOT        0x00
#define LPS331_RATE_1_1        0x10
#define LPS331_RATE_7_1        0x20
#define LPS331_RATE_12_1       0x30
#define LPS331_RATE_25_1       0x40
#define LPS331_RATE_7_7        0x50
#define LPS331_RATE_12_12      0x60
#define LPS331_RATE_25_25      0x70

#define LPS331_DIFF_DISABLE    0x00
#define LPS331_DIFF_ENABLE     0x08

#define LPS331_BDU_DISABLE     0x00
#define LPS331_BDU_ENABLE      0x04

#define LPS331_DELTA_DISABLE   0x00
#define LPS331_DELTA_ENABLE    0x02

#define LPS331_SPI_MODE_4WIRE  0x00
#define LPS331_SPI_MODE_3WIRE  0x01

uint8_t lps331_who_am_i();
uint8_t lps331_init();
Devices_StatusTypeDef lps331_res_conf(LPS331_ResConfTypeDef* config);
Devices_StatusTypeDef lps331_setup(LPS331_CtrlReg1TypeDef* config);
int16_t lps331_read_temp();
int32_t lps331_read_pres();
float lps331_read_temp_C();
float lps331_read_pres_mbar();

#ifndef I2C_AAI
/**
 * Set the MSB to enable address auto-increment when reading multiple bytes
 * over I2C.
 */
#define I2C_AAI(address) (address | 0x80)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _LPS331_H_ */

