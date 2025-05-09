 /**
  ******************************************************************************
  * @file           : gpio_config.h
  * @brief          : GPIO configuration and management for Audio Crossover DSP
  * @author         : Audio Crossover Project
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
#ifndef __GPIO_CONFIG_H
#define __GPIO_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief GPIO pin state enumeration
  */
typedef enum {
  GPIO_PIN_RESET = 0,
  GPIO_PIN_SET,
  GPIO_PIN_TOGGLE
} GPIO_PinState_t;

/**
  * @brief Push button enumeration
  */
typedef enum {
  BUTTON_MENU = 0,
  BUTTON_BACK,
  BUTTON_ENCODER,  /* Rotary encoder push button */
  BUTTON_PRESET1,
  BUTTON_PRESET2,
  BUTTON_MUTE,
  BUTTON_COUNT     /* Total number of buttons */
} Button_t;

/**
  * @brief GPIO LED enumeration
  */
typedef enum {
  LED_STATUS = 0,
  LED_ERROR,
  LED_CLIP,
  LED_SUB_ACTIVE,
  LED_LOW_ACTIVE,
  LED_MID_ACTIVE,
  LED_HIGH_ACTIVE,
  LED_COUNT       /* Total number of LEDs */
} Led_t;

/**
  * @brief Encoder channel enumeration
  */
typedef enum {
  ENCODER_A = 0,
  ENCODER_B
} EncoderChannel_t;

/* Exported constants --------------------------------------------------------*/
/* Default debounce time in ms */
#define GPIO_DEBOUNCE_TIME 20

/* Button active state (LOW for pullup configuration) */
#define BUTTON_ACTIVE_STATE GPIO_PIN_RESET

/* LED active state (HIGH for common cathode or direct drive) */
#define LED_ACTIVE_STATE GPIO_PIN_SET

/* Exported macro ------------------------------------------------------------*/
/* Simple pin manipulation macros */
#define GPIO_SET_PIN(port, pin)    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET)
#define GPIO_RESET_PIN(port, pin)  HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET)
#define GPIO_TOGGLE_PIN(port, pin) HAL_GPIO_TogglePin(port, pin)
#define GPIO_READ_PIN(port, pin)   HAL_GPIO_ReadPin(port, pin)

/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  Initialize all GPIO pins used in the application
  * @retval None
  */
void GPIO_Config_Init(void);

/**
  * @brief  Set the state of an LED
  * @param  led: The LED to control
  * @param  state: The desired state (GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_TOGGLE)
  * @retval None
  */
void GPIO_SetLed(Led_t led, GPIO_PinState_t state);

/**
  * @brief  Read the state of a button
  * @param  button: The button to read
  * @retval GPIO_PinState: The current state of the button (active state is defined by BUTTON_ACTIVE_STATE)
  */
GPIO_PinState GPIO_ReadButton(Button_t button);

/**
  * @brief  Read the state of an encoder channel
  * @param  channel: The encoder channel to read (ENCODER_A or ENCODER_B)
  * @retval GPIO_PinState: The current state of the encoder channel
  */
GPIO_PinState GPIO_ReadEncoderChannel(EncoderChannel_t channel);

/**
  * @brief  Configure all peripheral GPIOs (I2S, I2C, SPI)
  * @retval None
  */
void GPIO_ConfigurePeripherals(void);

/**
  * @brief  Update all LED indicators based on current system state
  * @param  systemSettings: Pointer to the current system settings
  * @retval None
  */
void GPIO_UpdateLedIndicators(SystemSettings_t *systemSettings);

#ifdef __cplusplus
}
#endif

#endif /* __GPIO_CONFIG_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
