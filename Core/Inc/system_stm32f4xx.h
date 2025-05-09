 /**
  ******************************************************************************
  * @file    system_stm32f4xx.h
  * @author  Based on STM32F411 Audio Crossover Project
  * @brief   CMSIS Cortex-M4 Device System Source File for STM32F4xx devices.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 Audio Crossover Project.
  * All rights reserved.
  *
  * The SystemInit() function defined in this file configures the system clock 
  * (PLL, AHB and APB) before branch to main program. This file primarily 
  * contains the initialization for the STM32F411 Cortex-M4 system.
  ******************************************************************************
  */

#ifndef __SYSTEM_STM32F4XX_H
#define __SYSTEM_STM32F4XX_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/**
  * @brief Define the HSE crystal frequency used in the board
  */
#if !defined  (HSE_VALUE) 
  #define HSE_VALUE    ((uint32_t)8000000) /*!< Default value of the External oscillator in Hz */
#endif /* HSE_VALUE */

/**
  * @brief Define the HSI (Internal High Speed clock) value
  */
#if !defined  (HSI_VALUE)
  #define HSI_VALUE    ((uint32_t)16000000) /*!< Value of the Internal oscillator in Hz*/
#endif /* HSI_VALUE */

/**
  * @brief External Low Speed oscillator (LSE) value
  */
#if !defined  (LSE_VALUE)
  #define LSE_VALUE  ((uint32_t)32768)    /*!< Value of the External Low Speed oscillator in Hz */
#endif /* LSE_VALUE */

/**
  * @brief Internal Low Speed oscillator (LSI) value
  */
#if !defined  (LSI_VALUE)
 #define LSI_VALUE  ((uint32_t)32000)      /*!< LSI Typical Value in Hz*/
#endif /* LSI_VALUE */

/**
  * @brief External clock source for I2S peripheral
  *        This value is used by the I2S HAL module to compute the I2S clock source 
  *        frequency, this source is inserted directly through I2S_CKIN pad. 
  */
#if !defined  (EXTERNAL_CLOCK_VALUE)
  #define EXTERNAL_CLOCK_VALUE    ((uint32_t)12288000) /*!< Value of the External audio clock in Hz*/
#endif /* EXTERNAL_CLOCK_VALUE */

/* Exported types ------------------------------------------------------------*/
extern uint32_t SystemCoreClock;          /*!< System Clock Frequency (Core Clock) */
extern const uint8_t  AHBPrescTable[16];  /*!< AHB prescalers table values */
extern const uint8_t  APBPrescTable[8];   /*!< APB prescalers table values */

/* Exported functions --------------------------------------------------------*/
/**
  * @brief Setup the microcontroller system
  *        Initialize the FPU setting, vector table location and the PLL configuration
  * @param  None
  * @retval None
  */
extern void SystemInit(void);

/**
  * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock (HCLK), it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  * @note   Each time the core clock (HCLK) changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.         
  * @param  None
  * @retval None
  */
extern void SystemCoreClockUpdate(void);

/**
  * @brief  Setup the external memory controller.
  *         Called in startup_stm32f4xx.s before jump to main.
  *         This function configures the external memories (SRAM/SDRAM/NOR/NAND/FLASH)
  *         This is optional and may be skipped for STM32F411 which doesn't
  *         require external memory setup.
  * @param  None
  * @retval None
  */
extern void SystemInit_ExtMemCtl(void);

#ifdef __cplusplus
}
#endif

#endif /*__SYSTEM_STM32F4XX_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
