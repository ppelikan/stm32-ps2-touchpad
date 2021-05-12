// PS/2 Driver for the STM32 platform
//
// This driver uses mixed SPI slave Rx only in interrupt mode
// with GPIO signals to implement PS/2 protocol communication.
// This has been tested on the STM32F769 microcontroller
// at SYSCLK = HCLK = 200MHz
//
// Copyright (c) 2019 by ppelikan
// github.com/ppelikan

#ifndef __PS2_H__
#define __PS2_H__

#include <stdbool.h>
#include "stm32f7xx_hal.h"

// user platform specific adaptation, change your setup here
// please remember to declare the SPI handle (SPI_HandleTypeDef hspiX) and
// call the HAL_SPI_IRQHandler(&hspiX) from your SPIx_IRQHandler() function
#define PS2_CLK_Pin                GPIO_PIN_12
#define PS2_CLK_GPIO_Port          GPIOA
#define PS2_DATA_Pin               GPIO_PIN_15
#define PS2_DATA_GPIO_Port         GPIOB
#define PS2_CLK_AF                 GPIO_AF5_SPI2
#define PS2_DATA_AF                GPIO_AF5_SPI2
#define PS2_SPI_HANDLE             hspi2
#define PS2_SPI                    SPI2
#define PS2_SPI_IRQn               SPI2_IRQn
#define PS2_ENABLE_ALL_CLOCKS     \
                                __HAL_RCC_SPI2_CLK_ENABLE();  \
                                __HAL_RCC_GPIOA_CLK_ENABLE(); \
                                __HAL_RCC_GPIOB_CLK_ENABLE();
#define PS2_SPI_CLOCK_DISABLE   __HAL_RCC_SPI2_CLK_DISABLE();

// driver works best if the value is multiple of 6
#define RX_FIFO_SIZE 24

bool    ps2_readByte(uint8_t *byte); 
void    ps2_sendByte(uint8_t byte);
bool    ps2_isDataAvaiable(uint8_t n_bytes);
void    ps2_scheduleRx();
bool    ps2_getACK();

#endif
