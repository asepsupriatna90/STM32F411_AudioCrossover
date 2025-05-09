 /**
  ******************************************************************************
  * @file           : dynamics.c
  * @brief          : Implementation of dynamic audio processing functions
  *                   (compressor and limiter).
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
#include "dynamics.h"
#include <math.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define DB_TO_LINEAR(x) (powf(10.0f, (x) / 20.0f))
#define LINEAR_TO_DB(x) (20.0f * log10f(MAX(x, 0.00001f)))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

/* Time constant conversion (ms to coefficient for lowpass filter) */
#define MS_TO_COEF(time_ms, sample_rate) (time_ms <= 0.0f ? 0.0f : expf(-1.0f / ((time_ms * 0.001f) * sample_rate)))

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static float calculateCompressorGain(const Compressor_t *comp, float inputLevel);

/* Private user code ---------------------------------------------------------*/

/**
  * @brief  Initialize a compressor instance
  * @param  comp: Pointer to compressor instance
  * @param  sampleRate: Sample rate in Hz
  * @retval None
  */
void Dynamics_CompressorInit(Compressor_t *comp, float sampleRate)
{
    /* Initialize state */
    comp->state.env = 0.0f;
    comp->state.gainReduction = 1.0f;
    comp->state.prevSample = 0.0f;
    comp->sampleRate = sampleRate;
    
    /* Set default parameters */
    CompressorParams_t defaultParams = {
        .threshold = COMPRESSOR_DEFAULT_THRESHOLD,
        .ratio = COMPRESSOR_DEFAULT_RATIO,
        .attack = COMPRESSOR_DEFAULT_ATTACK,
        .release = COMPRESSOR_DEFAULT_RELEASE,
        .makeupGain = COMPRESSOR_DEFAULT_MAKEUP,
        .enabled = COMPRESSOR_DEFAULT_ENABLED,
        .autoMakeup = 0,
        .kneeWidth = COMPRESSOR_DEFAULT_KNEE
    };
    
    Dynamics_CompressorSetParams(comp, &defaultParams);
}

/**
  * @brief  Set compressor parameters
  * @param  comp: Pointer to compressor instance
  * @param  params: Pointer to parameters structure
  * @retval None
  */
void Dynamics_CompressorSetParams(Compressor_t *comp, const CompressorParams_t *params)
{
    /* Copy parameters */
    comp->params = *params;
    
    /* Calculate coefficients */
    comp->attackCoef = MS_TO_COEF(comp->params.attack, comp->sampleRate);
    comp->releaseCoef = MS_TO_COEF(comp->params.release, comp->sampleRate);
}

/**
  * @brief  Process a buffer of audio samples through the compressor
  * @param  comp: Pointer to compressor instance
  * @param  input: Pointer to input buffer
  * @param  output: Pointer to output buffer (can be the same as input for in-place processing)
  * @param  size: Number of samples to process
  * @retval None
  */
void Dynamics_CompressorProcess(Compressor_t *comp, float *input, float *output, uint32_t size)
{
    /* If disabled, just copy input to output if they're different buffers */
    if (!comp->params.enabled) {
        if (input != output) {
            for (uint32_t i = 0; i < size; i++) {
                output[i] = input[i];
            }
        }
        return;
    }
    
    float makeupGainLinear = DB_TO_LINEAR(comp->params.makeupGain);
    
    /* Process each sample */
    for (uint32_t i = 0; i < size; i++) {
        output[i] = Dynamics_CompressorProcessSample(comp, input[i]) * makeupGainLinear;
    }
}

/**
  * @brief  Process a single audio sample through the compressor
  * @param  comp: Pointer to compressor instance
  * @param  sample: Input sample
  * @retval Processed output sample
  */
float Dynamics_CompressorProcessSample(Compressor_t *comp, float sample)
{
    if (!comp->params.enabled) {
        return sample;
    }
    
    /* Calculate the absolute value for envelope detection */
    float inputAbs = fabsf(sample);
    
    /* Peak detection */
    float peakValue = Dynamics_DetectPeak(inputAbs, comp->state.prevSample);
    comp->state.prevSample = inputAbs;
    
    /* Convert to dB for level detection */
    float inputLevel = LINEAR_TO_DB(peakValue);
    
    /* Envelope follower with different attack/release times */
    if (inputLevel > comp->state.env) {
        /* Attack phase */
        comp->state.env = comp->attackCoef * comp->state.env + (1.0f - comp->attackCoef) * inputLevel;
    } else {
        /* Release phase */
        comp->state.env = comp->releaseCoef * comp->state.env + (1.0f - comp->releaseCoef) * inputLevel;
    }
    
    /* Calculate gain reduction based on transfer function */
    float gain = calculateCompressorGain(comp, comp->state.env);
    
    /* Apply smoothing to gain reduction to avoid artifacts */
    if (gain < comp->state.gainReduction) {
        /* Gain is decreasing - use attack time */
        comp->state.gainReduction = comp->attackCoef * comp->state.gainReduction + 
                                   (1.0f - comp->attackCoef) * gain;
    } else {
        /* Gain is increasing - use release time */
        comp->state.gainReduction = comp->releaseCoef * comp->state.gainReduction + 
                                   (1.0f - comp->releaseCoef) * gain;
    }
    
    /* Apply gain reduction */
    return sample * comp->state.gainReduction;
}

/**
  * @brief  Reset compressor state
  * @param  comp: Pointer to compressor instance
  * @retval None
  */
void Dynamics_CompressorReset(Compressor_t *comp)
{
    comp->state.env = 0.0f;
    comp->state.gainReduction = 1.0f;
    comp->state.prevSample = 0.0f;
}

/**
  * @brief  Get current gain reduction in dB
  * @param  comp: Pointer to compressor instance
  * @retval Gain reduction in dB (negative value)
  */
float Dynamics_CompressorGetGainReduction(const Compressor_t *comp)
{
    return LINEAR_TO_DB(comp->state.gainReduction);
}

/**
  * @brief  Calculate compressor gain from input level
  * @param  comp: Pointer to compressor instance
  * @param  inputLevel: Input level in dB
  * @retval Gain factor (linear scale)
  */
static float calculateCompressorGain(const Compressor_t *comp, float inputLevel)
{
    float threshold = comp->params.threshold;
    float ratio = comp->params.ratio;
    float kneeWidth = comp->params.kneeWidth;
    float halfKneeWidth = kneeWidth * 0.5f;
    float gain = 1.0f;
    
    if (kneeWidth > 0.0f && inputLevel > (threshold - halfKneeWidth) && 
        inputLevel < (threshold + halfKneeWidth)) {
        /* Soft knee region */
        float kneeInput = inputLevel - threshold + halfKneeWidth;
        float kneeRatio = 1.0f + ((ratio - 1.0f) * kneeInput * kneeInput) / (2.0f * kneeWidth);
        float gainReduction = (threshold - inputLevel) / kneeRatio;
        gain = DB_TO_LINEAR(gainReduction);
    } else if (inputLevel > threshold) {
        /* Above threshold - apply compression */
        float gainReduction = (threshold - inputLevel) / ratio + (inputLevel - threshold);
        gainReduction = threshold - inputLevel + (inputLevel - threshold) / ratio;
        gain = DB_TO_LINEAR(gainReduction);
    }
    
    /* Limit the gain to a reasonable minimum to avoid underflows */
    return MAX(gain, 0.001f);
}

/**
  * @brief  Initialize a limiter instance
  * @param  lim: Pointer to limiter instance
  * @param  sampleRate: Sample rate in Hz
  * @retval None
  */
void Dynamics_LimiterInit(Limiter_t *lim, float sampleRate)
{
    /* Initialize state */
    lim->state.env = 0.0f;
    lim->state.gainReduction = 1.0f;
    lim->state.prevSample = 0.0f;
    lim->sampleRate = sampleRate;
    
    /* Set default parameters */
    LimiterParams_t defaultParams = {
        .threshold = LIMITER_DEFAULT_THRESHOLD,
        .release = LIMITER_DEFAULT_RELEASE,
        .enabled = LIMITER_DEFAULT_ENABLED,
        .lookAhead = 0.0f /* Not implemented yet */
    };
    
    Dynamics_LimiterSetParams(lim, &defaultParams);
}

/**
  * @brief  Set limiter parameters
  * @param  lim: Pointer to limiter instance
  * @param  params: Pointer to parameters structure
  * @retval None
  */
void Dynamics_LimiterSetParams(Limiter_t *lim, const LimiterParams_t *params)
{
    /* Copy parameters */
    lim->params = *params;
    
    /* Calculate coefficients */
    /* For limiter, use very fast attack (0.1ms) but configurable release */
    float attackTime = 0.1f; /* Fixed fast attack for limiter */
    float attackCoef = MS_TO_COEF(attackTime, lim->sampleRate);
    lim->releaseCoef = MS_TO_COEF(lim->params.release, lim->sampleRate);
}

/**
  * @brief  Process a buffer of audio samples through the limiter
  * @param  lim: Pointer to limiter instance
  * @param  input: Pointer to input buffer
  * @param  output: Pointer to output buffer (can be the same as input for in-place processing)
  * @param  size: Number of samples to process
  * @retval None
  */
void Dynamics_LimiterProcess(Limiter_t *lim, float *input, float *output, uint32_t size)
{
    /* If disabled, just copy input to output if they're different buffers */
    if (!lim->params.enabled) {
        if (input != output) {
            for (uint32_t i = 0; i < size; i++) {
                output[i] = input[i];
            }
        }
        return;
    }
    
    /* Process each sample */
    for (uint32_t i = 0; i < size; i++) {
        output[i] = Dynamics_LimiterProcessSample(lim, input[i]);
    }
}

/**
  * @brief  Process a single audio sample through the limiter
  * @param  lim: Pointer to limiter instance
  * @param  sample: Input sample
  * @retval Processed output sample
  */
float Dynamics_LimiterProcessSample(Limiter_t *lim, float sample)
{
    if (!lim->params.enabled) {
        return sample;
    }
    
    /* Calculate the absolute value for envelope detection */
    float inputAbs = fabsf(sample);
    
    /* Peak detection */
    float peakValue = Dynamics_DetectPeak(inputAbs, lim->state.prevSample);
    lim->state.prevSample = inputAbs;
    
    /* Convert to dB for level detection */
    float inputLevel = LINEAR_TO_DB(peakValue);
    
    /* Fast peak envelope follower for limiter - specialized for very fast attack */
    if (inputLevel > lim->state.env) {
        /* Instant attack for limiter */
        lim->state.env = inputLevel;
    } else {
        /* Normal release */
        lim->state.env = lim->releaseCoef * lim->state.env + 
                        (1.0f - lim->releaseCoef) * inputLevel;
    }
    
    /* Calculate gain reduction - hard knee for limiter */
    float gainReduction = 0.0f;
    if (lim->state.env > lim->params.threshold) {
        gainReduction = lim->params.threshold - lim->state.env;
    }
    
    /* Convert from dB to linear gain */
    float targetGain = gainReduction > 0.0f ? 1.0f : DB_TO_LINEAR(gainReduction);
    
    /* Apply smoothing only to release to avoid artifacts */
    if (targetGain < lim->state.gainReduction) {
        /* Gain is decreasing - use instant attack */
        lim->state.gainReduction = targetGain;
    } else {
        /* Gain is increasing - use release time */
        lim->state.gainReduction = lim->releaseCoef * lim->state.gainReduction + 
                                 (1.0f - lim->releaseCoef) * targetGain;
    }
    
    /* Apply gain reduction */
    return sample * lim->state.gainReduction;
}

/**
  * @brief  Reset limiter state
  * @param  lim: Pointer to limiter instance
  * @retval None
  */
void Dynamics_LimiterReset(Limiter_t *lim)
{
    lim->state.env = 0.0f;
    lim->state.gainReduction = 1.0f;
    lim->state.prevSample = 0.0f;
}

/**
  * @brief  Get current gain reduction in dB
  * @param  lim: Pointer to limiter instance
  * @retval Gain reduction in dB (negative value)
  */
float Dynamics_LimiterGetGainReduction(const Limiter_t *lim)
{
    return LINEAR_TO_DB(lim->state.gainReduction);
}

/**
  * @brief  Peak detector function
  * @param  sample: Current sample absolute value
  * @param  prevSample: Previous sample absolute value
  * @retval Peak value
  */
float Dynamics_DetectPeak(float sample, float prevSample)
{
    /* Simple peak detector - can be extended with more sophisticated algorithms */
    return MAX(sample, prevSample);
}

/**
  * @brief  Convert dB value to linear scale
  * @param  dB: Value in decibels
  * @retval Linear scale value
  */
float Dynamics_DBToLinear(float dB)
{
    return DB_TO_LINEAR(dB);
}

/**
  * @brief  Convert linear scale value to dB
  * @param  linear: Linear scale value
  * @retval Value in decibels
  */
float Dynamics_LinearToDB(float linear)
{
    return LINEAR_TO_DB(linear);
}

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
