 /**
  ******************************************************************************
  * @file           : delay.h
  * @brief          : Header for delay.c file.
  *                   This file contains declarations for delay and phase
  *                   adjustment functionality for the Audio Crossover system.
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
#ifndef __DELAY_H
#define __DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported constants --------------------------------------------------------*/
#define MAX_DELAY_MS           100.0f   /* Maximum delay time in milliseconds */
#define DELAY_BUFFER_SIZE      4800     /* Buffer size for delay lines (48kHz * 0.1s) */
#define DELAY_RESOLUTION_MS    0.02f    /* Delay resolution in milliseconds (1/48kHz) */

/* Delay channel identifiers */
#define DELAY_CHANNEL_SUB      0
#define DELAY_CHANNEL_LOW      1
#define DELAY_CHANNEL_MID      2
#define DELAY_CHANNEL_HIGH     3
#define DELAY_NUM_CHANNELS     4

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  Delay settings structure (also defined in main.h as part of SystemSettings_t)
  */
typedef struct DelaySettings_t {
    /* Delay for each band in milliseconds */
    float subDelay;
    float lowDelay;
    float midDelay;
    float highDelay;
    
    /* Phase inversion for each band (1: inverted, 0: normal) */
    uint8_t subPhaseInvert;
    uint8_t lowPhaseInvert;
    uint8_t midPhaseInvert;
    uint8_t highPhaseInvert;
} DelaySettings_t;

/**
  * @brief  Delay instance structure
  */
typedef struct {
    int16_t buffer[DELAY_NUM_CHANNELS][DELAY_BUFFER_SIZE];  /* Delay buffers for each channel */
    uint16_t writeIndex;                                   /* Current write position */
    uint16_t readIndex[DELAY_NUM_CHANNELS];                /* Read positions for each channel */
    uint8_t phaseInvert[DELAY_NUM_CHANNELS];               /* Phase inversion flags */
    float delaySamples[DELAY_NUM_CHANNELS];                /* Delay in samples for each channel */
    uint8_t needsUpdate;                                   /* Flag to update delay parameters */
} Delay_t;

/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  Initialize the delay module
  * @retval None
  */
void Delay_Init(void);

/**
  * @brief  Process a block of audio through the delay module
  * @param  input   Pointer to input buffer containing audio to be delayed
  * @param  output  Pointer to output buffer to receive delayed audio
  * @param  size    Number of samples to process
  * @retval None
  */
void Delay_Process(int16_t *input, int16_t *output, uint16_t size);

/**
  * @brief  Set delay time for a specific channel
  * @param  channel Channel identifier (DELAY_CHANNEL_SUB, etc.)
  * @param  delayMs Delay time in milliseconds
  * @retval None
  */
void Delay_SetDelayTime(uint8_t channel, float delayMs);

/**
  * @brief  Set phase inversion for a specific channel
  * @param  channel Channel identifier (DELAY_CHANNEL_SUB, etc.)
  * @param  invert  1 to invert phase, 0 for normal phase
  * @retval None
  */
void Delay_SetPhaseInvert(uint8_t channel, uint8_t invert);

/**
  * @brief  Set delay settings from configuration structure
  * @param  settings Pointer to delay settings structure
  * @retval None
  */
void Delay_SetSettings(const DelaySettings_t *settings);

/**
  * @brief  Get current delay settings
  * @param  settings Pointer to delay settings structure to fill
  * @retval None
  */
void Delay_GetSettings(DelaySettings_t *settings);

/**
  * @brief  Reset all delay lines to zero
  * @retval None
  */
void Delay_Reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
