 /**
  ******************************************************************************
  * @file           : delay.c
  * @brief          : Implementation of delay and phase adjustment module
  *                   for the Audio Crossover DSP Control Panel
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
#include "delay.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SAMPLE_RATE        48000   /* Audio sample rate in Hz */
#define MS_TO_SAMPLES(ms)  ((ms) * (SAMPLE_RATE / 1000.0f))

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static Delay_t delayInstance;

/* Private function prototypes -----------------------------------------------*/
static void Delay_UpdateParameters(void);
static int16_t Delay_GetSample(uint8_t channel, uint16_t index);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize the delay module
  * @retval None
  */
void Delay_Init(void)
{
  /* Initialize delay instance */
  memset(&delayInstance, 0, sizeof(Delay_t));
  
  /* Set default values */
  for (uint8_t i = 0; i < DELAY_NUM_CHANNELS; i++) {
    delayInstance.delaySamples[i] = 0.0f;
    delayInstance.phaseInvert[i] = 0;
    delayInstance.readIndex[i] = 0;
  }
  
  delayInstance.writeIndex = 0;
  delayInstance.needsUpdate = 0;
  
  /* Reset delay buffers */
  Delay_Reset();
  
  #ifdef DEBUG
  printf("Delay module initialized\r\n");
  #endif
}

/**
  * @brief  Process a block of audio through the delay module
  * @param  input   Pointer to input buffer containing audio to be delayed
  * @param  output  Pointer to output buffer to receive delayed audio
  * @param  size    Number of samples to process
  * @retval None
  */
void Delay_Process(int16_t *input, int16_t *output, uint16_t size)
{
  /* Update delay parameters if needed */
  if (delayInstance.needsUpdate) {
    Delay_UpdateParameters();
    delayInstance.needsUpdate = 0;
  }
  
  /* Process each sample */
  for (uint16_t i = 0; i < size; i++) {
    /* Process each channel */
    for (uint8_t ch = 0; ch < DELAY_NUM_CHANNELS; ch++) {
      /* Get the current sample from input (interleaved) */
      int16_t inputSample = input[i * DELAY_NUM_CHANNELS + ch];
      
      /* Store sample in delay buffer */
      delayInstance.buffer[ch][delayInstance.writeIndex] = inputSample;
      
      /* Read delayed sample from buffer */
      int16_t outputSample = Delay_GetSample(ch, delayInstance.readIndex[ch]);
      
      /* Apply phase inversion if needed */
      if (delayInstance.phaseInvert[ch]) {
        outputSample = -outputSample;
      }
      
      /* Write output sample (interleaved) */
      output[i * DELAY_NUM_CHANNELS + ch] = outputSample;
    }
    
    /* Update write index */
    delayInstance.writeIndex = (delayInstance.writeIndex + 1) % DELAY_BUFFER_SIZE;
    
    /* Update read indices for each channel */
    for (uint8_t ch = 0; ch < DELAY_NUM_CHANNELS; ch++) {
      delayInstance.readIndex[ch] = (delayInstance.readIndex[ch] + 1) % DELAY_BUFFER_SIZE;
    }
  }
}

/**
  * @brief  Set delay time for a specific channel
  * @param  channel Channel identifier (DELAY_CHANNEL_SUB, etc.)
  * @param  delayMs Delay time in milliseconds
  * @retval None
  */
void Delay_SetDelayTime(uint8_t channel, float delayMs)
{
  /* Check parameter validity */
  if (channel >= DELAY_NUM_CHANNELS) {
    return;
  }
  
  /* Clamp delay time to valid range */
  delayMs = CLAMP(delayMs, 0.0f, MAX_DELAY_MS);
  
  /* Convert delay time from milliseconds to samples */
  float delaySamples = MS_TO_SAMPLES(delayMs);
  
  /* Update delay samples */
  delayInstance.delaySamples[channel] = delaySamples;
  
  /* Mark for update */
  delayInstance.needsUpdate = 1;
  
  #ifdef DEBUG
  printf("Delay for channel %d set to %.2f ms (%.2f samples)\r\n", 
         channel, delayMs, delaySamples);
  #endif
}

/**
  * @brief  Set phase inversion for a specific channel
  * @param  channel Channel identifier (DELAY_CHANNEL_SUB, etc.)
  * @param  invert  1 to invert phase, 0 for normal phase
  * @retval None
  */
void Delay_SetPhaseInvert(uint8_t channel, uint8_t invert)
{
  /* Check parameter validity */
  if (channel >= DELAY_NUM_CHANNELS) {
    return;
  }
  
  /* Update phase inversion */
  delayInstance.phaseInvert[channel] = invert ? 1 : 0;
  
  #ifdef DEBUG
  printf("Phase inversion for channel %d set to %d\r\n", channel, invert);
  #endif
}

/**
  * @brief  Set delay settings from configuration structure
  * @param  settings Pointer to delay settings structure
  * @retval None
  */
void Delay_SetSettings(const DelaySettings_t *settings)
{
  /* Check if settings pointer is valid */
  if (settings == NULL) {
    return;
  }
  
  /* Apply settings to each channel */
  Delay_SetDelayTime(DELAY_CHANNEL_SUB, settings->subDelay);
  Delay_SetDelayTime(DELAY_CHANNEL_LOW, settings->lowDelay);
  Delay_SetDelayTime(DELAY_CHANNEL_MID, settings->midDelay);
  Delay_SetDelayTime(DELAY_CHANNEL_HIGH, settings->highDelay);
  
  Delay_SetPhaseInvert(DELAY_CHANNEL_SUB, settings->subPhaseInvert);
  Delay_SetPhaseInvert(DELAY_CHANNEL_LOW, settings->lowPhaseInvert);
  Delay_SetPhaseInvert(DELAY_CHANNEL_MID, settings->midPhaseInvert);
  Delay_SetPhaseInvert(DELAY_CHANNEL_HIGH, settings->highPhaseInvert);
  
  /* Update all parameters */
  Delay_UpdateParameters();
}

/**
  * @brief  Get current delay settings
  * @param  settings Pointer to delay settings structure to fill
  * @retval None
  */
void Delay_GetSettings(DelaySettings_t *settings)
{
  /* Check if settings pointer is valid */
  if (settings == NULL) {
    return;
  }
  
  /* Convert delay from samples to milliseconds */
  settings->subDelay = delayInstance.delaySamples[DELAY_CHANNEL_SUB] / (SAMPLE_RATE / 1000.0f);
  settings->lowDelay = delayInstance.delaySamples[DELAY_CHANNEL_LOW] / (SAMPLE_RATE / 1000.0f);
  settings->midDelay = delayInstance.delaySamples[DELAY_CHANNEL_MID] / (SAMPLE_RATE / 1000.0f);
  settings->highDelay = delayInstance.delaySamples[DELAY_CHANNEL_HIGH] / (SAMPLE_RATE / 1000.0f);
  
  /* Get phase inversion settings */
  settings->subPhaseInvert = delayInstance.phaseInvert[DELAY_CHANNEL_SUB];
  settings->lowPhaseInvert = delayInstance.phaseInvert[DELAY_CHANNEL_LOW];
  settings->midPhaseInvert = delayInstance.phaseInvert[DELAY_CHANNEL_MID];
  settings->highPhaseInvert = delayInstance.phaseInvert[DELAY_CHANNEL_HIGH];
}

/**
  * @brief  Reset all delay lines to zero
  * @retval None
  */
void Delay_Reset(void)
{
  /* Clear all delay buffers */
  for (uint8_t ch = 0; ch < DELAY_NUM_CHANNELS; ch++) {
    memset(delayInstance.buffer[ch], 0, DELAY_BUFFER_SIZE * sizeof(int16_t));
  }
  
  /* Reset indices */
  delayInstance.writeIndex = 0;
  for (uint8_t ch = 0; ch < DELAY_NUM_CHANNELS; ch++) {
    delayInstance.readIndex[ch] = 0;
  }
  
  #ifdef DEBUG
  printf("Delay buffers reset\r\n");
  #endif
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Update delay parameters after settings change
  * @retval None
  */
static void Delay_UpdateParameters(void)
{
  /* Calculate read indices for each channel based on current write index and delay */
  for (uint8_t ch = 0; ch < DELAY_NUM_CHANNELS; ch++) {
    /* Calculate the read position based on write position and delay */
    int32_t offset = (int32_t)delayInstance.delaySamples[ch];
    
    /* Calculate read index with buffer wrap-around */
    int32_t readPos = (int32_t)delayInstance.writeIndex - offset;
    
    /* Handle negative index (wrap around) */
    while (readPos < 0) {
      readPos += DELAY_BUFFER_SIZE;
    }
    
    /* Set read index */
    delayInstance.readIndex[ch] = (uint16_t)(readPos % DELAY_BUFFER_SIZE);
  }
}

/**
  * @brief  Get a sample from the delay buffer with proper handling of non-integer delays
  * @param  channel Channel identifier
  * @param  index   Buffer index to read from
  * @retval Delayed sample value
  */
static int16_t Delay_GetSample(uint8_t channel, uint16_t index)
{
  /* Extract the integer and fractional parts of the delay */
  float delaySamples = delayInstance.delaySamples[channel];
  int32_t intDelay = (int32_t)delaySamples;
  float fraction = delaySamples - intDelay;
  
  /* If no fractional part, just return the sample at the index */
  if (fraction < 0.001f) {
    return delayInstance.buffer[channel][index];
  }
  
  /* For fractional delays, perform linear interpolation between adjacent samples */
  int16_t sample1 = delayInstance.buffer[channel][index];
  int16_t sample2 = delayInstance.buffer[channel][(index + 1) % DELAY_BUFFER_SIZE];
  
  /* Linear interpolation formula: sample1 * (1 - fraction) + sample2 * fraction */
  return (int16_t)(sample1 * (1.0f - fraction) + sample2 * fraction);
}

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
