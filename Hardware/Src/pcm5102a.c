 /**
  ******************************************************************************
  * @file           : pcm5102a.c
  * @brief          : PCM5102A DAC driver implementation
  * @author         : Audio Crossover DSP Project
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
#include "pcm5102a.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void PCM5102A_ConfigurePins(PCM5102A_HandleTypeDef *hpcm);
static HAL_StatusTypeDef PCM5102A_ConfigureI2S(PCM5102A_HandleTypeDef *hpcm);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize PCM5102A DAC
  * @param  hpcm Pointer to PCM5102A handle structure
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_Init(PCM5102A_HandleTypeDef *hpcm)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check if handle is valid */
  if (hpcm == NULL)
  {
    return HAL_ERROR;
  }
  
  /* Configure control pins */
  PCM5102A_ConfigurePins(hpcm);
  
  /* Configure I2S interface */
  status = PCM5102A_ConfigureI2S(hpcm);
  if (status != HAL_OK)
  {
    return status;
  }
  
  /* Set initial state - muted */
  HAL_GPIO_WritePin(PCM5102A_XSMT_PORT, PCM5102A_XSMT_PIN, GPIO_PIN_RESET);
  hpcm->muted = 1;
  
  /* Set format (I2S or Left-justified) */
  HAL_GPIO_WritePin(PCM5102A_FMT_PORT, PCM5102A_FMT_PIN, 
                    (hpcm->config.format == PCM5102A_FORMAT_I2S) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  
  /* Set filter roll-off */
  HAL_GPIO_WritePin(PCM5102A_FLT_PORT, PCM5102A_FLT_PIN, 
                    (hpcm->config.filterRolloff == PCM5102A_FILTER_SLOW) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  
  /* Set de-emphasis */
  HAL_GPIO_WritePin(PCM5102A_DMP_PORT, PCM5102A_DMP_PIN, 
                    (hpcm->config.deemphasis == PCM5102A_DEEMPH_ON) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  
  /* Set system clock divider */
  HAL_GPIO_WritePin(PCM5102A_SCL_PORT, PCM5102A_SCL_PIN, 
                    (hpcm->config.sysclkDiv == PCM5102A_SCL_MCLK) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  
  /* Mark as initialized */
  hpcm->initialized = 1;
  
  /* Un-mute after a short delay to prevent pops */
  HAL_Delay(10);
  PCM5102A_SetMute(hpcm, 0);
  
  return HAL_OK;
}

/**
  * @brief  Deinitialize PCM5102A DAC
  * @param  hpcm Pointer to PCM5102A handle structure
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_DeInit(PCM5102A_HandleTypeDef *hpcm)
{
  /* Check if handle is valid */
  if (hpcm == NULL || hpcm->initialized == 0)
  {
    return HAL_ERROR;
  }
  
  /* Mute the DAC before deinitialization */
  PCM5102A_SetMute(hpcm, 1);
  
  /* Stop I2S peripheral */
  if (hpcm->config.hi2s != NULL)
  {
    HAL_I2S_DMAStop(hpcm->config.hi2s);
  }
  
  /* Mark as not initialized */
  hpcm->initialized = 0;
  
  return HAL_OK;
}

/**
  * @brief  Start PCM5102A DAC (unmute)
  * @param  hpcm Pointer to PCM5102A handle structure
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_Start(PCM5102A_HandleTypeDef *hpcm)
{
  /* Check if handle is valid */
  if (hpcm == NULL || hpcm->initialized == 0)
  {
    return HAL_ERROR;
  }
  
  /* Un-mute the DAC */
  return PCM5102A_SetMute(hpcm, 0);
}

/**
  * @brief  Stop PCM5102A DAC (mute)
  * @param  hpcm Pointer to PCM5102A handle structure
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_Stop(PCM5102A_HandleTypeDef *hpcm)
{
  /* Check if handle is valid */
  if (hpcm == NULL || hpcm->initialized == 0)
  {
    return HAL_ERROR;
  }
  
  /* Mute the DAC */
  return PCM5102A_SetMute(hpcm, 1);
}

/**
  * @brief  Set PCM5102A mute state
  * @param  hpcm Pointer to PCM5102A handle structure
  * @param  state Mute state (1: muted, 0: unmuted)
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_SetMute(PCM5102A_HandleTypeDef *hpcm, uint8_t state)
{
  /* Check if handle is valid */
  if (hpcm == NULL || hpcm->initialized == 0)
  {
    return HAL_ERROR;
  }
  
  /* Set XSMT pin (active low for mute) */
  HAL_GPIO_WritePin(PCM5102A_XSMT_PORT, PCM5102A_XSMT_PIN, 
                   (state == 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  
  /* Update state */
  hpcm->muted = state;
  
  return HAL_OK;
}

/**
  * @brief  Set PCM5102A filter roll-off
  * @param  hpcm Pointer to PCM5102A handle structure
  * @param  filter Filter setting (PCM5102A_FILTER_SLOW or PCM5102A_FILTER_FAST)
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_SetFilter(PCM5102A_HandleTypeDef *hpcm, uint8_t filter)
{
  /* Check if handle is valid */
  if (hpcm == NULL || hpcm->initialized == 0)
  {
    return HAL_ERROR;
  }
  
  /* Set FLT pin */
  HAL_GPIO_WritePin(PCM5102A_FLT_PORT, PCM5102A_FLT_PIN, 
                   (filter == PCM5102A_FILTER_SLOW) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  
  /* Update configuration */
  hpcm->config.filterRolloff = filter;
  
  return HAL_OK;
}

/**
  * @brief  Set PCM5102A de-emphasis
  * @param  hpcm Pointer to PCM5102A handle structure
  * @param  deemph De-emphasis setting (PCM5102A_DEEMPH_ON or PCM5102A_DEEMPH_OFF)
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_SetDeemphasis(PCM5102A_HandleTypeDef *hpcm, uint8_t deemph)
{
  /* Check if handle is valid */
  if (hpcm == NULL || hpcm->initialized == 0)
  {
    return HAL_ERROR;
  }
  
  /* Set DMP pin */
  HAL_GPIO_WritePin(PCM5102A_DMP_PORT, PCM5102A_DMP_PIN, 
                   (deemph == PCM5102A_DEEMPH_ON) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  
  /* Update configuration */
  hpcm->config.deemphasis = deemph;
  
  return HAL_OK;
}

/**
  * @brief  Set PCM5102A system clock divider
  * @param  hpcm Pointer to PCM5102A handle structure
  * @param  div Clock divider setting (PCM5102A_SCL_MCLK or PCM5102A_SCL_MCLK_DIV2)
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_SetSysclkDiv(PCM5102A_HandleTypeDef *hpcm, uint8_t div)
{
  /* Check if handle is valid */
  if (hpcm == NULL || hpcm->initialized == 0)
  {
    return HAL_ERROR;
  }
  
  /* Set SCL pin */
  HAL_GPIO_WritePin(PCM5102A_SCL_PORT, PCM5102A_SCL_PIN, 
                   (div == PCM5102A_SCL_MCLK) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  
  /* Update configuration */
  hpcm->config.sysclkDiv = div;
  
  return HAL_OK;
}

/**
  * @brief  Set PCM5102A sample rate
  * @param  hpcm Pointer to PCM5102A handle structure
  * @param  sampleRate Sample rate in Hz (one of PCM5102A_RATE_x values)
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_SetSampleRate(PCM5102A_HandleTypeDef *hpcm, PCM5102A_SampleRate_t sampleRate)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check if handle is valid */
  if (hpcm == NULL || hpcm->initialized == 0 || hpcm->config.hi2s == NULL)
  {
    return HAL_ERROR;
  }
  
  /* Temporarily mute to prevent pops during sample rate change */
  uint8_t wasMuted = hpcm->muted;
  if (!wasMuted)
  {
    PCM5102A_SetMute(hpcm, 1);
  }
  
  /* Stop I2S */
  HAL_I2S_DMAStop(hpcm->config.hi2s);
  
  /* Update configuration */
  hpcm->config.sampleRate = sampleRate;
  
  /* Reconfigure I2S with new sample rate */
  hpcm->config.hi2s->Init.AudioFreq = (uint32_t)sampleRate;
  status = HAL_I2S_Init(hpcm->config.hi2s);
  
  /* Restore mute state */
  if (!wasMuted)
  {
    PCM5102A_SetMute(hpcm, 0);
  }
  
  return status;
}

/**
  * @brief  Send audio data to PCM5102A via I2S
  * @param  hpcm Pointer to PCM5102A handle structure
  * @param  pData Pointer to data buffer
  * @param  Size Amount of data elements to send
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_SendData(PCM5102A_HandleTypeDef *hpcm, uint16_t *pData, uint16_t Size)
{
  /* Check if handle is valid */
  if (hpcm == NULL || hpcm->initialized == 0 || hpcm->config.hi2s == NULL)
  {
    return HAL_ERROR;
  }
  
  /* Send data via I2S */
  return HAL_I2S_Transmit(hpcm->config.hi2s, pData, Size, HAL_MAX_DELAY);
}

/**
  * @brief  Send audio data to PCM5102A via I2S with DMA
  * @param  hpcm Pointer to PCM5102A handle structure
  * @param  pData Pointer to data buffer
  * @param  Size Amount of data elements to send
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_SendData_DMA(PCM5102A_HandleTypeDef *hpcm, uint16_t *pData, uint16_t Size)
{
  /* Check if handle is valid */
  if (hpcm == NULL || hpcm->initialized == 0 || hpcm->config.hi2s == NULL)
  {
    return HAL_ERROR;
  }
  
  /* Send data via I2S with DMA */
  return HAL_I2S_Transmit_DMA(hpcm->config.hi2s, pData, Size);
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Configure PCM5102A control pins
  * @param  hpcm Pointer to PCM5102A handle structure
  * @retval None
  */
static void PCM5102A_ConfigurePins(PCM5102A_HandleTypeDef *hpcm)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE(); /* Adjust if your pins are on different ports */
  
  /* Configure FMT pin */
  GPIO_InitStruct.Pin = PCM5102A_FMT_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PCM5102A_FMT_PORT, &GPIO_InitStruct);
  
  /* Configure XSMT pin */
  GPIO_InitStruct.Pin = PCM5102A_XSMT_PIN;
  HAL_GPIO_Init(PCM5102A_XSMT_PORT, &GPIO_InitStruct);
  
  /* Configure FLT pin */
  GPIO_InitStruct.Pin = PCM5102A_FLT_PIN;
  HAL_GPIO_Init(PCM5102A_FLT_PORT, &GPIO_InitStruct);
  
  /* Configure DMP pin */
  GPIO_InitStruct.Pin = PCM5102A_DMP_PIN;
  HAL_GPIO_Init(PCM5102A_DMP_PORT, &GPIO_InitStruct);
  
  /* Configure SCL pin */
  GPIO_InitStruct.Pin = PCM5102A_SCL_PIN;
  HAL_GPIO_Init(PCM5102A_SCL_PORT, &GPIO_InitStruct);
  
  /* Set initial states to default (all LOW) */
  HAL_GPIO_WritePin(PCM5102A_FMT_PORT, PCM5102A_FMT_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(PCM5102A_XSMT_PORT, PCM5102A_XSMT_PIN, GPIO_PIN_RESET); /* Muted */
  HAL_GPIO_WritePin(PCM5102A_FLT_PORT, PCM5102A_FLT_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(PCM5102A_DMP_PORT, PCM5102A_DMP_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(PCM5102A_SCL_PORT, PCM5102A_SCL_PIN, GPIO_PIN_RESET);
}

/**
  * @brief  Configure I2S interface for PCM5102A
  * @param  hpcm Pointer to PCM5102A handle structure
  * @retval HAL status
  */
static HAL_StatusTypeDef PCM5102A_ConfigureI2S(PCM5102A_HandleTypeDef *hpcm)
{
  /* Check if I2S handle is valid */
  if (hpcm->config.hi2s == NULL)
  {
    return HAL_ERROR;
  }
  
  /* I2S configuration is typically done in the MX-generated code */
  /* Here we only adjust parameters specific to PCM5102A requirements */
  
  /* PCM5102A supports I2S Philips standard */
  hpcm->config.hi2s->Init.Standard = I2S_STANDARD_PHILIPS;
  
  /* PCM5102A supports 16 or 24-bit data */
  hpcm->config.hi2s->Init.DataFormat = I2S_DATAFORMAT_16B;
  
  /* Configure for master transmit mode */
  hpcm->config.hi2s->Init.Mode = I2S_MODE_MASTER_TX;
  
  /* Configure sample rate */
  hpcm->config.hi2s->Init.AudioFreq = (uint32_t)hpcm->config.sampleRate;
  
  /* Initialize I2S peripheral */
  return HAL_I2S_Init(hpcm->config.hi2s);
}

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/