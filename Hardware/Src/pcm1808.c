 /**
  ******************************************************************************
  * @file           : pcm1808.c
  * @brief          : PCM1808 ADC driver implementation
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
#include "pcm1808.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define PCM1808_TIMEOUT 1000  /* Timeout for I2S operations (ms) */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void PCM1808_ConfigPins(PCM1808_HandleTypeDef *hpcm);
static void PCM1808_SetFormat(PCM1808_HandleTypeDef *hpcm, uint8_t format);
static void PCM1808_SetMode(PCM1808_HandleTypeDef *hpcm, uint8_t mode);
static uint8_t PCM1808_ConfigI2S(PCM1808_HandleTypeDef *hpcm);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize PCM1808 ADC
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval Status (PCM1808_STATUS_OK if successful)
  */
uint8_t PCM1808_Init(PCM1808_HandleTypeDef *hpcm)
{
  /* Check if already initialized */
  if (hpcm->initialized)
  {
    return PCM1808_STATUS_OK;
  }
  
  /* Configure control pins */
  PCM1808_ConfigPins(hpcm);
  
  /* Set mode to slave */
  PCM1808_SetMode(hpcm, PCM1808_MD_SLAVE);
  
  /* Set format to I2S */
  PCM1808_SetFormat(hpcm, PCM1808_FMT_24BIT_I2S);
  
  /* Set gain to 0dB by default */
  if (hpcm->config.useHwGain)
  {
    PCM1808_SET_HW_GAIN(hpcm, PCM1808_GAIN_0DB);
  }
  
  /* Configure I2S interface */
  if (PCM1808_ConfigI2S(hpcm) != PCM1808_STATUS_OK)
  {
    return PCM1808_STATUS_ERROR;
  }
  
  /* Mark as initialized */
  hpcm->initialized = 1;
  hpcm->running = 0;
  hpcm->overrunError = 0;
  
  return PCM1808_STATUS_OK;
}

/**
  * @brief  Start PCM1808 ADC operation
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @param  pBuffer pointer to the buffer to receive data
  * @param  Size buffer size in number of samples
  * @retval Status (PCM1808_STATUS_OK if successful)
  */
uint8_t PCM1808_Start(PCM1808_HandleTypeDef *hpcm, void *pBuffer, uint32_t Size)
{
  HAL_StatusTypeDef hal_status;
  
  /* Check if initialized */
  if (!hpcm->initialized)
  {
    return PCM1808_STATUS_ERROR;
  }
  
  /* Check if already running */
  if (hpcm->running)
  {
    return PCM1808_STATUS_BUSY;
  }
  
  /* Store buffer information */
  hpcm->rxBuffer = pBuffer;
  hpcm->rxBufferSize = Size;
  
  /* Start receiving data using DMA */
  hal_status = HAL_I2S_Receive_DMA(hpcm->config.i2s, pBuffer, Size);
  
  if (hal_status != HAL_OK)
  {
    return PCM1808_STATUS_ERROR;
  }
  
  /* Mark as running */
  hpcm->running = 1;
  
  return PCM1808_STATUS_OK;
}

/**
  * @brief  Stop PCM1808 ADC operation
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval Status (PCM1808_STATUS_OK if successful)
  */
uint8_t PCM1808_Stop(PCM1808_HandleTypeDef *hpcm)
{
  HAL_StatusTypeDef hal_status;
  
  /* Check if running */
  if (!hpcm->running)
  {
    return PCM1808_STATUS_OK; /* Already stopped */
  }
  
  /* Stop DMA reception */
  hal_status = HAL_I2S_DMAStop(hpcm->config.i2s);
  
  if (hal_status != HAL_OK)
  {
    return PCM1808_STATUS_ERROR;
  }
  
  /* Mark as stopped */
  hpcm->running = 0;
  
  return PCM1808_STATUS_OK;
}

/**
  * @brief  Set gain level for PCM1808
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @param  gain gain level (PCM1808_GAIN_0DB or PCM1808_GAIN_PLUS_3_5DB)
  * @retval Status (PCM1808_STATUS_OK if successful)
  */
uint8_t PCM1808_SetGain(PCM1808_HandleTypeDef *hpcm, uint8_t gain)
{
  /* Check if hardware gain control is enabled */
  if (!hpcm->config.useHwGain)
  {
    return PCM1808_STATUS_ERROR;
  }
  
  /* Set gain level */
  PCM1808_SET_HW_GAIN(hpcm, gain);
  
  return PCM1808_STATUS_OK;
}

/**
  * @brief  Check if PCM1808 has detected an overrun error
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval 1 if overrun detected, 0 otherwise
  */
uint8_t PCM1808_HasOverrun(PCM1808_HandleTypeDef *hpcm)
{
  return hpcm->overrunError;
}

/**
  * @brief  Clear overrun error flag
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval None
  */
void PCM1808_ClearOverrun(PCM1808_HandleTypeDef *hpcm)
{
  hpcm->overrunError = 0;
}

/**
  * @brief  Check if PCM1808 is currently running
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval 1 if running, 0 otherwise
  */
uint8_t PCM1808_IsRunning(PCM1808_HandleTypeDef *hpcm)
{
  return hpcm->running;
}

/**
  * @brief  Handle I2S transfer complete callback
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval None
  */
void PCM1808_RxCpltCallback(PCM1808_HandleTypeDef *hpcm)
{
  /* This function should be called from the HAL_I2S_RxCpltCallback */
  /* Implement application-specific handling here */
}

/**
  * @brief  Handle I2S error callback
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval None
  */
void PCM1808_ErrorCallback(PCM1808_HandleTypeDef *hpcm)
{
  /* Check for overrun error */
  if (hpcm->config.i2s->ErrorCode & HAL_I2S_ERROR_OVR)
  {
    hpcm->overrunError = 1;
  }
  
  /* Stop the transfer if running */
  if (hpcm->running)
  {
    PCM1808_Stop(hpcm);
  }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Configure PCM1808 control pins
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval None
  */
static void PCM1808_ConfigPins(PCM1808_HandleTypeDef *hpcm)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  /* Configure FMT pin */
  GPIO_InitStruct.Pin = hpcm->config.fmtPin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(hpcm->config.fmtGpioPort, &GPIO_InitStruct);
  
  /* Configure MD pin */
  GPIO_InitStruct.Pin = hpcm->config.mdPin;
  HAL_GPIO_Init(hpcm->config.mdGpioPort, &GPIO_InitStruct);
  
  /* Configure HW gain pin if used */
  if (hpcm->config.useHwGain)
  {
    GPIO_InitStruct.Pin = hpcm->config.hwGainPin;
    HAL_GPIO_Init(hpcm->config.hwGainGpioPort, &GPIO_InitStruct);
  }
  
  /* Configure SCKO pin as input (if used for monitoring) */
  if (hpcm->config.sckoGpioPort != NULL)
  {
    GPIO_InitStruct.Pin = hpcm->config.sckoPin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(hpcm->config.sckoGpioPort, &GPIO_InitStruct);
  }
}

/**
  * @brief  Set the format mode for PCM1808
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @param  format format mode (PCM1808_FMT_24BIT_MSB or PCM1808_FMT_24BIT_I2S)
  * @retval None
  */
static void PCM1808_SetFormat(PCM1808_HandleTypeDef *hpcm, uint8_t format)
{
  PCM1808_SET_FMT(hpcm, format);
}

/**
  * @brief  Set the operation mode for PCM1808
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @param  mode operation mode (PCM1808_MD_SLAVE or PCM1808_MD_MASTER)
  * @retval None
  */
static void PCM1808_SetMode(PCM1808_HandleTypeDef *hpcm, uint8_t mode)
{
  PCM1808_SET_MD(hpcm, mode);
}

/**
  * @brief  Configure I2S interface for PCM1808
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval Status (PCM1808_STATUS_OK if successful)
  */
static uint8_t PCM1808_ConfigI2S(PCM1808_HandleTypeDef *hpcm)
{
  I2S_HandleTypeDef *hi2s = hpcm->config.i2s;
  
  /* Check if I2S handle is valid */
  if (hi2s == NULL)
  {
    return PCM1808_STATUS_ERROR;
  }
  
  /* I2S should already be initialized by MX_I2S_Init functions */
  /* Set the appropriate data format and sample rate if needed */
  
  /* PCM1808 uses standard I2S protocol with 24-bit data */
  /* Make sure I2S is configured for appropriate word length */
  hi2s->Init.DataFormat = I2S_DATAFORMAT_24B;
  
  /* Set sample rate according to configuration */
  switch (hpcm->config.sampleRate)
  {
    case 44100:
      hi2s->Init.AudioFreq = I2S_AUDIOFREQ_44K;
      break;
    case 48000:
      hi2s->Init.AudioFreq = I2S_AUDIOFREQ_48K;
      break;
    case 96000:
      hi2s->Init.AudioFreq = I2S_AUDIOFREQ_96K;
      break;
    default:
      /* Default to 48kHz */
      hi2s->Init.AudioFreq = I2S_AUDIOFREQ_48K;
      break;
  }
  
  /* PCM1808 should be in slave mode, so I2S should be master */
  if (hi2s->Init.Mode != I2S_MODE_MASTER_RX)
  {
    hi2s->Init.Mode = I2S_MODE_MASTER_RX;
  }
  
  /* Re-initialize I2S with new settings */
  if (HAL_I2S_Init(hi2s) != HAL_OK)
  {
    return PCM1808_STATUS_ERROR;
  }
  
  return PCM1808_STATUS_OK;
}

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
