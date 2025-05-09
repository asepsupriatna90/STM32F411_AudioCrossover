 /**
  ******************************************************************************
  * @file           : i2s_config.c
  * @brief          : I2S configuration and management source file
  *                   This file provides code for I2S configuration and 
  *                   management for audio codec interfaces.
  * @author         : Based on STM32F411 Audio Crossover Project
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
#include "i2s_config.h"
#include "gpio.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define I2S_TIMEOUT                  1000U /* Timeout for I2S operations in ms */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static AudioFreq_t CurrentAudioFreq = DEFAULT_AUDIO_SAMPLE_RATE;
static AudioRes_t CurrentAudioResolution = DEFAULT_AUDIO_RESOLUTION;
static uint8_t CurrentAudioChannels = DEFAULT_AUDIO_CHANNELS;
static uint8_t AudioMuted = 0;
static uint8_t I2SInitialized = 0;

/* Private function prototypes -----------------------------------------------*/
static uint32_t I2S_GetPLLI2S_N(AudioFreq_t AudioFreq);
static uint32_t I2S_GetPLLI2S_R(AudioFreq_t AudioFreq);

/* External variables --------------------------------------------------------*/
extern I2S_HandleTypeDef hi2s2; /* I2S handler for input (ADC) */
extern I2S_HandleTypeDef hi2s3; /* I2S handler for output (DAC) */

/**
  * @brief  Initialize I2S interface for audio communication
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure that contains
  *         the configuration information for I2S module
  * @param  Config Pointer to I2S_AudioConfigTypeDef structure that contains
  *         the audio configuration parameters
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_Init(I2S_HandleTypeDef* I2SHandle, I2S_AudioConfigTypeDef* Config) {
  HAL_StatusTypeDef hal_status;
  
  /* Validate parameters */
  if (I2SHandle == NULL || Config == NULL) {
    return I2S_STATUS_ERROR;
  }
  
  if (!IS_AUDIO_FREQUENCY(Config->SampleRate)) {
    return I2S_STATUS_ERROR;
  }
  
  /* Store configuration in static variables */
  CurrentAudioFreq = Config->SampleRate;
  CurrentAudioResolution = Config->Resolution;
  CurrentAudioChannels = Config->ChannelCount;
  
  /* Configure I2S PLL clock based on the sample rate */
  if (I2S_Config_SetupClock(I2SHandle, Config->SampleRate) != I2S_STATUS_OK) {
    return I2S_STATUS_ERROR;
  }
  
  /* Initialize I2S peripheral */
  I2SHandle->Init.Mode = I2S_MODE_MASTER_TX;
  I2SHandle->Init.Standard = I2S_STANDARD_PHILIPS;
  I2SHandle->Init.DataFormat = (Config->Resolution == AUDIO_RESOLUTION_16B) ? I2S_DATAFORMAT_16B : 
                              (Config->Resolution == AUDIO_RESOLUTION_24B) ? I2S_DATAFORMAT_24B : 
                              I2S_DATAFORMAT_32B;
  I2SHandle->Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  I2SHandle->Init.AudioFreq = (uint32_t)Config->SampleRate;
  I2SHandle->Init.CPOL = I2S_CPOL_LOW;
  I2SHandle->Init.ClockSource = I2S_CLOCK_PLL;
  I2SHandle->Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  
  /* Initialize the I2S peripheral with the new configuration */
  hal_status = HAL_I2S_Init(I2SHandle);
  if (hal_status != HAL_OK) {
    return I2S_STATUS_ERROR;
  }
  
  I2SInitialized = 1;
  return I2S_STATUS_OK;
}

/**
  * @brief  Initialize PCM1808 ADC
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_InitPCM1808(void) {
  /* Configure PCM1808 pins and mode */
  
  /* Configure Format (I2S mode) */
  HAL_GPIO_WritePin(PCM1808_FMT_PORT, PCM1808_FMT_PIN, GPIO_PIN_RESET); /* I2S format */
  
  /* Configure Mode (slave mode) */
  HAL_GPIO_WritePin(PCM1808_MD_PORT, PCM1808_MD_PIN, GPIO_PIN_RESET); /* Slave mode */
  
  /* Additional PCM1808 initialization code as needed */
  
  return I2S_STATUS_OK;
}

/**
  * @brief  Initialize PCM5102A DAC
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_InitPCM5102A(void) {
  /* PCM5102A is a simple I2S DAC with no control interface required */
  /* Just ensure it's not muted */
  HAL_GPIO_WritePin(PCM5102A_MUTE_PORT, PCM5102A_MUTE_PIN, GPIO_PIN_RESET); /* Unmute */
  AudioMuted = 0;
  
  return I2S_STATUS_OK;
}

/**
  * @brief  Start I2S audio reception with DMA
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure
  * @param  pData Pointer to reception data buffer
  * @param  Size Size of data buffer
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_StartAudioReceive(I2S_HandleTypeDef* I2SHandle, uint16_t* pData, uint16_t Size) {
  HAL_StatusTypeDef hal_status;
  
  /* Validate parameters */
  if (I2SHandle == NULL || pData == NULL || Size == 0) {
    return I2S_STATUS_ERROR;
  }
  
  /* Start I2S reception with DMA */
  hal_status = HAL_I2S_Receive_DMA(I2SHandle, pData, Size);
  if (hal_status != HAL_OK) {
    return I2S_STATUS_ERROR;
  }
  
  return I2S_STATUS_OK;
}

/**
  * @brief  Start I2S audio transmission with DMA
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure
  * @param  pData Pointer to transmission data buffer
  * @param  Size Size of data buffer
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_StartAudioTransmit(I2S_HandleTypeDef* I2SHandle, uint16_t* pData, uint16_t Size) {
  HAL_StatusTypeDef hal_status;
  
  /* Validate parameters */
  if (I2SHandle == NULL || pData == NULL || Size == 0) {
    return I2S_STATUS_ERROR;
  }
  
  /* Start I2S transmission with DMA */
  hal_status = HAL_I2S_Transmit_DMA(I2SHandle, pData, Size);
  if (hal_status != HAL_OK) {
    return I2S_STATUS_ERROR;
  }
  
  return I2S_STATUS_OK;
}

/**
  * @brief  Stop I2S audio reception
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_StopAudioReceive(I2S_HandleTypeDef* I2SHandle) {
  HAL_StatusTypeDef hal_status;
  
  /* Validate parameters */
  if (I2SHandle == NULL) {
    return I2S_STATUS_ERROR;
  }
  
  /* Stop I2S reception */
  hal_status = HAL_I2S_DMAStop(I2SHandle);
  if (hal_status != HAL_OK) {
    return I2S_STATUS_ERROR;
  }
  
  return I2S_STATUS_OK;
}

/**
  * @brief  Stop I2S audio transmission
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_StopAudioTransmit(I2S_HandleTypeDef* I2SHandle) {
  HAL_StatusTypeDef hal_status;
  
  /* Validate parameters */
  if (I2SHandle == NULL) {
    return I2S_STATUS_ERROR;
  }
  
  /* Stop I2S transmission */
  hal_status = HAL_I2S_DMAStop(I2SHandle);
  if (hal_status != HAL_OK) {
    return I2S_STATUS_ERROR;
  }
  
  return I2S_STATUS_OK;
}

/**
  * @brief  Mute or unmute the PCM5102A output
  * @param  Mute 0: unmute, 1: mute
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_SetMute(uint8_t Mute) {
  /* Check if already in requested state */
  if (AudioMuted == Mute) {
    return I2S_STATUS_OK;
  }
  
  /* Set mute pin state */
  HAL_GPIO_WritePin(PCM5102A_MUTE_PORT, PCM5102A_MUTE_PIN, Mute ? GPIO_PIN_SET : GPIO_PIN_RESET);
  
  /* Update state */
  AudioMuted = Mute;
  
  return I2S_STATUS_OK;
}

/**
  * @brief  Change the audio frequency
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure
  * @param  AudioFreq Audio frequency to be configured
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_SetAudioFreq(I2S_HandleTypeDef* I2SHandle, AudioFreq_t AudioFreq) {
  HAL_StatusTypeDef hal_status;
  
  /* Validate parameters */
  if (I2SHandle == NULL || !IS_AUDIO_FREQUENCY(AudioFreq)) {
    return I2S_STATUS_ERROR;
  }
  
  /* Check if I2S is busy */
  if (I2S_Config_IsBusy(I2SHandle)) {
    return I2S_STATUS_BUSY;
  }
  
  /* Update the I2S PLL config for the new frequency */
  if (I2S_Config_SetupClock(I2SHandle, AudioFreq) != I2S_STATUS_OK) {
    return I2S_STATUS_ERROR;
  }
  
  /* Update I2S peripheral */
  I2SHandle->Init.AudioFreq = (uint32_t)AudioFreq;
  
  /* Re-initialize the I2S peripheral with the new configuration */
  hal_status = HAL_I2S_Init(I2SHandle);
  if (hal_status != HAL_OK) {
    return I2S_STATUS_ERROR;
  }
  
  /* Store new frequency */
  CurrentAudioFreq = AudioFreq;
  
  return I2S_STATUS_OK;
}

/**
  * @brief  Get current audio frequency
  * @retval Current audio frequency
  */
AudioFreq_t I2S_Config_GetAudioFreq(void) {
  return CurrentAudioFreq;
}

/**
  * @brief  Setup and configure I2S clock
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure
  * @param  AudioFreq Audio frequency to be configured
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_SetupClock(I2S_HandleTypeDef* I2SHandle, AudioFreq_t AudioFreq) {
  RCC_PeriphCLKInitTypeDef RCC_ExCLKInitStruct;
  HAL_StatusTypeDef hal_status;
  
  /* Validate parameters */
  if (I2SHandle == NULL || !IS_AUDIO_FREQUENCY(AudioFreq)) {
    return I2S_STATUS_ERROR;
  }
  
  /* Get the PLLI2S parameters for the requested audio frequency */
  RCC_ExCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
  RCC_ExCLKInitStruct.PLLI2S.PLLI2SN = I2S_GetPLLI2S_N(AudioFreq);
  RCC_ExCLKInitStruct.PLLI2S.PLLI2SR = I2S_GetPLLI2S_R(AudioFreq);
  
  /* Configure PLLI2S clock source */
  hal_status = HAL_RCCEx_PeriphCLKConfig(&RCC_ExCLKInitStruct);
  if (hal_status != HAL_OK) {
    return I2S_STATUS_ERROR;
  }
  
  return I2S_STATUS_OK;
}

/**
  * @brief  Check if I2S link is busy
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure
  * @retval 1 if busy, 0 if not busy
  */
uint8_t I2S_Config_IsBusy(I2S_HandleTypeDef* I2SHandle) {
  if (I2SHandle == NULL) {
    return 0;
  }
  
  /* Check if I2S is currently transmitting or receiving */
  if (HAL_I2S_GetState(I2SHandle) == HAL_I2S_STATE_BUSY ||
      HAL_I2S_GetState(I2SHandle) == HAL_I2S_STATE_BUSY_TX ||
      HAL_I2S_GetState(I2SHandle) == HAL_I2S_STATE_BUSY_RX ||
      HAL_I2S_GetState(I2SHandle) == HAL_I2S_STATE_BUSY_TX_RX) {
    return 1;
  }
  
  return 0;
}

/**
  * @brief  Get the PLLI2S N value for a given audio frequency
  * @param  AudioFreq Audio frequency
  * @retval PLLI2S N value
  */
static uint32_t I2S_GetPLLI2S_N(AudioFreq_t AudioFreq) {
  uint32_t plli2sn;
  
  /* Calculate the PLLI2S N parameter based on the audio frequency */
  /* For STM32F411, we need to set the PLLI2S to generate the proper I2S clock */
  switch (AudioFreq) {
    case AUDIO_FREQUENCY_8K:
    case AUDIO_FREQUENCY_16K:
    case AUDIO_FREQUENCY_32K:
    case AUDIO_FREQUENCY_48K:
    case AUDIO_FREQUENCY_96K:
      plli2sn = 192; /* Multiple of 48kHz family */
      break;
    case AUDIO_FREQUENCY_11K:
    case AUDIO_FREQUENCY_22K:
    case AUDIO_FREQUENCY_44K:
      plli2sn = 213; /* Multiple of 44.1kHz family */
      break;
    case AUDIO_FREQUENCY_192K:
      plli2sn = 384; /* For 192kHz sampling rate */
      break;
    default:
      plli2sn = 192;
      break;
  }
  
  return plli2sn;
}

/**
  * @brief  Get the PLLI2S R value for a given audio frequency
  * @param  AudioFreq Audio frequency
  * @retval PLLI2S R value
  */
static uint32_t I2S_GetPLLI2S_R(AudioFreq_t AudioFreq) {
  uint32_t plli2sr;
  
  /* Calculate the PLLI2S R parameter based on the audio frequency */
  switch (AudioFreq) {
    case AUDIO_FREQUENCY_8K:
      plli2sr = 32; /* For 8kHz sampling rate */
      break;
    case AUDIO_FREQUENCY_11K:
      plli2sr = 26; /* For 11.025kHz sampling rate */
      break;
    case AUDIO_FREQUENCY_16K:
      plli2sr = 16; /* For 16kHz sampling rate */
      break;
    case AUDIO_FREQUENCY_22K:
      plli2sr = 13; /* For 22.05kHz sampling rate */
      break;
    case AUDIO_FREQUENCY_32K:
      plli2sr = 8;  /* For 32kHz sampling rate */
      break;
    case AUDIO_FREQUENCY_44K:
      plli2sr = 6;  /* For 44.1kHz sampling rate */
      break;
    case AUDIO_FREQUENCY_48K:
      plli2sr = 4;  /* For 48kHz sampling rate */
      break;
    case AUDIO_FREQUENCY_96K:
      plli2sr = 2;  /* For 96kHz sampling rate */
      break;
    case AUDIO_FREQUENCY_192K:
      plli2sr = 2;  /* For 192kHz sampling rate, using higher PLLI2SN */
      break;
    default:
      plli2sr = 4;  /* Default value for 48kHz */
      break;
  }
  
  return plli2sr;
}

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
