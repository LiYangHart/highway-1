/**
 * Media driver implementation for FreeRTOS FAT SL using SPI SD driver for SDHC and SDXC cards.
 *
 * A basic SPI SD driver implementation using HAL SPI device with polled IO.
 *
 * TODO Clock speed increase after card initialization.
 * TODO Implement read and write sector using DMA or interrupt.
 *
 * Author: Mark Lieberman
 */

#ifndef _API_MDRIVER_SPI_SD_H_
#define _API_MDRIVER_SPI_SD_H_

#include "api_mdriver.h"
#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>
#include "FreeRTOS.h"
#include "task.h"
#include "diag/Trace.h"

#ifdef __cplusplus
extern "C" {
#endif

/* HAL driver handle to SPI1 device. */
extern SPI_HandleTypeDef SPI_HandleSPI1;

/* SPI SD Driver response codes. */
#define SPI_SD_OK   HAL_OK
#define SPI_SD_FAIL HAL_ERROR

/* Media card types. */
#define SPI_SD_CARD_UNKNOWN 0
#define SPI_SD_CARD_SD2     1
#define SPI_SD_CARD_SD1     2
#define SPI_SD_CARD_MMC3    3

/**
 * User data struct for F_DRIVER containing SPI SD driver information.
 */
typedef struct {
	SPI_HandleTypeDef* hspi;    /* Handle to SPI HAL driver */
	GPIO_TypeDef* ss_gpio_port; /* Slave select port */
	uint16_t ss_gpio_pin;       /* Slave select pin */
	uint8_t card_type;          /* Type of memory card */
	uint8_t card_ready;         /* Flag indicating card is mounted */
} MMC_SD_MDriver;

/* MDriver API */
F_DRIVER * mmc_spi_initfunc ( unsigned long driver_param );


#ifdef __cplusplus
}
#endif

#endif /* _API_MDRIVER_SPI_SD_H_ */

