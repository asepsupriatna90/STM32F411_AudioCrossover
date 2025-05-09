 /**
  ******************************************************************************
  * @file           : crossover.h
  * @brief          : Header for crossover.c file.
  *                   This file contains the declarations for the audio crossover
  *                   module of the DSP system.
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
#ifndef __CROSSOVER_H
#define __CROSSOVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
/* Filter types */
#define FILTER_TYPE_BUTTERWORTH   0
#define FILTER_TYPE_LINKWITZ_RILEY 1

/* Filter orders */
#define FILTER_ORDER_12DB      2  /* 12dB/octave (2nd order) */
#define FILTER_ORDER_24DB      4  /* 24dB/octave (4th order) */
#define FILTER_ORDER_48DB      8  /* 48dB/octave (8th order) */

/* Maximum filter order supported */
#define MAX_FILTER_ORDER       8

/* Filter type enumeration */
typedef enum {
    FILTER_LOW_PASS,
    FILTER_HIGH_PASS,
    FILTER_BAND_PASS,
    FILTER_BAND_STOP
} FilterType_t;

/* Second-order section (biquad) filter structure */
typedef struct {
    float b0, b1, b2;  /* Numerator coefficients */
    float a1, a2;      /* Denominator coefficients (a0 is assumed to be 1.0) */
    float x1, x2;      /* Input history */
    float y1, y2;      /* Output history */
} BiquadFilter_t;

/* Band structure for each frequency band */
typedef struct {
    BiquadFilter_t filters[MAX_FILTER_ORDER/2];  /* Array of biquad filters (cascade) */
    float gain;         /* Band gain (linear) */
    uint8_t mute;       /* Mute flag (1: muted, 0: active) */
    uint8_t numFilters; /* Number of active filters in the cascade */
} CrossoverBand_t;

/* Exported constants --------------------------------------------------------*/
/* Default crossover frequencies */
#define DEFAULT_LOW_CUTOFF    100.0f   /* Hz, Sub-Low transition */
#define DEFAULT_MID_CUTOFF    1000.0f  /* Hz, Low-Mid transition */
#define DEFAULT_HIGH_CUTOFF   8000.0f  /* Hz, Mid-High transition */

/* Default gains (in dB) */
#define DEFAULT_SUB_GAIN      0.0f
#define DEFAULT_LOW_GAIN      0.0f
#define DEFAULT_MID_GAIN      0.0f
#define DEFAULT_HIGH_GAIN     0.0f

/* Default filter settings */
#define DEFAULT_FILTER_TYPE   FILTER_TYPE_LINKWITZ_RILEY
#define DEFAULT_FILTER_ORDER  FILTER_ORDER_24DB

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  Initialize the crossover module
  * @retval None
  */
void Crossover_Init(void);

/**
  * @brief  Process audio through the crossover filters
  * @param  input: Pointer to input audio buffer
  * @param  subOut: Pointer to subwoofer output buffer
  * @param  lowOut: Pointer to low frequency output buffer
  * @param  midOut: Pointer to mid frequency output buffer
  * @param  highOut: Pointer to high frequency output buffer
  * @param  numSamples: Number of samples to process
  * @retval None
  */
void Crossover_Process(
    const float *input,
    float *subOut,
    float *lowOut,
    float *midOut, 
    float *highOut,
    uint16_t numSamples
);

/**
  * @brief  Set crossover settings from SystemSettings structure
  * @param  settings: Pointer to crossover settings
  * @retval None
  */
void Crossover_SetSettings(const struct CrossoverSettings_t *settings);

/**
  * @brief  Get current crossover settings
  * @param  settings: Pointer to store the current settings
  * @retval None
  */
void Crossover_GetSettings(struct CrossoverSettings_t *settings);

/**
  * @brief  Set the cutoff frequency for the specific crossover point
  * @param  point: Crossover point (0: low, 1: mid, 2: high)
  * @param  frequency: Cutoff frequency in Hz
  * @retval None
  */
void Crossover_SetCutoff(uint8_t point, float frequency);

/**
  * @brief  Set the gain for a specific frequency band
  * @param  band: Band index (0: sub, 1: low, 2: mid, 3: high)
  * @param  gainDB: Gain value in dB
  * @retval None
  */
void Crossover_SetGain(uint8_t band, float gainDB);

/**
  * @brief  Set mute state for a specific frequency band
  * @param  band: Band index (0: sub, 1: low, 2: mid, 3: high)
  * @param  mute: Mute state (1: muted, 0: active)
  * @retval None
  */
void Crossover_SetMute(uint8_t band, uint8_t mute);

/**
  * @brief  Set the filter type for all crossover filters
  * @param  type: Filter type (FILTER_TYPE_BUTTERWORTH or FILTER_TYPE_LINKWITZ_RILEY)
  * @retval None
  */
void Crossover_SetFilterType(uint8_t type);

/**
  * @brief  Set the filter order for all crossover filters
  * @param  order: Filter order (FILTER_ORDER_12DB, FILTER_ORDER_24DB, FILTER_ORDER_48DB)
  * @retval None
  */
void Crossover_SetFilterOrder(uint8_t order);

/**
  * @brief  Reset all filter states (clear history)
  * @retval None
  */
void Crossover_Reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __CROSSOVER_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
