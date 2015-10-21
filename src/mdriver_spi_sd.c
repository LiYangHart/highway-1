#include <api_mdriver_spi_sd.h>

SPI_HandleTypeDef hspi_sdcard;

/* MDriver and SPI SD driver structures. */
static F_DRIVER t_driver;
static MMC_SD_MDriver spi_sd_mdriver;

/* Macros for decoding register fields. */
#define OCR_CCS_FLAG(ocr)          ((ocr >> 6) & 0x1)
#define CSD_VERSION(csd)           ((csd & 0xC0) >> 6)
#define CSD_V1                     0x0
#define CSD_V1_READ_BL_LEN(csd)    (*(csd + 5) & 0x0F)
#define CSD_V1_C_SIZE(csd)    	   (__builtin_bswap32(*(uint32_t*)(csd + 6) & 0x00C0FF03) >> 14)
#define CSD_V1_C_SIZE_MULT(csd)    ((*(csd + 8) & 0x70) >> 4)
#define CSD_V2                     0x1
#define CSD_V2_C_SIZE(csd)         (__builtin_bswap32(*(uint32_t*)(csd + 6) & 0xFFFFC000))

/* Buffer of 0xFF used to hold MOSI high when transmitting. */
uint8_t ffff_buffer[32] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
							0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
							0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
							0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

/* Buffer to receive data. */
uint8_t recv_buffer[32] = { 0 };

/* Predicates for command and data response tokens. */
typedef uint8_t (*TOKEN_PREDICATE)(uint8_t token);
uint8_t pred_res_idle(uint8_t token)   { return token == 0x01; }
uint8_t pred_res_ready(uint8_t token)  { return token == 0x00; }
uint8_t pred_res_error(uint8_t token)  { return token == 0x05; }
uint8_t pred_res_any(uint8_t token)    { return token <= 0x05; }
uint8_t pred_not_busy(uint8_t token)   { return token == 0xFF; }
uint8_t pred_data_start(uint8_t token) { return token == 0xFE; }
uint8_t pred_data_ok(uint8_t token)    { return (token & 0x0F) == 0x05; }

/**
 * Create an SD command.
 */
void
make_command(uint8_t* buffer, uint8_t command, uint32_t argument, uint8_t crc) {
	buffer[0] = 0x40 | command;
	buffer[1] = (argument >> 24) & 0xFF;
	buffer[2] = (argument >> 16) & 0xFF;
	buffer[3] = (argument >> 8)  & 0xFF;
	buffer[4] = argument         & 0xFF;
	buffer[5] = crc;
}

/**
 * Assert the slave select signal for the SPI SD device.
 */
void
spi_sd_assert_ss(MMC_SD_MDriver* spi_sd_mdriver, uint8_t select) {
	if (select) {
		HAL_GPIO_WritePin(spi_sd_mdriver->ss_gpio_port, spi_sd_mdriver->ss_gpio_pin, GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(spi_sd_mdriver->ss_gpio_port, spi_sd_mdriver->ss_gpio_pin, GPIO_PIN_SET);
	}
}

/**
 * Transmit the contents of a buffer over SPI.
 */
uint8_t
spi_sd_transmit_bytes(SPI_HandleTypeDef* hspi, uint8_t* data, uint16_t length) {
	while (length > 0) {
		uint8_t send = (length < 32) ? length : 32;
		if (HAL_SPI_TransmitReceive(hspi, data, recv_buffer, send, 65535) != HAL_OK) {
			return SPI_SD_FAIL;
		}
		length -= send;
		data += send;
	}
	return SPI_SD_OK;
}

/**
 * Receive bytes by transmitting 0xFF to hold MOSI high.
 */
uint8_t
spi_sd_receive_bytes_ff(SPI_HandleTypeDef* hspi, uint8_t* buffer, uint16_t length) {
	while (length > 0) {
		uint8_t read = (length < 32) ? length : 32;
		if (HAL_SPI_TransmitReceive(hspi, ffff_buffer, buffer, read, 65535) != HAL_OK) {
			return SPI_SD_FAIL;
		}
		buffer += read;
		length -= read;
	}
	return SPI_SD_OK;
}

/**
 * Receive bytes until the received byte satisfies the predicate.
 */
uint8_t
spi_sd_receive_token(SPI_HandleTypeDef* hspi, TOKEN_PREDICATE predicate, uint8_t* token, uint16_t attempts) {
	HAL_StatusTypeDef hr = HAL_OK;
	for (uint8_t i = 0; i < attempts && hr == HAL_OK; ++i) {
		hr = HAL_SPI_TransmitReceive(hspi, ffff_buffer, token, 1, 65535);
		if (predicate(*token)) {
			return SPI_SD_OK;
		}
	}
	*token = 0xFF;
	return SPI_SD_FAIL;
}

/**
 * Release the slave, transmit one byte of 0xFF to advance the clock, and select the slave.
 * Allows the card to recover after servicing a command.
 */
uint8_t
spi_sd_command_recover(MMC_SD_MDriver* spi_sd_mdriver) {
	SPI_HandleTypeDef* hspi = spi_sd_mdriver->hspi;
	//spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
	/* Read into the end of the receive buffer. */
	if (HAL_SPI_TransmitReceive(hspi, ffff_buffer, &recv_buffer[31], 1, 65535) != HAL_OK) {
		return SPI_SD_FAIL;
	} else {
		//spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_SELECT);
		return SPI_SD_OK;
	}
}

/**
 * Send ACMD41 until the card reports that it is ready.
 */
uint8_t
spi_sd_acmd41_loop(MMC_SD_MDriver* spi_sd_mdriver, uint32_t argument, uint8_t attempts) {
	uint8_t command[6], token = 0xFF;

	SPI_HandleTypeDef* hspi = spi_sd_mdriver->hspi;

	for (uint8_t i = 0; i < attempts && token != 0x00; ++i) {
		/* Send CMD55 - response should be card is idle. */
		make_command(command, 55, 0x00000000, 0xFF);
		if (   spi_sd_transmit_bytes(hspi, command, 6)               != SPI_SD_OK
			|| spi_sd_receive_token(hspi, pred_res_any, &token, 255) != SPI_SD_OK
			|| spi_sd_transmit_bytes(hspi, ffff_buffer, 8)           != SPI_SD_OK) {
			spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
			return SPI_SD_FAIL;
		}

		/* Send ACMD41 - response should be card is ready. */
		make_command(command, 41, argument, 0xFF);
		if (   spi_sd_transmit_bytes(hspi, command, 6)               != SPI_SD_OK
			|| spi_sd_receive_token(hspi, pred_res_any, &token, 255) != SPI_SD_OK
			|| spi_sd_command_recover(spi_sd_mdriver)                != SPI_SD_OK) {
			spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
			return SPI_SD_FAIL;
		}

		vTaskDelay(500);
	}

	return (token == 0x00) ? SPI_SD_OK : SPI_SD_FAIL;
}

/**
 * Initialize the SD card in SPI mode.
 */
uint8_t
spi_sd_init_card(MMC_SD_MDriver* spi_sd_mdriver) {
	uint8_t command[6], token;

	SPI_HandleTypeDef* hspi = spi_sd_mdriver->hspi;

	/* Send at least 74 clock transitions. */
	spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
	spi_sd_transmit_bytes(hspi, ffff_buffer, 32);
	spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_SELECT);

	/**
	 * Send the GO_IDLE_STATE (CMD0) command.
	 * Wait for the response token.
	 * Recovery time after command.
	 **/
	make_command(command, 0, 0x00000000, 0x95);
	if (   spi_sd_transmit_bytes(hspi, command, 6)               != SPI_SD_OK
	    || spi_sd_receive_token(hspi, pred_res_any, &token, 255) != SPI_SD_OK
		|| spi_sd_command_recover(spi_sd_mdriver)                != SPI_SD_OK) {
		spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
		return SPI_SD_FAIL;
	}
	if (token == 0x00) {
		/* Card is already in ready state.    */
		/* Possibly the card is not inserted. */
	}

	/**
	 * Send the SEND_IF_COND (CMD8) command.
	 * Wait for the response token.
	 **/
	make_command(command, 8, 0x000001AA, 0x87);
	if (   spi_sd_transmit_bytes(hspi, command, 6)               != SPI_SD_OK
	    || spi_sd_receive_token(hspi, pred_res_any, &token, 255) != SPI_SD_OK) {
		spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
		return SPI_SD_FAIL;
	}
	if (token == 0x05) {
		/**
		 * SDv1.0 and MMCv3 do not understand CMD8.
		 * Recovery time after command.
		 **/
		spi_sd_mdriver->card_type = SPI_SD_CARD_SD1;
		if (spi_sd_command_recover(spi_sd_mdriver) != SPI_SD_OK) {
			return SPI_SD_FAIL;
		}
	} else {
		/**
		 * Receive the interface conditions register.
		 * Recovery time after command.
		 **/
		if (   spi_sd_receive_bytes_ff(hspi, recv_buffer, 4) != SPI_SD_OK
			|| spi_sd_command_recover(spi_sd_mdriver)        != SPI_SD_OK) {
			spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
			return SPI_SD_FAIL;
		}
		uint32_t* cic = (uint32_t*)recv_buffer;
		if ((*cic & 0xAA010000) == 0xAA010000) {
			// Match - card conforms to SD v2.0.
			spi_sd_mdriver->card_type = SPI_SD_CARD_SD2;
		} else {
			// Mismatch - unknown card type.
			spi_sd_mdriver->card_type = SPI_SD_CARD_UNKNOWN;
			return SPI_SD_FAIL;
		}
	}

	/* Card specific initialization flow begins here. */
	switch (spi_sd_mdriver->card_type) {
	case SPI_SD_CARD_SD2:
		/* Put the card into the ready state. */
		if (spi_sd_acmd41_loop(spi_sd_mdriver, 0x40000000, 255) != SPI_SD_OK) {
			spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
			return SPI_SD_FAIL;
		}

		/**
		 * Send the READ_OCR (CMD58) command.
		 * Wait for the response token.
		 * Receive the operating conditions register.
		 * Recovery time after command.
		 **/
		make_command(command, 58, 0x00000000, 0xFF);
		if (   spi_sd_transmit_bytes(hspi, command, 6)                 != SPI_SD_OK
			|| spi_sd_receive_token(hspi, pred_res_ready, &token, 255) != SPI_SD_OK
			|| spi_sd_receive_bytes_ff(hspi, recv_buffer, 4)           != SPI_SD_OK
			|| spi_sd_command_recover(spi_sd_mdriver)                  != SPI_SD_OK) {
			spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
			return SPI_SD_FAIL;
		}
		uint32_t* ocr = (uint32_t*)recv_buffer;
		if (OCR_CCS_FLAG(*ocr) == 1) {
			/* Card is already using block addressing. */
			spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
			spi_sd_mdriver->card_ready = 1;
			trace_printf("SDv2.0 mounted\n");
			return SPI_SD_OK;
		}
		break;
	case SPI_SD_CARD_SD1:
		/**
		 * Put the card into the ready state.
		 * Fall back to MMC initialization if not successful.
		 **/
		if (spi_sd_acmd41_loop(spi_sd_mdriver, 0x00000000, 64) == SPI_SD_OK) {
			/* Card is SDv1.0 */
			break;
		}
		/* Fall through and attempt MMC initialization. */
		//no break
	case SPI_SD_CARD_MMC3:
		spi_sd_mdriver->card_type = SPI_SD_CARD_MMC3;
		/**
		 * TODO Not implemented yet.
		 */
		spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
		return SPI_SD_FAIL;
	default:
		/**
		 * Unknown card type.
		 **/
		spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
		return SPI_SD_FAIL;
	}

	/**
	 * Send the SET_BLOCKLEN (CMD16) command with 512 byte blocks.
	 * Wait for the response token.
	 * Recovery time after command.
	 */
	make_command(command, 16, 0x00000200, 0xFF);
	if (   spi_sd_transmit_bytes(hspi, command, 6)                 != SPI_SD_OK
		|| spi_sd_receive_token(hspi, pred_res_ready, &token, 255) != SPI_SD_OK
		|| spi_sd_command_recover(spi_sd_mdriver)                  != SPI_SD_OK) {
		spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
		return SPI_SD_FAIL;
	}

	spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
	spi_sd_mdriver->card_ready = 1;
	trace_printf("SDv1.0 mounted\n");
	return SPI_SD_OK;
}

/**
 * Get the card capacity in sectors.
 */
uint8_t
spi_sd_get_capacity(MMC_SD_MDriver* spi_sd_mdriver, uint32_t* capacity) {
	uint8_t command[6], token;

	SPI_HandleTypeDef* hspi = spi_sd_mdriver->hspi;

	spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_SELECT);

	/**
	 * Send the GET_CSD command.
	 * Wait for the response token.
	 * Wait for the data start token.
	 * Receive the CSD register data and CRC bytes.
	 * Recovery time after command.
	 **/
	make_command(command, 9, 0x00000000, 0xFF);
	if (   spi_sd_transmit_bytes(hspi, command, 6)                    != SPI_SD_OK
		|| spi_sd_receive_token(hspi, pred_res_ready, &token, 255)    != SPI_SD_OK
		|| spi_sd_receive_token(hspi, pred_data_start, &token, 16384) != SPI_SD_OK
		|| spi_sd_receive_bytes_ff(hspi, recv_buffer, 18)             != SPI_SD_OK
		|| spi_sd_command_recover(spi_sd_mdriver)                     != SPI_SD_OK) {
		spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
		return SPI_SD_FAIL;
	}

	spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);

	/* Determine the capacity of the card in sectors. */
	if (CSD_VERSION(*recv_buffer) == CSD_V2) {
		*capacity = CSD_V2_C_SIZE(recv_buffer) * 1024;
		return SPI_SD_OK;
	} else
	if (CSD_VERSION(*recv_buffer) == CSD_V1) {
		/* Calculate device capacity in bytes. */
		*capacity = (1 << (CSD_V1_C_SIZE_MULT(recv_buffer) + 2));
		*capacity = *capacity * (CSD_V1_C_SIZE(recv_buffer) + 1);
		*capacity = *capacity * (1 << CSD_V1_READ_BL_LEN(recv_buffer));
		/* Divide by sector size. */
		*capacity = *capacity / 512;
		return SPI_SD_OK;
	} else {
		// Not a known CSD format.
		*capacity = 0;
		return SPI_SD_FAIL;
	}
}

/**
 * Read one sector of data from the card.
 */
uint8_t
spi_sd_read_sector(MMC_SD_MDriver* spi_sd_mdriver, uint32_t sector, uint8_t* data) {
	uint8_t command[6], token;

	SPI_HandleTypeDef* hspi = spi_sd_mdriver->hspi;

	spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_SELECT);

	/* Calculate the sector start address if this card uses byte addressing. */
	if (spi_sd_mdriver->card_type != SPI_SD_CARD_SD2) {
		sector = sector * 512;
	}

	/**
	 * Send the READ_BLOCK command.
	 * Wait for the response token.
	 * Wait for the data start token.
	 * Receive one sector of data.
	 * Discard the CRC bytes.
	 * Recovery time after command.
	 **/
	make_command(command, 17, sector, 0xFF);
	if (   spi_sd_transmit_bytes(hspi, command, 6)                    != SPI_SD_OK /* CMD17 */
		|| spi_sd_receive_token(hspi, pred_res_ready, &token, 255)    != SPI_SD_OK /* Command response */
		|| spi_sd_receive_token(hspi, pred_data_start, &token, 16384) != SPI_SD_OK /* Data start token */
		|| spi_sd_receive_bytes_ff(hspi, data, 512)                   != SPI_SD_OK /* Data*/
		|| spi_sd_transmit_bytes(hspi, recv_buffer, 2)                != SPI_SD_OK /* CRC */
		|| spi_sd_command_recover(spi_sd_mdriver)                     != SPI_SD_OK) {
		spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
		return SPI_SD_FAIL;
	}

	spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
	return SPI_SD_OK;
}

/**
 * Write one sector of data on the card.
 */
uint8_t
spi_sd_write_sector(MMC_SD_MDriver* spi_sd_mdriver, uint32_t sector, uint8_t* data) {
	uint8_t command[6], token;

	SPI_HandleTypeDef* hspi = spi_sd_mdriver->hspi;

	spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_SELECT);

	/* Calculate the sector start address if this card uses byte addressing. */
	if (spi_sd_mdriver->card_type != SPI_SD_CARD_SD2) {
		sector = sector * 512;
	}

	/**
	 * Send the WRITE_BLOCK command.
	 * Wait for the response token.
	 * Send 8 idle clocks.
	 * Send the data start token.
	 * Send the data packet containing the sector.
	 * Send the CRC bytes.
	 * Wait for the data response token.
	 * Wait for the card to be ready.
	 * Recovery time after command.
	 */
	make_command(command, 24, sector, 0xFF);
	uint8_t data_start = 0xFE;
	if (   spi_sd_transmit_bytes(hspi, command, 6)                  != SPI_SD_OK /* CMD24 */
		|| spi_sd_receive_token(hspi, pred_res_ready, &token, 255)  != SPI_SD_OK /* Command response */
		|| spi_sd_transmit_bytes(hspi, ffff_buffer, 1)              != SPI_SD_OK /* Idle byte */
		|| spi_sd_transmit_bytes(hspi, &data_start, 1)              != SPI_SD_OK /* Data start token */
		|| spi_sd_transmit_bytes(hspi, data, 512)                   != SPI_SD_OK /* Data */
		|| spi_sd_transmit_bytes(hspi, ffff_buffer, 2)              != SPI_SD_OK /* CRC */
		|| spi_sd_receive_token(hspi, pred_data_ok, &token, 255)    != SPI_SD_OK /* Data response*/
		|| spi_sd_receive_token(hspi, pred_not_busy, &token, 65535) != SPI_SD_OK /* Wait until ready*/
		|| spi_sd_command_recover(spi_sd_mdriver)                   != SPI_SD_OK) {
		spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
		return SPI_SD_FAIL;
	}

	spi_sd_assert_ss(spi_sd_mdriver, SPI_SD_RELEASE);
	return SPI_SD_OK;
}

/**
 * Attempt to mount and initialize a SD card.
 * Does nothing if a card is currently mounted.
 */
uint8_t
spi_sd_mount_card(MMC_SD_MDriver* spi_sd_mdriver) {
	if (!spi_sd_mdriver->card_ready) {
		if (spi_sd_init_card(spi_sd_mdriver) != SPI_SD_OK) {
			return F_ST_MISSING;
		} else {
			spi_sd_mdriver->card_ready = 1;
		}
	}
	return 0;
}

/**
 * MDriver read sector implementation.
 */
static int
spi_sd_readsector ( F_DRIVER * driver, void * data, unsigned long sector )
{
	MMC_SD_MDriver* spi_sd_mdriver = driver->user_ptr;

	if (spi_sd_mount_card(spi_sd_mdriver) != 0) {
		// Card is not and could not be mounted.
		return F_ST_MISSING;
	}

	if (spi_sd_read_sector(spi_sd_mdriver, sector, data) != SPI_SD_OK) {
		// Read sector command failed.
		spi_sd_mdriver->card_ready = 0;
		trace_printf("SD: Read %d Failed\n", sector);
		return 1;
	}

	return 0;
}

/**
 * MDriver write sector implementation.
 */
static int
spi_sd_writesector ( F_DRIVER * driver, void * data, unsigned long sector )
{
	MMC_SD_MDriver* spi_sd_mdriver = driver->user_ptr;

	if (spi_sd_mount_card(spi_sd_mdriver) != 0) {
		// Card is not and could not be mounted.
		return F_ST_MISSING;
	}

	if (spi_sd_write_sector(spi_sd_mdriver, sector, data) != SPI_SD_OK) {
		// Write sector command failed.
		spi_sd_mdriver->card_ready = 0;
		trace_printf("SD: Write %d Failed\n", sector);
		return 1;
	}

	return 0;
}

/**
 * MDriver get status implementation.
 */
static long
spi_sd_getstatus ( F_DRIVER * driver )
{
	MMC_SD_MDriver* spi_sd_mdriver = driver->user_ptr;

	if (spi_sd_mount_card(spi_sd_mdriver) > 0) {
		return F_ST_MISSING;
	}

	return 0;
}

/**
 * MDriver get physical information implementation.
 */
static int
spi_sd_getphy ( F_DRIVER * driver, F_PHY * phy )
{
	MMC_SD_MDriver* spi_sd_mdriver = driver->user_ptr;

	if (spi_sd_mount_card(spi_sd_mdriver) != 0) {
		// Card is not and could not be mounted.
		return F_ST_MISSING;
	}

	phy->bytes_per_sector = 512;

	if (spi_sd_get_capacity(spi_sd_mdriver, &phy->number_of_sectors) != SPI_SD_OK) {
		// Failed to retrieve card information.
		spi_sd_mdriver->card_ready = 0;
		return F_ST_MISSING;
	}

	return 0;
}

/**
 * MDriver release implementation.
 */
static void
spi_sd_release ( F_DRIVER * driver )
{
	/* Not used. */
	( void ) driver;
}

/**
 * MDriver initialize implementation.
 */
F_DRIVER *
mmc_spi_initfunc ( unsigned long driver_param )
{
	__HAL_RCC_SPI1_CLK_ENABLE();
	hspi_sdcard.Instance = SPI1;
	hspi_sdcard.State = HAL_SPI_STATE_RESET;
	hspi_sdcard.Init.Mode = SPI_MODE_MASTER;
	hspi_sdcard.Init.Direction = SPI_DIRECTION_2LINES;
	hspi_sdcard.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi_sdcard.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi_sdcard.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi_sdcard.Init.NSS = SPI_NSS_SOFT;
	hspi_sdcard.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
	hspi_sdcard.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi_sdcard.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi_sdcard.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi_sdcard.Init.CRCPolynomial = 1;
	HAL_SPI_Init(&hspi_sdcard);

	// SPI SD MDriver settings definition
	spi_sd_mdriver.card_ready = 0;
	spi_sd_mdriver.hspi = &hspi_sdcard;
	spi_sd_mdriver.ss_gpio_port = GPIOB;
	spi_sd_mdriver.ss_gpio_pin = GPIO_PIN_10;

	// MDriver interface definition
	t_driver.user_ptr = &spi_sd_mdriver;
	t_driver.readsector = spi_sd_readsector;
	t_driver.writesector = spi_sd_writesector;
	t_driver.getphy = spi_sd_getphy;
	t_driver.getstatus = spi_sd_getstatus;
	t_driver.release = spi_sd_release;

	return &t_driver;
}

