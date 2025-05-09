 /**
  ******************************************************************************
  * @file    stm32f4xx_it.h
  * @brief   This file contains the headers of the interrupt handlers.
  * @author  Based on STM32F411 Audio Crossover Project
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 Audio Crossover Project.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/

/* Non-maskable interrupt handler */
void NMI_Handler(void);

/* Hard fault interrupt handler */
void HardFault_Handler(void);

/* Memory management fault interrupt handler */
void MemManage_Handler(void);

/* Bus fault interrupt handler */
void BusFault_Handler(void);

/* Usage fault interrupt handler */
void UsageFault_Handler(void);

/* SVCall interrupt handler */
void SVC_Handler(void);

/* Debug monitor interrupt handler */
void DebugMon_Handler(void);

/* PendSV interrupt handler */
void PendSV_Handler(void);

/* SysTick interrupt handler */
void SysTick_Handler(void);

/* DMA1 Stream3 interrupt handler - used for SPI2_RX (I2S2_RX) */
void DMA1_Stream3_IRQHandler(void);

/* DMA1 Stream5 interrupt handler - used for SPI3_TX (I2S3_TX) */
void DMA1_Stream5_IRQHandler(void);

/* I2C1 event interrupt handler */
void I2C1_EV_IRQHandler(void);

/* I2C1 error interrupt handler */
void I2C1_ER_IRQHandler(void);

/* SPI2 interrupt handler (I2S2 audio input) */
void SPI2_IRQHandler(void);

/* SPI3 interrupt handler (I2S3 audio output) */
void SPI3_IRQHandler(void);

/* USART1 interrupt handler */
void USART1_IRQHandler(void);

/* EXTI Line 0 interrupt handler - used for buttons/input */
void EXTI0_IRQHandler(void);

/* EXTI Line 1 interrupt handler - used for buttons/input */
void EXTI1_IRQHandler(void);

/* EXTI Line 2 interrupt handler - used for rotary encoder */
void EXTI2_IRQHandler(void);

/* EXTI Line 3 interrupt handler - used for rotary encoder */
void EXTI3_IRQHandler(void);

/* EXTI Line 4 interrupt handler - used for buttons/input */
void EXTI4_IRQHandler(void);

/* EXTI Lines 5-10 interrupt handler */
void EXTI9_5_IRQHandler(void);

/* EXTI Lines 10-15 interrupt handler */
void EXTI15_10_IRQHandler(void);

/* TIM2 global interrupt handler - system tick for UI refresh */
void TIM2_IRQHandler(void);

/* TIM3 global interrupt handler - used for encoder sampling */
void TIM3_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_IT_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
