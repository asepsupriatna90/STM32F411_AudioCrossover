 /**
  ******************************************************************************
  * @file           : audio_processing.c
  * @brief          : Implementation of the audio processing chain
  *                   This file implements the core audio processing for the
  *                   Audio Crossover DSP Control Panel project.
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
#include "audio_processing.h"
#include "crossover.h"
#include "compressor.h"
#include "limiter.h"
#include "delay.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define AUDIO_TEMP_BUFFER_SIZE (AUDIO_BUFFER_SIZE * 4)  /* 4x for crossover bands */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Audio processing statistics */
static AudioProcessingStats_t audioStats = {0};

/* Temporary buffers for band-split audio processing */
static float tempBufferL[AUDIO_TEMP_BUFFER_SIZE];
static float tempBufferR[AUDIO_TEMP_BUFFER_SIZE];

/* Band-specific processing buffers */
static float bandBufferL[NUM_BANDS][AUDIO_BUFFER_SIZE/2]; /* Stereo -> mono */
static float bandBufferR[NUM_BANDS][AUDIO_BUFFER_SIZE/2];

/* Debug timing measurement */
static uint32_t processingStartTime = 0;
static uint32_t processingEndTime = 0;

/* Bypass control */
static uint8_t bypassEnabled = 0;

/* Private function prototypes -----------------------------------------------*/
static void ConvertToFloat(const int16_t *input, float *outputL, float *outputR, uint16_t length);
static void ConvertToInt16(const float *inputL, const float *inputR, int16_t *output, uint16_t length);
static void UpdatePeakLevels(const float *bufferL, const float *bufferR, uint16_t length, float *peakL, float *peakR);
static void MixBands(float *subL, float *lowL, float *midL, float *highL,
                    float *subR, float *lowR, float *midR, float *highR,
                    float *outputL, float *outputR, uint16_t length);
static void ApplyGain(float *buffer, uint16_t length, float gainDB);
static void GetProcessingTime(void);

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim2; /* Used for timing measurement */

/**
  * @brief  Initialize audio processing modules
  * @retval None
  */
void AudioProcessing_Init(void)
{
  /* Initialize statistics */
  memset(&audioStats, 0, sizeof(AudioProcessingStats_t));
  
  /* Initialize processing buffers */
  memset(tempBufferL, 0, sizeof(tempBufferL));
  memset(tempBufferR, 0, sizeof(tempBufferR));
  
  for (int band = 0; band < NUM_BANDS; band++) {
    memset(bandBufferL[band], 0, sizeof(bandBufferL[band]));
    memset(bandBufferR[band], 0, sizeof(bandBufferR[band]));
  }

  /* Initialize bypass mode */
  bypassEnabled = 0;
  
  #ifdef DEBUG
  printf("Audio processing initialized\r\n");
  #endif
}

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
    SystemSettings_t *pSettings)
{
  uint16_t monoFrames = AUDIO_BUFFER_SIZE / 2; /* Convert from stereo samples to mono frames */
  
  /* Start timing measurement */
  processingStartTime = HAL_GetTick();
  
  /* If bypass is enabled, just copy input to output */
  if (bypassEnabled) {
    memcpy(pOutputBuffer->data, pInputBuffer->data, AUDIO_BUFFER_SIZE * sizeof(int16_t));
    
    /* Update peak levels for display purposes */
    float dummyPeakL, dummyPeakR;
    ConvertToFloat(pInputBuffer->data, tempBufferL, tempBufferR, monoFrames);
    UpdatePeakLevels(tempBufferL, tempBufferR, monoFrames, &audioStats.inputPeakLevel[CHANNEL_LEFT], &audioStats.inputPeakLevel[CHANNEL_RIGHT]);
    audioStats.outputPeakLevel[CHANNEL_LEFT] = audioStats.inputPeakLevel[CHANNEL_LEFT];
    audioStats.outputPeakLevel[CHANNEL_RIGHT] = audioStats.inputPeakLevel[CHANNEL_RIGHT];
    
    /* End timing measurement */
    processingEndTime = HAL_GetTick();
    audioStats.processingTime = processingEndTime - processingStartTime;
    
    return;
  }
  
  /* Convert input samples from int16_t to float for DSP processing */
  ConvertToFloat(pInputBuffer->data, tempBufferL, tempBufferR, monoFrames);
  
  /* Update input peak levels for metering */
  UpdatePeakLevels(tempBufferL, tempBufferR, monoFrames, 
                  &audioStats.inputPeakLevel[CHANNEL_LEFT], 
                  &audioStats.inputPeakLevel[CHANNEL_RIGHT]);
  
  /* Apply crossover to split the signal into bands */
  Crossover_Process(tempBufferL, tempBufferR, monoFrames,
                   bandBufferL[BAND_SUB], bandBufferR[BAND_SUB],
                   bandBufferL[BAND_LOW], bandBufferR[BAND_LOW],
                   bandBufferL[BAND_MID], bandBufferR[BAND_MID],
                   bandBufferL[BAND_HIGH], bandBufferR[BAND_HIGH],
                   &pSettings->crossover);
  
  /* Apply band-specific processing for each band */
  for (int band = 0; band < NUM_BANDS; band++) {
    float *leftBuffer = bandBufferL[band];
    float *rightBuffer = bandBufferR[band];
    
    /* Apply band-specific gain */
    float bandGain = 0.0f;
    uint8_t bandMute = 0;
    
    /* Get the appropriate gain and mute settings for this band */
    switch (band) {
      case BAND_SUB:
        bandGain = pSettings->crossover.subGain;
        bandMute = pSettings->crossover.subMute;
        break;
      case BAND_LOW:
        bandGain = pSettings->crossover.lowGain;
        bandMute = pSettings->crossover.lowMute;
        break;
      case BAND_MID:
        bandGain = pSettings->crossover.midGain;
        bandMute = pSettings->crossover.midMute;
        break;
      case BAND_HIGH:
        bandGain = pSettings->crossover.highGain;
        bandMute = pSettings->crossover.highMute;
        break;
    }
    
    /* If band is muted, zero out the buffer and skip processing */
    if (bandMute) {
      memset(leftBuffer, 0, monoFrames * sizeof(float));
      memset(rightBuffer, 0, monoFrames * sizeof(float));
      continue;
    }
    
    /* Apply band gain */
    ApplyGain(leftBuffer, monoFrames, bandGain);
    ApplyGain(rightBuffer, monoFrames, bandGain);
    
    /* Update peak levels for this band for metering */
    UpdatePeakLevels(leftBuffer, rightBuffer, monoFrames,
                    &audioStats.bandPeakLevel[band][CHANNEL_LEFT],
                    &audioStats.bandPeakLevel[band][CHANNEL_RIGHT]);
    
    /* Apply compressor for dynamic control */
    float compressionAmount = 0.0f;
    struct CompressorSettings_t *compSettings = &pSettings->compressor;
    
    /* Select the appropriate band compressor settings */
    struct {
      float threshold;
      float ratio;
      float attack;
      float release;
      float makeupGain;
      uint8_t enabled;
    } bandComp;
    
    switch (band) {
      case BAND_SUB:
        bandComp = compSettings->sub;
        break;
      case BAND_LOW:
        bandComp = compSettings->low;
        break;
      case BAND_MID:
        bandComp = compSettings->mid;
        break;
      case BAND_HIGH:
        bandComp = compSettings->high;
        break;
    }
    
    /* Apply compressor if enabled */
    if (bandComp.enabled) {
      Compressor_Process(leftBuffer, rightBuffer, monoFrames,
                        bandComp.threshold, bandComp.ratio,
                        bandComp.attack, bandComp.release,
                        bandComp.makeupGain, band, &compressionAmount);
    }
    
    /* Store compression amount for metering */
    audioStats.compressionAmount[band] = compressionAmount;
    
    /* Apply limiter for overload protection */
    struct LimiterSettings_t *limSettings = &pSettings->limiter;
    float limiterGainReduction = 0.0f;
    
    /* Select the appropriate band limiter settings */
    struct {
      float threshold;
      float release;
      uint8_t enabled;
    } bandLim;
    
    switch (band) {
      case BAND_SUB:
        bandLim = limSettings->sub;
        break;
      case BAND_LOW:
        bandLim = limSettings->low;
        break;
      case BAND_MID:
        bandLim = limSettings->mid;
        break;
      case BAND_HIGH:
        bandLim = limSettings->high;
        break;
    }
    
    /* Apply limiter if enabled */
    if (bandLim.enabled) {
      Limiter_Process(leftBuffer, rightBuffer, monoFrames,
                     bandLim.threshold, bandLim.release,
                     band, &limiterGainReduction);
    }
    
    /* Store limiter activity for metering */
    audioStats.limiterActivity[band] = limiterGainReduction;
    
    /* Apply delay and phase adjustments */
    struct DelaySettings_t *delaySettings = &pSettings->delay;
    float delayMs = 0.0f;
    uint8_t phaseInvert = 0;
    
    /* Select the appropriate delay settings for this band */
    switch (band) {
      case BAND_SUB:
        delayMs = delaySettings->subDelay;
        phaseInvert = delaySettings->subPhaseInvert;
        break;
      case BAND_LOW:
        delayMs = delaySettings->lowDelay;
        phaseInvert = delaySettings->lowPhaseInvert;
        break;
      case BAND_MID:
        delayMs = delaySettings->midDelay;
        phaseInvert = delaySettings->midPhaseInvert;
        break;
      case BAND_HIGH:
        delayMs = delaySettings->highDelay;
        phaseInvert = delaySettings->highPhaseInvert;
        break;
    }
    
    /* Apply delay and phase adjustments */
    Delay_Process(leftBuffer, rightBuffer, monoFrames, 
                 delayMs, phaseInvert, band);
  }
  
  /* Mix all bands back together */
  MixBands(bandBufferL[BAND_SUB], bandBufferL[BAND_LOW], bandBufferL[BAND_MID], bandBufferL[BAND_HIGH],
           bandBufferR[BAND_SUB], bandBufferR[BAND_LOW], bandBufferR[BAND_MID], bandBufferR[BAND_HIGH],
           tempBufferL, tempBufferR, monoFrames);
  
  /* Update output peak levels for metering */
  UpdatePeakLevels(tempBufferL, tempBufferR, monoFrames,
                  &audioStats.outputPeakLevel[CHANNEL_LEFT],
                  &audioStats.outputPeakLevel[CHANNEL_RIGHT]);
  
  /* Convert float back to int16_t for output */
  ConvertToInt16(tempBufferL, tempBufferR, pOutputBuffer->data, monoFrames);
  
  /* End timing measurement */
  GetProcessingTime();
}

/**
  * @brief  Get current audio processing statistics
  * @param  pStats Pointer to statistics structure to fill
  * @retval None
  */
void AudioProcessing_GetStats(AudioProcessingStats_t *pStats)
{
  if (pStats != NULL) {
    /* Copy current statistics */
    memcpy(pStats, &audioStats, sizeof(AudioProcessingStats_t));
  }
}

/**
  * @brief  Reset audio processing state (e.g., after settings change)
  * @retval None
  */
void AudioProcessing_Reset(void)
{
  /* Reset internal states of all DSP modules */
  Crossover_Reset();
  Compressor_Reset();
  Limiter_Reset();
  Delay_Reset();
  
  /* Clear statistics */
  memset(&audioStats, 0, sizeof(AudioProcessingStats_t));
  
  #ifdef DEBUG
  printf("Audio processing reset\r\n");
  #endif
}

/**
  * @brief  Enable or disable bypass mode (raw audio pass-through)
  * @param  enable 1 to enable bypass, 0 to disable
  * @retval None
  */
void AudioProcessing_SetBypass(uint8_t enable)
{
  bypassEnabled = enable ? 1 : 0;
  
  #ifdef DEBUG
  printf("Audio processing bypass %s\r\n", bypassEnabled ? "enabled" : "disabled");
  #endif
}

/**
  * @brief  Get current bypass mode status
  * @retval 1 if bypass is enabled, 0 otherwise
  */
uint8_t AudioProcessing_GetBypass(void)
{
  return bypassEnabled;
}

/* Private Functions ---------------------------------------------------------*/

/**
  * @brief  Convert interleaved int16_t stereo samples to separate float arrays
  * @note   Assumes input buffer has 2*length elements (left/right interleaved)
  * @param  input Input buffer with interleaved stereo samples
  * @param  outputL Output buffer for left channel (float)
  * @param  outputR Output buffer for right channel (float)
  * @param  length Number of frames (stereo pairs) to convert
  * @retval None
  */
static void ConvertToFloat(const int16_t *input, float *outputL, float *outputR, uint16_t length)
{
  const float scale = 1.0f / MAX_SAMPLE_VALUE;
  
  for (uint16_t i = 0; i < length; i++) {
    outputL[i] = input[2*i] * scale;         /* Left channel */
    outputR[i] = input[2*i + 1] * scale;     /* Right channel */
  }
}

/**
  * @brief  Convert separate float arrays back to interleaved int16_t stereo samples
  * @param  inputL Input buffer with left channel samples (float)
  * @param  inputR Input buffer with right channel samples (float)
  * @param  output Output buffer for interleaved stereo samples
  * @param  length Number of frames (stereo pairs) to convert
  * @retval None
  */
static void ConvertToInt16(const float *inputL, const float *inputR, int16_t *output, uint16_t length)
{
  uint32_t clippingCount = 0;
  
  for (uint16_t i = 0; i < length; i++) {
    float leftSample = inputL[i] * MAX_SAMPLE_VALUE;
    float rightSample = inputR[i] * MAX_SAMPLE_VALUE;
    
    /* Check for clipping */
    if (leftSample > MAX_SAMPLE_VALUE || leftSample < MIN_SAMPLE_VALUE) {
      clippingCount++;
      leftSample = CLAMP(leftSample, MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);
    }
    
    if (rightSample > MAX_SAMPLE_VALUE || rightSample < MIN_SAMPLE_VALUE) {
      clippingCount++;
      rightSample = CLAMP(rightSample, MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);
    }
    
    /* Convert to int16_t */
    output[2*i] = (int16_t)leftSample;       /* Left channel */
    output[2*i + 1] = (int16_t)rightSample;  /* Right channel */
  }
  
  /* Update clipping count in statistics */
  audioStats.clippingCount += clippingCount;
}

/**
  * @brief  Update peak level measurements for audio monitoring
  * @param  bufferL Left channel buffer
  * @param  bufferR Right channel buffer
  * @param  length Number of samples to analyze
  * @param  peakL Pointer to store left channel peak level
  * @param  peakR Pointer to store right channel peak level
  * @retval None
  */
static void UpdatePeakLevels(const float *bufferL, const float *bufferR, uint16_t length, 
                           float *peakL, float *peakR)
{
  float maxL = 0.0f;
  float maxR = 0.0f;
  
  /* Find peak absolute value */
  for (uint16_t i = 0; i < length; i++) {
    float absL = fabsf(bufferL[i]);
    float absR = fabsf(bufferR[i]);
    
    if (absL > maxL) maxL = absL;
    if (absR > maxR) maxR = absR;
  }
  
  /* Apply peak decay for smoother metering */
  const float decay = 0.8f;  /* Decay factor for peak hold */
  
  if (maxL > *peakL) {
    *peakL = maxL;  /* New peak */
  } else {
    *peakL *= decay;  /* Decay old peak */
  }
  
  if (maxR > *peakR) {
    *peakR = maxR;  /* New peak */
  } else {
    *peakR *= decay;  /* Decay old peak */
  }
}

/**
  * @brief  Mix all frequency bands back together into final output
  * @param  subL Sub band, left channel
  * @param  lowL Low band, left channel
  * @param  midL Mid band, left channel
  * @param  highL High band, left channel
  * @param  subR Sub band, right channel
  * @param  lowR Low band, right channel
  * @param  midR Mid band, right channel
  * @param  highR High band, right channel
  * @param  outputL Output left channel
  * @param  outputR Output right channel
  * @param  length Number of samples per buffer
  * @retval None
  */
static void MixBands(float *subL, float *lowL, float *midL, float *highL,
                    float *subR, float *lowR, float *midR, float *highR,
                    float *outputL, float *outputR, uint16_t length)
{
  for (uint16_t i = 0; i < length; i++) {
    /* Sum all bands for left channel */
    outputL[i] = subL[i] + lowL[i] + midL[i] + highL[i];
    
    /* Sum all bands for right channel */
    outputR[i] = subR[i] + lowR[i] + midR[i] + highR[i];
  }
}

/**
  * @brief  Apply gain to audio buffer (in dB)
  * @param  buffer Audio buffer to process
  * @param  length Number of samples
  * @param  gainDB Gain in decibels
  * @retval None
  */
static void ApplyGain(float *buffer, uint16_t length, float gainDB)
{
  /* Convert dB to linear gain */
  float gain = DB_TO_LINEAR(gainDB);
  
  /* Apply gain to all samples */
  for (uint16_t i = 0; i < length; i++) {
    buffer[i] *= gain;
  }
}

/**
  * @brief  Calculate processing time for one audio block
  * @retval None
  */
static void GetProcessingTime(void)
{
  processingEndTime = HAL_GetTick();
  
  /* Calculate time in microseconds */
  audioStats.processingTime = (processingEndTime - processingStartTime) * 1000;
}

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
