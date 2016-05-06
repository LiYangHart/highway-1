/**
 * Simple SPI/I2C interface to the ArduCAM 5MP module.
 */

#ifndef _ARDUCAM_H_
#define _ARDUCAM_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#include <peripheral/i2c_spi_bus.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Device addresses */
#define OV5642_ADDRESS_W         0x78
#define OV5642_ADDRESS_R         0x79

/* Register addresses */
#define ARDUCHIP_TEST            0x00
#define ARDUCHIP_CCR             0x01
#define ARDUCHIP_SITR            0x03
#define ARDUCHIP_FIFO_CR         0x04
#define ARDUCHIP_GPIO_WRITE		 0x06
#define ARDUCHIP_VERSION         0x40
#define ARDUCHIP_STATUS          0x41
#define ARDUCHIP_BURST_READ      0x3C
#define ARDUCHIP_SINGLE_READ     0x3D
#define ARDUCHIP_FIFO_WRITE_0    0x42
#define ARDUCHIP_FIFO_WRITE_1    0x43
#define ARDUCHIP_FIFO_WRITE_2    0x44
#define ARDUCHIP_GPIO_READ		 0x45

#define OV5642_CHIP_ID_HIGH_BYTE 0x300A
#define OV5642_CHIP_ID_LOW_BYTE  0x300B

/* Register constants */
#define ARDUCHIP_5MP             0x41
#define OV5642_CHIP_ID           0x5642

/* Register masks */
#define STNDBY_ENABLE_MASK		 0x02
#define SITR_VSYNC_MASK          0x02
#define SITR_FIFO_MASK           0x10
#define SITR_LOW_POWER_MASK      0x10
#define FIFO_CLEAR_MASK    		 0x01
#define FIFO_START_MASK    		 0x02
#define FIFO_RDPTR_RST_MASK      0x10
#define FIFO_WRPTR_RST_MASK      0x20
#define STATUS_FIFO_DONE_MASK    0x08

/* Register access macros */
#define CCR_FRAMES(n) (n & 0x3)

/* ArduChip API */
uint8_t arducam_chip();
uint8_t arducam_init();
Devices_StatusTypeDef arducam_setup();
Devices_StatusTypeDef arducam_start_capture();
Devices_StatusTypeDef arducam_low_power_set();
Devices_StatusTypeDef arducam_low_power_remove();
Devices_StatusTypeDef arducam_wait_capture(uint32_t* capture_length);
Devices_StatusTypeDef arducam_read_capture(uint8_t* buffer, uint16_t length);
Devices_StatusTypeDef arducam_burst_read(uint8_t* buffer, uint16_t length);


/* CMOS sensor API */
uint16_t ov5642_chip();
uint8_t ov5642_init();
Devices_StatusTypeDef ov5642_setup();

/* OV5642 register configuration arrays */
extern RegisterTuple16_8 ov5642_dvp_fmt_global_init[];
extern RegisterTuple16_8 ov5642_dvp_fmt_jpeg_qvga[];
extern RegisterTuple16_8 ov5642_res_1080P[];

#ifdef __cplusplus
}
#endif

#endif /* _ARDUCAM_H_ */

