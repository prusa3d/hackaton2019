// stm32f4xx_hal.h (fake header for simulator)


#ifndef _STM32F4XX_HAL_H
#define _STM32F4XX_HAL_H

#include <inttypes.h>

#define HAL_MAX_DELAY      0xFFFFFFFFU


////////////////////////////////////////////////////////////////////////////////
// GPIO

#define  GPIO_MODE_INPUT                        0x00000000U   /*!< Input Floating Mode                   */

#define  GPIO_SPEED_FREQ_LOW         0x00000000U  /*!< IO works at 2 MHz, please refer to the product datasheet */
#define  GPIO_SPEED_FREQ_MEDIUM      0x00000001U  /*!< range 12,5 MHz to 50 MHz, please refer to the product datasheet */
#define  GPIO_SPEED_FREQ_HIGH        0x00000002U  /*!< range 25 MHz to 100 MHz, please refer to the product datasheet  */
#define  GPIO_SPEED_FREQ_VERY_HIGH   0x00000003U  /*!< range 50 MHz to 200 MHz, please refer to the product datasheet  */

#define  GPIO_MODE_OUTPUT_PP                    0x00000001U   /*!< Output Push Pull Mode                 */
#define  GPIO_MODE_OUTPUT_OD                    0x00000011U   /*!< Output Open Drain Mode                */

#define  GPIO_NOPULL        0x00000000U   /*!< No Pull-up or Pull-down activation  */
#define  GPIO_PULLUP        0x00000001U   /*!< Pull-up activation                  */
#define  GPIO_PULLDOWN      0x00000002U   /*!< Pull-down activation                */

typedef struct
{
	void* dummy;
} GPIO_TypeDef;

typedef enum
{
  HAL_OK       = 0x00U,
  HAL_ERROR    = 0x01U,
  HAL_BUSY     = 0x02U,
  HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

////////////////////////////////////////////////////////////////////////////////
// SPI

typedef struct __SPI_HandleTypeDef
{
	void* dummy;
} SPI_HandleTypeDef;

extern HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);

extern HAL_StatusTypeDef HAL_SPI_Transmit_DMA(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size);


////////////////////////////////////////////////////////////////////////////////
// SCB

typedef struct
{
	uint32_t ICSR;
} SCB_Type;


#define SCB_ICSR_VECTACTIVE_Msk            (0x1FFUL /*<< SCB_ICSR_VECTACTIVE_Pos*/)       /*!< SCB ICSR: VECTACTIVE Mask */

extern SCB_Type* SCB;


////////////////////////////////////////////////////////////////////////////////
// SysTick

typedef struct
{
	uint32_t CTRL;
} SysTick_Type;

extern SysTick_Type* SysTick;


#endif // _STM32F4XX_HAL_H
