 /**
  ******************************************************************************
  * @file           : audio_processing.h
  * @brief          : Header for audio_processing.c file.
  *                   This file contains the definitions and prototypes for the
  *                   audio processing core of the Audio Crossover DSP Control Panel.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AUDIO_PROCESSING_H
#define __AUDIO_PROCESSING_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  Audio processing statistics structure
  */
typedef struct {
    float inputPeakLevel[2];      /* Peak level for left and right input channels */
    float outputPeakLevel[2];     /* Peak level for left and right output channels */
    float bandPeakLevel[4][2];    /* Peak level for each band (sub, low, mid, high) and channel (L,R) */
    float compressionAmount[4];   /* Compression amount in dB for each band */
    float limiterActivity[4];     /* Limiter activity (gain reduction) in dB for each band */
    uint32_t clippingCount;       /* Number of samples that would have clipped without limiter */
    uint32_t processingTime;      /* Time in microseconds to process last block */
} AudioProcessingStats_t;

/* Exported constants --------------------------------------------------------*/
/* Audio band indices */
#define BAND_SUB    0
#define BAND_LOW    1
#define BAND_MID    2
#define BAND_HIGH   3
#define NUM_BANDS   4

/* Channel indices */
#define CHANNEL_LEFT   0
#define CHANNEL_RIGHT  1
#define NUM_CHANNELS   2

/* Maximum sample value for int16_t */
#define MAX_SAMPLE_VALUE 32767
#define MIN_SAMPLE_VALUE -32768

/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  Initialize audio processing modules
  * @retval None
  */
void AudioProcessing_Init(void);

/**
  * @brief  Process a block of audio samples through the DSP chain
  * @param  pInputBuffer  Pointer to input audio buffer
  * @param  pOutputBuffer Pointer to output audio buffer
  * @param  pSettings     Pointer to system settings
  * @retval None
  */
void AudioProcessing_Process(
    AudioBuffer_t *pInputBuffer, 
    AudioBuffer_t *pOutputBuffer,
    SystemSettings_t *pSettings
);

/**
  * @brief  Get current audio processing statistics
  * @param  pStats Pointer to statistics structure to fill
  * @retval None
  */
void AudioProcessing_GetStats(AudioProcessingStats_t *pStats);

/**
  * @brief  Reset audio processing state (e.g., after settings change)
  * @retval None
  */
void AudioProcessing_Reset(void);

/**
  * @brief  Enable or disable bypass mode (raw audio pass-through)
  * @param  enable 1 to enable bypass, 0 to disable
  * @retval None
  */
void AudioProcessing_SetBypass(uint8_t enable);

/**
  * @brief  Get current bypass mode status
  * @retval 1 if bypass is enabled, 0 otherwise
  */
uint8_t AudioProcessing_GetBypass(void);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_PROCESSING_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/