/**
  ******************************************************************************
  * @file    stm32f4xx_hal_msp_template.c
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    26-December-2014
  * @brief   This file contains the HAL System and Peripheral (PPP) MSP initialization
  *          and de-initialization functions.
  *          It should be copied to the application folder and renamed into 'stm32f4xx_hal_msp.c'.           
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

#include "diag/Trace.h"

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_conf.h"

/** @addtogroup STM32F4xx_HAL_Driver
  * @{
  */

/** @defgroup HAL_MSP HAL MSP
  * @brief HAL MSP module.
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup HAL_MSP_Private_Functions HAL MSP Private Functions
  * @{
  */

void
HAL_MspInit(void) {
	GPIO_InitTypeDef gpio_init;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* Configure the pin that controls the onboard LED
	   PA5 (GPIO).
	   D13 = PA5 */
	gpio_init.Pin = GPIO_PIN_5;
	gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_init.Speed = GPIO_SPEED_HIGH;
	gpio_init.Pull = GPIO_NOPULL;
	gpio_init.Alternate = GPIO_AF0_MCO;
	HAL_GPIO_Init(GPIOA, &gpio_init);
}

/**
  * @brief  DeInitializes the Global MSP.
  * @note   This functiona is called from HAL_DeInit() function to perform system
  *         level de-initialization (GPIOs, clock, DMA, interrupt).
  * @retval None
  */
void
HAL_MspDeInit(void) {

}

// [ILG]
#if defined ( __GNUC__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#endif

void
HAL_SPI_MspInit(SPI_HandleTypeDef* hspi) {
	GPIO_InitTypeDef gpio_init;

	/* Configure SPI1 to interface with the SD shield. */
	if (hspi->Instance == SPI1) {
		/* PB10 (GPIO - Chip Select)
		   Arduino header: D6 */
		gpio_init.Pin = GPIO_PIN_10;
		gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
		gpio_init.Speed = GPIO_SPEED_HIGH;
		gpio_init.Pull = GPIO_NOPULL;
		gpio_init.Alternate = GPIO_AF0_MCO;
		HAL_GPIO_Init(GPIOB, &gpio_init);

		/* PB3 (SPI1_SCK)
		   Arduino header: D3 */
		gpio_init.Pin = GPIO_PIN_3;
		gpio_init.Mode = GPIO_MODE_AF_PP;
		gpio_init.Speed = GPIO_SPEED_HIGH;
		gpio_init.Pull = GPIO_NOPULL;
		gpio_init.Alternate = GPIO_AF5_SPI1;
		HAL_GPIO_Init(GPIOB, &gpio_init);

		/* PB5 (SPI1_MOSI)
		   Arduino header: D4 */
		gpio_init.Pin = GPIO_PIN_5;
		gpio_init.Mode = GPIO_MODE_AF_PP;
		gpio_init.Speed = GPIO_SPEED_HIGH;
		gpio_init.Pull = GPIO_NOPULL;
		gpio_init.Alternate = GPIO_AF5_SPI1;
		HAL_GPIO_Init(GPIOB, &gpio_init);

		/* PB4 (SPI1_MISO)
		   Arduino header: D5 */
		gpio_init.Pin = GPIO_PIN_4;
		gpio_init.Mode = GPIO_MODE_AF_PP;
		gpio_init.Speed = GPIO_SPEED_HIGH;
		gpio_init.Pull = GPIO_PULLUP;
		gpio_init.Alternate = GPIO_AF5_SPI1;
		HAL_GPIO_Init(GPIOB, &gpio_init);
	}
}

void
HAL_UART_MspInit(UART_HandleTypeDef* huart) {
	GPIO_InitTypeDef gpio_init;

	/**
	 * Note that if this is invoked during HAL_MspInit(), the system clock
	 * will be configured at 16MHz. This will cause the UART driver to run
	 * at a higher baud rate once the clock is increased to 84MHz. Invoke
	 * HAL_UART_Init() after HAL setup to avoid this problem.
	 */

	/* Configure UART1 to interface with the Skywire cellular modem. */
	if (huart->Instance == USART1) {
		/* PA9 (USART1_TX)
		   Arduino header: D8/TX
		   Skywire header: IO0/IO2 (MCU_RX) */
		gpio_init.Pin = GPIO_PIN_9;
		gpio_init.Mode = GPIO_MODE_AF_PP;
		gpio_init.Speed = GPIO_SPEED_HIGH;
		gpio_init.Pull = GPIO_NOPULL;
		gpio_init.Alternate = GPIO_AF7_USART1;
		HAL_GPIO_Init(GPIOA, &gpio_init);

		/* PA10 (USART1_RX)
		   Arduino header: D2/RX
		   Skywire header: IO1/IO8 (MCU_TX) */
		gpio_init.Pin = GPIO_PIN_10;
		gpio_init.Mode = GPIO_MODE_AF_PP;
		gpio_init.Speed = GPIO_SPEED_HIGH;
		gpio_init.Pull = GPIO_NOPULL;
		gpio_init.Alternate = GPIO_AF7_USART1;
		HAL_GPIO_Init(GPIOA, &gpio_init);

		/* PA6 (GPIO)
		   Arduino header: MISO/D12
		   Skywire header: IO12 (ON_OFF) */
		gpio_init.Pin = GPIO_PIN_6;
		gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
		gpio_init.Speed = GPIO_SPEED_HIGH;
		gpio_init.Pull = GPIO_NOPULL;
		gpio_init.Alternate = GPIO_AF0_MCO;
		HAL_GPIO_Init(GPIOA, &gpio_init);

		/* PA7 (GPIO)
		   Arduino header: MOSI/D11
		   Skywire header: IO11 (RTS) */
		gpio_init.Pin = GPIO_PIN_7;
		gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
		gpio_init.Speed = GPIO_SPEED_HIGH;
		gpio_init.Pull = GPIO_NOPULL;
		gpio_init.Alternate = GPIO_AF0_MCO;
		HAL_GPIO_Init(GPIOA, &gpio_init);
	}

	/* Configure UART2 to interface with the ST-LINK virtual COM port. */
	if (huart->Instance == USART2) {
		/* PA2 (USART2_TX)
		   Arduino header: D1 */
		gpio_init.Pin = GPIO_PIN_2;
		gpio_init.Mode = GPIO_MODE_AF_PP;
		gpio_init.Speed = GPIO_SPEED_HIGH;
		gpio_init.Pull = GPIO_NOPULL;
		gpio_init.Alternate = GPIO_AF7_USART2;
		HAL_GPIO_Init(GPIOA, &gpio_init);

		/* PA3 (USART2_RX)
		   Arduino header: D0 */
		gpio_init.Pin = GPIO_PIN_3;
		gpio_init.Mode = GPIO_MODE_AF_PP;
		gpio_init.Speed = GPIO_SPEED_HIGH;
		gpio_init.Pull = GPIO_NOPULL;
		gpio_init.Alternate = GPIO_AF7_USART2;
		HAL_GPIO_Init(GPIOA, &gpio_init);
	}
}

void
HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c) {
	GPIO_InitTypeDef gpio_init;

	/* Configure I2C1 to interface with sensors on the Skywire shield. */
	if (hi2c->Instance == I2C1) {
		/* PB8 (I2C1_SCL)
		   Arduino header: D15 */
		gpio_init.Pin = GPIO_PIN_8;
		gpio_init.Mode = GPIO_MODE_AF_OD;
		gpio_init.Speed = GPIO_SPEED_HIGH;
		gpio_init.Pull = GPIO_PULLUP;
		gpio_init.Alternate = GPIO_AF4_I2C1;
		HAL_GPIO_Init(GPIOB, &gpio_init);

		/* PB9 (I2C1_SDA)
		   Arduino header: D14 */
		gpio_init.Pin = GPIO_PIN_9;
		gpio_init.Mode = GPIO_MODE_AF_OD;
		gpio_init.Speed = GPIO_SPEED_HIGH;
		gpio_init.Pull = GPIO_PULLUP;
		gpio_init.Alternate = GPIO_AF4_I2C1;
		HAL_GPIO_Init(GPIOB, &gpio_init);
	}
}

/**
  * @brief  Initializes the PPP MSP.
  * @note   This functiona is called from HAL_PPP_Init() function to perform
  *         peripheral(PPP) system level initialization (GPIOs, clock, DMA, interrupt)
  * @retval None
  */
void
HAL_PPP_MspInit(void)
{

}

/**
  * @brief  DeInitializes the PPP MSP.
  * @note   This functiona is called from HAL_PPP_DeInit() function to perform 
  *         peripheral(PPP) system level de-initialization (GPIOs, clock, DMA, interrupt)
  * @retval None
  */
void
HAL_PPP_MspDeInit(void)
{

}

// [ILG]
#if defined ( __GNUC__ )
#pragma GCC diagnostic pop
#endif

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
