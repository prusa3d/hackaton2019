// stm32f4xx_hal.c (fake source for simulator)

#include <stm32f4xx_hal.h>

////////////////////////////////////////////////////////////////////////////////
// SPI

HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size, uint32_t Timeout)
{
}

HAL_StatusTypeDef HAL_SPI_Transmit_DMA(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size)
{
}

////////////////////////////////////////////////////////////////////////////////
// SCB

SCB_Type _SCB;
SCB_Type* SCB = &_SCB;

////////////////////////////////////////////////////////////////////////////////
// SysTick

SysTick_Type _SysTick;
SysTick_Type* SysTick = &_SysTick;
