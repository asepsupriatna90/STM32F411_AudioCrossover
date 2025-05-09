 /**
  ******************************************************************************
  * @file           : dynamics.h
  * @brief          : Header for dynamics.c file.
  *                   This file contains the common defines and functions for
  *                   dynamic audio processing (compressor and limiter).
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
#ifndef __DYNAMICS_H
#define __DYNAMICS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief State structure for dynamics processors
 * This maintains the necessary state between processing blocks
 */
typedef struct {
    float env;            /* Current envelope follower value */
    float gainReduction;  /* Current gain reduction in linear scale */
    float prevSample;     /* Previous sample value for peak detection */
} DynamicsState_t;

/**
 * @brief Compressor parameters structure
 */
typedef struct {
    float threshold;      /* Threshold level in dB, typically -60 to 0 */
    float ratio;          /* Compression ratio, typically 1 to 20 (1:1 to 20:1) */
    float attack;         /* Attack time in ms, typically 0.1 to 100 */
    float release;        /* Release time in ms, typically 10 to 1000 */
    float makeupGain;     /* Makeup gain in dB, typically 0 to 20 */
    uint8_t enabled;      /* 1: enabled, 0: bypassed */
    uint8_t autoMakeup;   /* 1: auto makeup gain enabled, 0: disabled */
    float kneeWidth;      /* Knee width in dB (0 = hard knee, >0 = soft knee) */
} CompressorParams_t;

/**
 * @brief Limiter parameters structure
 */
typedef struct {
    float threshold;      /* Threshold level in dB, typically -20 to 0 */
    float release;        /* Release time in ms, typically 10 to 1000 */
    uint8_t enabled;      /* 1: enabled, 0: bypassed */
    float lookAhead;      /* Look-ahead time in ms (for future implementation) */
} LimiterParams_t;

/**
 * @brief Compressor instance structure
 */
typedef struct {
    CompressorParams_t params;  /* Compressor parameters */
    DynamicsState_t state;      /* State variables */
    float attackCoef;           /* Pre-calculated attack coefficient */
    float releaseCoef;          /* Pre-calculated release coefficient */
    float sampleRate;           /* Sample rate for coefficient calculation */
} Compressor_t;

/**
 * @brief Limiter instance structure
 */
typedef struct {
    LimiterParams_t params;     /* Limiter parameters */
    DynamicsState_t state;      /* State variables */
    float releaseCoef;          /* Pre-calculated release coefficient */
    float sampleRate;           /* Sample rate for coefficient calculation */
} Limiter_t;

/* Exported constants --------------------------------------------------------*/
/* Default compressor settings */
#define COMPRESSOR_DEFAULT_THRESHOLD  -20.0f
#define COMPRESSOR_DEFAULT_RATIO       4.0f
#define COMPRESSOR_DEFAULT_ATTACK      5.0f
#define COMPRESSOR_DEFAULT_RELEASE    100.0f
#define COMPRESSOR_DEFAULT_MAKEUP      0.0f
#define COMPRESSOR_DEFAULT_KNEE        3.0f
#define COMPRESSOR_DEFAULT_ENABLED     1

/* Default limiter settings */
#define LIMITER_DEFAULT_THRESHOLD     -3.0f 
#define LIMITER_DEFAULT_RELEASE       50.0f
#define LIMITER_DEFAULT_ENABLED        1

/* Exported macros -----------------------------------------------------------*/
/* None */

/* Exported functions prototypes ---------------------------------------------*/
/* Compressor functions */
void Dynamics_CompressorInit(Compressor_t *comp, float sampleRate);
void Dynamics_CompressorSetParams(Compressor_t *comp, const CompressorParams_t *params);
void Dynamics_CompressorProcess(Compressor_t *comp, float *input, float *output, uint32_t size);
float Dynamics_CompressorProcessSample(Compressor_t *comp, float sample);
void Dynamics_CompressorReset(Compressor_t *comp);
float Dynamics_CompressorGetGainReduction(const Compressor_t *comp);

/* Limiter functions */
void Dynamics_LimiterInit(Limiter_t *lim, float sampleRate);
void Dynamics_LimiterSetParams(Limiter_t *lim, const LimiterParams_t *params);
void Dynamics_LimiterProcess(Limiter_t *lim, float *input, float *output, uint32_t size);
float Dynamics_LimiterProcessSample(Limiter_t *lim, float sample);
void Dynamics_LimiterReset(Limiter_t *lim);
float Dynamics_LimiterGetGainReduction(const Limiter_t *lim);

/* Utility functions */
float Dynamics_DetectPeak(float sample, float prevSample);
float Dynamics_DBToLinear(float dB);
float Dynamics_LinearToDB(float linear);

#ifdef __cplusplus
}
#endif

#endif /* __DYNAMICS_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
