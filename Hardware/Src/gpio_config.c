 /**
  ******************************************************************************
  * @file           : gpio_config.c
  * @brief          : GPIO configuration and management implementation
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

/* Includes ------------------------------------------------------------------*/
#include "gpio_config.h"

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief GPIO pin definition structure
  */
typedef struct {
  GPIO_TypeDef* port;
  uint16_t pin;
} GPIO_Pin_t;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* LED pin definitions */
static const GPIO_Pin_t ledPins[LED_COUNT] = {
  {GPIOC, GPIO_PIN_13},  /* LED_STATUS - Uses the built-in LED on most STM32F411 boards */
  {GPIOB, GPIO_PIN_12},  /* LED_ERROR */
  {GPIOB, GPIO_PIN_13},  /* LED_CLIP */
  {GPIOB, GPIO_PIN_14},  /* LED_SUB_ACTIVE */
  {GPIOB, GPIO_PIN_15},  /* LED_LOW_ACTIVE */
  {GPIOA, GPIO_PIN_8},   /* LED_MID_ACTIVE */
  {GPIOA, GPIO_PIN_9}    /* LED_HIGH_ACTIVE */
};

/* Button pin definitions */
static const GPIO_Pin_t buttonPins[BUTTON_COUNT] = {
  {GPIOA, GPIO_PIN_0},   /* BUTTON_MENU */
  {GPIOA, GPIO_PIN_1},   /* BUTTON_BACK */
  {GPIOA, GPIO_PIN_2},   /* BUTTON_ENCODER (Push button on rotary encoder) */
  {GPIOA, GPIO_PIN_3},   /* BUTTON_PRESET1 */
  {GPIOA, GPIO_PIN_4},   /* BUTTON_PRESET2 */
  {GPIOA, GPIO_PIN_5}    /* BUTTON_MUTE */
};

/* Encoder channel pin definitions */
static const GPIO_Pin_t encoderPins[2] = {
  {GPIOA, GPIO_PIN_6},   /* ENCODER_A */
  {GPIOA, GPIO_PIN_7}    /* ENCODER_B */
};

/* Private function prototypes -----------------------------------------------*/
static void GPIO_ConfigureUserInputs(void);
static void GPIO_ConfigureOutputs(void);

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Initialize all GPIO pins used in the application
  * @retval None
  */
void GPIO_Config_Init(void)
{
  /* Configure user input pins (buttons and encoder) */
  GPIO_ConfigureUserInputs();
  
  /* Configure output pins (LEDs) */
  GPIO_ConfigureOutputs();
  
  /* Initial LED states */
  GPIO_SetLed(LED_STATUS, GPIO_PIN_SET);     /* Status LED on */
  GPIO_SetLed(LED_ERROR, GPIO_PIN_RESET);    /* Error LED off */
  GPIO_SetLed(LED_CLIP, GPIO_PIN_RESET);     /* Clip LED off */
  GPIO_SetLed(LED_SUB_ACTIVE, GPIO_PIN_SET); /* Sub band active */
  GPIO_SetLed(LED_LOW_ACTIVE, GPIO_PIN_SET); /* Low band active */
  GPIO_SetLed(LED_MID_ACTIVE, GPIO_PIN_SET); /* Mid band active */
  GPIO_SetLed(LED_HIGH_ACTIVE, GPIO_PIN_SET);/* High band active */
  
  #ifdef DEBUG
  printf("GPIO configuration complete\r\n");
  #endif
}

/**
  * @brief  Set the state of an LED
  * @param  led: The LED to control
  * @param  state: The desired state (GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_TOGGLE)
  * @retval None
  */
void GPIO_SetLed(Led_t led, GPIO_PinState_t state)
{
  if (led >= LED_COUNT) {
    return;  /* Invalid LED index */
  }
  
  switch (state) {
    case GPIO_PIN_SET:
      HAL_GPIO_WritePin(ledPins[led].port, ledPins[led].pin, GPIO_PIN_SET);
      break;
    
    case GPIO_PIN_RESET:
      HAL_GPIO_WritePin(ledPins[led].port, ledPins[led].pin, GPIO_PIN_RESET);
      break;
    
    case GPIO_PIN_TOGGLE:
      HAL_GPIO_TogglePin(ledPins[led].port, ledPins[led].pin);
      break;
    
    default:
      break;
  }
}

/**
  * @brief  Read the state of a button
  * @param  button: The button to read
  * @retval GPIO_PinState: The current state of the button
  */
GPIO_PinState GPIO_ReadButton(Button_t button)
{
  if (button >= BUTTON_COUNT) {
    return GPIO_PIN_RESET;  /* Invalid button index */
  }
  
  return HAL_GPIO_ReadPin(buttonPins[button].port, buttonPins[button].pin);
}

/**
  * @brief  Read the state of an encoder channel
  * @param  channel: The encoder channel to read (ENCODER_A or ENCODER_B)
  * @retval GPIO_PinState: The current state of the encoder channel
  */
GPIO_PinState GPIO_ReadEncoderChannel(EncoderChannel_t channel)
{
  if (channel > ENCODER_B) {
    return GPIO_PIN_RESET;  /* Invalid channel */
  }
  
  return HAL_GPIO_ReadPin(encoderPins[channel].port, encoderPins[channel].pin);
}

/**
  * @brief  Configure all peripheral GPIOs (I2S, I2C, SPI)
  * @retval None
  */
void GPIO_ConfigurePeripherals(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  /*--------------------- I2S2 GPIO Configuration --------------------------*/
  /* I2S2_SD (MOSI) pin configuration : PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  /* I2S2_WS (LRCK) pin configuration : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  /* I2S2_CK (SCLK) pin configuration : PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  /*--------------------- I2S3 GPIO Configuration --------------------------*/
  /* I2S3_SD (MOSI) pin configuration : PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  /* I2S3_WS (LRCK) pin configuration : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  /* I2S3_CK (SCLK) pin configuration : PC10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
  /*--------------------- I2C1 GPIO Configuration --------------------------*/
  /* I2C1_SCL pin configuration : PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  /* I2C1_SDA pin configuration : PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  /*--------------------- SPI1 GPIO Configuration --------------------------*/
  /* SPI1_SCK pin configuration : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  /* SPI1_MISO pin configuration : PA6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  /* SPI1_MOSI pin configuration : PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  /* Configure the CODEC reset pin */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  /* Reset the CODEC - Active low */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
  HAL_Delay(5);  /* Wait for reset to take effect */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
  HAL_Delay(5);  /* Wait for CODEC to initialize */
}

/**
  * @brief  Update all LED indicators based on current system state
  * @param  systemSettings: Pointer to the current system settings
  * @retval None
  */
void GPIO_UpdateLedIndicators(SystemSettings_t *systemSettings)
{
  /* Update band active LEDs based on mute status */
  GPIO_SetLed(LED_SUB_ACTIVE, systemSettings->crossover.subMute ? GPIO_PIN_RESET : GPIO_PIN_SET);
  GPIO_SetLed(LED_LOW_ACTIVE, systemSettings->crossover.lowMute ? GPIO_PIN_RESET : GPIO_PIN_SET);
  GPIO_SetLed(LED_MID_ACTIVE, systemSettings->crossover.midMute ? GPIO_PIN_RESET : GPIO_PIN_SET);
  GPIO_SetLed(LED_HIGH_ACTIVE, systemSettings->crossover.highMute ? GPIO_PIN_RESET : GPIO_PIN_SET);
  
  /* Clip LED will be controlled by audio processing when levels exceed threshold */
}

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Configure user input pins (buttons and encoder)
  * @retval None
  */
static void GPIO_ConfigureUserInputs(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  /* Configure button pins */
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;  /* Use pullup for buttons */
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  
  for (int i = 0; i < BUTTON_COUNT; i++) {
    GPIO_InitStruct.Pin = buttonPins[i].pin;
    HAL_GPIO_Init(buttonPins[i].port, &GPIO_InitStruct);
  }
  
  /* Configure encoder pins */
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;  /* Use pullup for encoder */
  
  for (int i = 0; i < 2; i++) {
    GPIO_InitStruct.Pin = encoderPins[i].pin;
    HAL_GPIO_Init(encoderPins[i].port, &GPIO_InitStruct);
  }
}

/**
  * @brief  Configure output pins (LEDs)
  * @retval None
  */
static void GPIO_ConfigureOutputs(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  /* Configure LED pins */
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  
  for (int i = 0; i < LED_COUNT; i++) {
    GPIO_InitStruct.Pin = ledPins[i].pin;
    HAL_GPIO_Init(ledPins[i].port, &GPIO_InitStruct);
    
    /* Initialize all LEDs to off state */
    HAL_GPIO_WritePin(ledPins[i].port, ledPins[i].pin, GPIO_PIN_RESET);
  }
}

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
