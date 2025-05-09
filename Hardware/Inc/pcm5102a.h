 /**
  ******************************************************************************
  * @file           : pcm5102a.h
  * @brief          : Header file for PCM5102A DAC driver
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PCM5102A_H
#define __PCM5102A_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported constants --------------------------------------------------------*/
/**
  * @brief PCM5102A Control Pins
  * 
  * PCM5102A needs these control pins:
  * - FMT: Format selection (High: I2S, Low: Left-justified)
  * - XSMT: Mute control (Low: Mute, High: Normal operation)
  * - FLT: Filter selection (Low: Fast roll-off, High: Slow roll-off)
  * - DMP: De-emphasis control (Low: Off, High: On, 44.1kHz)
  * - SCL: System clock divider (Low: MCLK/2, High: MCLK)
  */
#define PCM5102A_FMT_PIN          GPIO_PIN_4
#define PCM5102A_FMT_PORT         GPIOA
#define PCM5102A_XSMT_PIN         GPIO_PIN_5
#define PCM5102A_XSMT_PORT        GPIOA
#define PCM5102A_FLT_PIN          GPIO_PIN_6
#define PCM5102A_FLT_PORT         GPIOA
#define PCM5102A_DMP_PIN          GPIO_PIN_7
#define PCM5102A_DMP_PORT         GPIOA
#define PCM5102A_SCL_PIN          GPIO_PIN_8
#define PCM5102A_SCL_PORT         GPIOA

/* Format settings */
#define PCM5102A_FORMAT_I2S           1
#define PCM5102A_FORMAT_LEFT_JUST     0

/* Filter roll-off settings */
#define PCM5102A_FILTER_SLOW          1
#define PCM5102A_FILTER_FAST          0

/* De-emphasis settings */
#define PCM5102A_DEEMPH_ON            1
#define PCM5102A_DEEMPH_OFF           0

/* System clock divider settings */
#define PCM5102A_SCL_MCLK             1
#define PCM5102A_SCL_MCLK_DIV2        0

/* Supported sample rates */
typedef enum {
    PCM5102A_RATE_44K1 = 44100,
    PCM5102A_RATE_48K = 48000,
    PCM5102A_RATE_88K2 = 88200,
    PCM5102A_RATE_96K = 96000,
    PCM5102A_RATE_176K4 = 176400,
    PCM5102A_RATE_192K = 192000
} PCM5102A_SampleRate_t;

/* Exported types ------------------------------------------------------------*/
/**
  * @brief PCM5102A Configuration Structure
  */
typedef struct {
    uint8_t format;         /* I2S format (I2S or Left-justified) */
    uint8_t filterRolloff;  /* Filter roll-off setting */
    uint8_t deemphasis;     /* De-emphasis setting */
    uint8_t sysclkDiv;      /* System clock divider setting */
    PCM5102A_SampleRate_t sampleRate; /* Sample rate setting */
    I2S_HandleTypeDef *hi2s; /* I2S handle for this DAC */
} PCM5102A_Config_t;

/**
  * @brief PCM5102A Handle Structure
  */
typedef struct {
    PCM5102A_Config_t config;  /* Configuration */
    uint8_t initialized;       /* Initialization state */
    uint8_t muted;             /* Mute state */
} PCM5102A_HandleTypeDef;

/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  Initialize PCM5102A DAC
  * @param  hpcm Pointer to PCM5102A handle structure
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_Init(PCM5102A_HandleTypeDef *hpcm);

/**
  * @brief  Deinitialize PCM5102A DAC
  * @param  hpcm Pointer to PCM5102A handle structure
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_DeInit(PCM5102A_HandleTypeDef *hpcm);

/**
  * @brief  Start PCM5102A DAC (unmute)
  * @param  hpcm Pointer to PCM5102A handle structure
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_Start(PCM5102A_HandleTypeDef *hpcm);

/**
  * @brief  Stop PCM5102A DAC (mute)
  * @param  hpcm Pointer to PCM5102A handle structure
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_Stop(PCM5102A_HandleTypeDef *hpcm);

/**
  * @brief  Set PCM5102A mute state
  * @param  hpcm Pointer to PCM5102A handle structure
  * @param  state Mute state (1: muted, 0: unmuted)
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_SetMute(PCM5102A_HandleTypeDef *hpcm, uint8_t state);

/**
  * @brief  Set PCM5102A filter roll-off
  * @param  hpcm Pointer to PCM5102A handle structure
  * @param  filter Filter setting (PCM5102A_FILTER_SLOW or PCM5102A_FILTER_FAST)
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_SetFilter(PCM5102A_HandleTypeDef *hpcm, uint8_t filter);

/**
  * @brief  Set PCM5102A de-emphasis
  * @param  hpcm Pointer to PCM5102A handle structure
  * @param  deemph De-emphasis setting (PCM5102A_DEEMPH_ON or PCM5102A_DEEMPH_OFF)
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_SetDeemphasis(PCM5102A_HandleTypeDef *hpcm, uint8_t deemph);

/**
  * @brief  Set PCM5102A system clock divider
  * @param  hpcm Pointer to PCM5102A handle structure
  * @param  div Clock divider setting (PCM5102A_SCL_MCLK or PCM5102A_SCL_MCLK_DIV2)
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_SetSysclkDiv(PCM5102A_HandleTypeDef *hpcm, uint8_t div);

/**
  * @brief  Set PCM5102A sample rate
  * @param  hpcm Pointer to PCM5102A handle structure
  * @param  sampleRate Sample rate in Hz (one of PCM5102A_RATE_x values)
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_SetSampleRate(PCM5102A_HandleTypeDef *hpcm, PCM5102A_SampleRate_t sampleRate);

/**
  * @brief  Send audio data to PCM5102A via I2S
  * @param  hpcm Pointer to PCM5102A handle structure
  * @param  pData Pointer to data buffer
  * @param  Size Amount of data elements to send
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_SendData(PCM5102A_HandleTypeDef *hpcm, uint16_t *pData, uint16_t Size);

/**
  * @brief  Send audio data to PCM5102A via I2S with DMA
  * @param  hpcm Pointer to PCM5102A handle structure
  * @param  pData Pointer to data buffer
  * @param  Size Amount of data elements to send
  * @retval HAL status
  */
HAL_StatusTypeDef PCM5102A_SendData_DMA(PCM5102A_HandleTypeDef *hpcm, uint16_t *pData, uint16_t Size);

#ifdef __cplusplus
}
#endif

#endif /* __PCM5102A_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
