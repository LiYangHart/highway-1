/**
 * Simple I2C interface to the HTS221 sensor on the Skywire developer board.
 */

#ifndef _HTS221_H_
#define _HTS221_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Device addresses */
#define HTS221_ADDRESS_W       0xBE
#define HTS221_ADDRESS_R       0xBF

/* Register addresses */
#define HTS221_WHO_AM_I        0x0F
#define HTS221_AV_CONF         0x10
#define HTS221_CTRL_REG_1      0x20
#define HTS221_HUMIDITY_OUT_L  0x28
#define HTS221_HUMIDITY_OUT_H  0x29
#define HTS221_TEMP_OUT_L      0x2A
#define HTS221_TEMP_OUT_H      0x2B
#define HTS221_H0_rH_x2        0x30
#define HTS221_H1_rH_x2        0x31

/* Contents of the WHO_AM_I register */
#define HTS221_WHO_I_AM        0xBC

typedef struct __HTS221_ResConfTypeDef {
	uint8_t AverageHumidity;
	uint8_t AverageTemperature;
} HTS221_ResConfTypeDef;

#define HTS221_AVGH_4          0x00
#define HTS221_AVGH_8          0x01
#define HTS221_AVGH_16         0x02
#define HTS221_AVGH_32         0x03
#define HTS221_AVGH_64         0x04
#define HTS221_AVGH_128        0x05
#define HTS221_AVGH_256        0x06
#define HTS221_AVGH_512        0x07

#define HTS221_AVGT_2          0x00
#define HTS221_AVGT_4          0x08
#define HTS221_AVGT_8          0x10
#define HTS221_AVGT_16         0x18
#define HTS221_AVGT_32         0x20
#define HTS221_AVGT_64         0x28
#define HTS221_AVGT_128        0x30
#define HTS221_AVGT_256        0x38

typedef struct __HTS221_CtrlReg1TypeDef {
	uint8_t PowerDown;
	uint8_t BlockDataUpdate;
	uint8_t OutputDataRate;
} HTS221_CtrlReg1TypeDef;

#define HTS221_POWER_DOWN      0x00
#define HTS221_POWER_UP        0x80

#define HTS221_BDU_DISABLE     0x00
#define HTS221_BDU_ENABLE      0x04

#define HTS221_ONE_SHOT        0x00
#define HTS221_RATE_1Hz        0x01
#define HTS221_RATE_7Hz        0x02
#define HTS221_RATE_12_5Hz     0x03

typedef struct __HTS221_CalibTypeDef {
	float H0_rH;
	float H1_rH;
	float T0_degC;
	float T1_degC;
	int16_t H0_T0_OUT;
	int16_t H1_T0_OUT;
	int16_t T0_OUT;
	int16_t T1_OUT;
} HTS221_CalibTypeDef;

/**
 * Calibration values stored on the sensor
 * These are required to interpolate the sensor readings into physical units.
 */
extern HTS221_CalibTypeDef hts221_calib;

uint8_t hts221_who_am_i();
Sensors_StatusTypeDef hts221_read_calib(HTS221_CalibTypeDef* calib);
Sensors_StatusTypeDef hts221_res_conf(HTS221_ResConfTypeDef* config);
Sensors_StatusTypeDef hts221_setup(HTS221_CtrlReg1TypeDef* config);
int16_t hts221_read_temp();
int16_t hts221_read_hum();
float hts221_read_temp_C();
float hts221_read_hum_rel();

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

#endif /* _HTS221_H_ */

