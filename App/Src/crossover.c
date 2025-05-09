 /**
  ******************************************************************************
  * @file           : crossover.c
  * @brief          : Implementation of audio crossover filter functionality
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
#include "crossover.h"
#include "main.h"
#include <math.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  Biquad filter structure for second-order IIR filters
  */
typedef struct {
    float a0, a1, a2;  // Numerator coefficients
    float b0, b1, b2;  // Denominator coefficients
    float x1, x2;      // Input delay elements
    float y1, y2;      // Output delay elements
} BiquadFilter_t;

/**
  * @brief  Linked filters structure for higher-order filters
  */
typedef struct {
    BiquadFilter_t* filters;  // Array of biquad filters
    uint8_t filterCount;      // Number of filters (order/2)
} FilterChain_t;

/**
  * @brief  Complete crossover filter bank structure
  */
typedef struct {
    // Low-pass filter for subwoofer band (0Hz - lowCutoff)
    FilterChain_t subLowPass;
    
    // Band-pass filter for low band (lowCutoff - midCutoff)
    FilterChain_t lowLowPass;
    FilterChain_t lowHighPass;
    
    // Band-pass filter for mid band (midCutoff - highCutoff)
    FilterChain_t midLowPass;
    FilterChain_t midHighPass;
    
    // High-pass filter for high band (highCutoff - Nyquist)
    FilterChain_t highHighPass;
    
    // Current filter settings
    float lowCutoff;   // Hz
    float midCutoff;   // Hz
    float highCutoff;  // Hz
    
    // Current gain settings in linear scale
    float subGain;
    float lowGain;
    float midGain;
    float highGain;
    
    // Filter type and order
    uint8_t filterType;  // 0: Butterworth, 1: Linkwitz-Riley
    uint8_t filterOrder; // 2, 4, or 8
    
    // Band mute flags
    uint8_t subMute;
    uint8_t lowMute;
    uint8_t midMute;
    uint8_t highMute;
    
    // Sample rate
    float sampleRate;
} CrossoverFilters_t;

/* Private constants ---------------------------------------------------------*/
#define PI 3.14159265358979323846f
#define SQRT2 1.4142135623730951f

/* Default sample rate if not specified */
#define DEFAULT_SAMPLE_RATE 48000.0f

/* Default filter settings */
#define DEFAULT_LOW_CUTOFF  100.0f
#define DEFAULT_MID_CUTOFF  1000.0f
#define DEFAULT_HIGH_CUTOFF 5000.0f
#define DEFAULT_FILTER_TYPE 1  // Linkwitz-Riley
#define DEFAULT_FILTER_ORDER 4 // 4th order (24 dB/octave)

/* Max filter order supported */
#define MAX_FILTER_ORDER 8

/* Private variables ---------------------------------------------------------*/
static CrossoverFilters_t crossoverFilters;
static CrossoverSettings_t currentSettings;

/* Buffer for each band's processed audio */
static float subBuffer[AUDIO_BUFFER_SIZE];
static float lowBuffer[AUDIO_BUFFER_SIZE];
static float midBuffer[AUDIO_BUFFER_SIZE];
static float highBuffer[AUDIO_BUFFER_SIZE];

/* Biquad filter instances */
static BiquadFilter_t subLowPassFilters[MAX_FILTER_ORDER/2];
static BiquadFilter_t lowLowPassFilters[MAX_FILTER_ORDER/2];
static BiquadFilter_t lowHighPassFilters[MAX_FILTER_ORDER/2];
static BiquadFilter_t midLowPassFilters[MAX_FILTER_ORDER/2];
static BiquadFilter_t midHighPassFilters[MAX_FILTER_ORDER/2];
static BiquadFilter_t highHighPassFilters[MAX_FILTER_ORDER/2];

/* Private function prototypes -----------------------------------------------*/
static void CalculateFilterCoefficients(void);
static void CalculateButterworthCoefficients(BiquadFilter_t* filter, float frequency, float q, uint8_t type);
static void CalculateLinkwitzRileyCoefficients(BiquadFilter_t* filter, float frequency, uint8_t type);
static float ProcessBiquad(BiquadFilter_t* filter, float input);
static float ProcessFilterChain(FilterChain_t* chain, float input);
static void ResetFilter(BiquadFilter_t* filter);
static void ResetAllFilters(void);

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Initialize the crossover module
  * @param  None
  * @retval None
  */
void Crossover_Init(void)
{
    // Initialize filter chains
    crossoverFilters.subLowPass.filters = subLowPassFilters;
    crossoverFilters.lowLowPass.filters = lowLowPassFilters;
    crossoverFilters.lowHighPass.filters = lowHighPassFilters;
    crossoverFilters.midLowPass.filters = midLowPassFilters;
    crossoverFilters.midHighPass.filters = midHighPassFilters;
    crossoverFilters.highHighPass.filters = highHighPassFilters;
    
    // Set default values
    crossoverFilters.lowCutoff = DEFAULT_LOW_CUTOFF;
    crossoverFilters.midCutoff = DEFAULT_MID_CUTOFF;
    crossoverFilters.highCutoff = DEFAULT_HIGH_CUTOFF;
    crossoverFilters.filterType = DEFAULT_FILTER_TYPE;
    crossoverFilters.filterOrder = DEFAULT_FILTER_ORDER;
    crossoverFilters.sampleRate = DEFAULT_SAMPLE_RATE;
    
    // Set default gains to 0dB (unity gain)
    crossoverFilters.subGain = 1.0f;
    crossoverFilters.lowGain = 1.0f;
    crossoverFilters.midGain = 1.0f;
    crossoverFilters.highGain = 1.0f;
    
    // No bands muted by default
    crossoverFilters.subMute = 0;
    crossoverFilters.lowMute = 0;
    crossoverFilters.midMute = 0;
    crossoverFilters.highMute = 0;
    
    // Set filter counts based on order
    uint8_t filterCount = crossoverFilters.filterOrder / 2;
    crossoverFilters.subLowPass.filterCount = filterCount;
    crossoverFilters.lowLowPass.filterCount = filterCount;
    crossoverFilters.lowHighPass.filterCount = filterCount;
    crossoverFilters.midLowPass.filterCount = filterCount;
    crossoverFilters.midHighPass.filterCount = filterCount;
    crossoverFilters.highHighPass.filterCount = filterCount;
    
    // Reset all filters
    ResetAllFilters();
    
    // Calculate initial filter coefficients
    CalculateFilterCoefficients();
    
    // Store initial settings for UI
    currentSettings.lowCutoff = crossoverFilters.lowCutoff;
    currentSettings.midCutoff = crossoverFilters.midCutoff;
    currentSettings.highCutoff = crossoverFilters.highCutoff;
    currentSettings.subGain = LINEAR_TO_DB(crossoverFilters.subGain);
    currentSettings.lowGain = LINEAR_TO_DB(crossoverFilters.lowGain);
    currentSettings.midGain = LINEAR_TO_DB(crossoverFilters.midGain);
    currentSettings.highGain = LINEAR_TO_DB(crossoverFilters.highGain);
    currentSettings.filterType = crossoverFilters.filterType;
    currentSettings.filterOrder = crossoverFilters.filterOrder;
    currentSettings.subMute = crossoverFilters.subMute;
    currentSettings.lowMute = crossoverFilters.lowMute;
    currentSettings.midMute = crossoverFilters.midMute;
    currentSettings.highMute = crossoverFilters.highMute;
    
    #ifdef DEBUG
    printf("Crossover module initialized\r\n");
    printf("Frequency Points: %.1f Hz, %.1f Hz, %.1f Hz\r\n", 
           crossoverFilters.lowCutoff,
           crossoverFilters.midCutoff,
           crossoverFilters.highCutoff);
    #endif
}

/**
  * @brief  Process audio through the crossover filters
  * @param  input: Pointer to input audio buffer
  * @param  subOut: Pointer to subwoofer band output buffer (can be NULL)
  * @param  lowOut: Pointer to low band output buffer (can be NULL)
  * @param  midOut: Pointer to mid band output buffer (can be NULL)
  * @param  highOut: Pointer to high band output buffer (can be NULL)
  * @param  output: Pointer to combined output buffer (can be NULL)
  * @param  size: Number of samples to process
  * @retval None
  */
void Crossover_Process(float* input, float* subOut, float* lowOut, float* midOut, float* highOut, float* output, uint16_t size)
{
    // Process each sample
    for (uint16_t i = 0; i < size; i++) {
        float inputSample = input[i];
        
        // Process subwoofer band (low-pass filter)
        float subSample = ProcessFilterChain(&crossoverFilters.subLowPass, inputSample);
        
        // Process low band (band-pass filter - low-pass after high-pass)
        float lowSample = ProcessFilterChain(&crossoverFilters.lowLowPass, 
                          ProcessFilterChain(&crossoverFilters.lowHighPass, inputSample));
        
        // Process mid band (band-pass filter - low-pass after high-pass)
        float midSample = ProcessFilterChain(&crossoverFilters.midLowPass, 
                          ProcessFilterChain(&crossoverFilters.midHighPass, inputSample));
        
        // Process high band (high-pass filter)
        float highSample = ProcessFilterChain(&crossoverFilters.highHighPass, inputSample);
        
        // Apply gain and mute to each band
        subSample = crossoverFilters.subMute ? 0.0f : subSample * crossoverFilters.subGain;
        lowSample = crossoverFilters.lowMute ? 0.0f : lowSample * crossoverFilters.lowGain;
        midSample = crossoverFilters.midMute ? 0.0f : midSample * crossoverFilters.midGain;
        highSample = crossoverFilters.highMute ? 0.0f : highSample * crossoverFilters.highGain;
        
        // Store individual band outputs if pointers are provided
        if (subOut != NULL) subOut[i] = subSample;
        if (lowOut != NULL) lowOut[i] = lowSample;
        if (midOut != NULL) midOut[i] = midSample;
        if (highOut != NULL) highOut[i] = highSample;
        
        // Sum all bands for the combined output if pointer is provided
        if (output != NULL) {
            output[i] = subSample + lowSample + midSample + highSample;
        }
        
        // Store results in internal buffers for other modules to access
        subBuffer[i] = subSample;
        lowBuffer[i] = lowSample;
        midBuffer[i] = midSample;
        highBuffer[i] = highSample;
    }
}

/**
  * @brief  Process audio through the crossover from int16_t input/output buffers
  * @param  input: Pointer to int16_t input audio buffer
  * @param  output: Pointer to int16_t output audio buffer
  * @param  size: Number of samples to process
  * @retval None
  */
void Crossover_ProcessI16(int16_t* input, int16_t* output, uint16_t size)
{
    float floatInput[AUDIO_BUFFER_SIZE];
    float floatOutput[AUDIO_BUFFER_SIZE];
    
    // Convert int16_t to float
    for (uint16_t i = 0; i < size; i++) {
        floatInput[i] = (float)input[i] / 32768.0f;
    }
    
    // Process through crossover
    Crossover_Process(floatInput, NULL, NULL, NULL, NULL, floatOutput, size);
    
    // Convert float back to int16_t with limiting
    for (uint16_t i = 0; i < size; i++) {
        // Apply soft limiting to prevent clipping
        float limitedSample = floatOutput[i];
        if (limitedSample > 0.99f) limitedSample = 0.99f;
        if (limitedSample < -0.99f) limitedSample = -0.99f;
        
        // Convert to int16_t
        output[i] = (int16_t)(limitedSample * 32767.0f);
    }
}

/**
  * @brief  Get band-specific output buffers
  * @param  band: Band to get (0: Sub, 1: Low, 2: Mid, 3: High)
  * @retval Pointer to the band's output buffer
  */
float* Crossover_GetBandOutput(uint8_t band)
{
    switch (band) {
        case 0: return subBuffer;
        case 1: return lowBuffer;
        case 2: return midBuffer;
        case 3: return highBuffer;
        default: return NULL;
    }
}

/**
  * @brief  Set crossover settings
  * @param  settings: Pointer to settings structure
  * @retval None
  */
void Crossover_SetSettings(CrossoverSettings_t* settings)
{
    // Update current settings
    memcpy(&currentSettings, settings, sizeof(CrossoverSettings_t));
    
    // Apply settings to internal filter structures
    crossoverFilters.lowCutoff = settings->lowCutoff;
    crossoverFilters.midCutoff = settings->midCutoff;
    crossoverFilters.highCutoff = settings->highCutoff;
    crossoverFilters.subGain = DB_TO_LINEAR(settings->subGain);
    crossoverFilters.lowGain = DB_TO_LINEAR(settings->lowGain);
    crossoverFilters.midGain = DB_TO_LINEAR(settings->midGain);
    crossoverFilters.highGain = DB_TO_LINEAR(settings->highGain);
    crossoverFilters.filterType = settings->filterType;
    crossoverFilters.filterOrder = settings->filterOrder;
    crossoverFilters.subMute = settings->subMute;
    crossoverFilters.lowMute = settings->lowMute;
    crossoverFilters.midMute = settings->midMute;
    crossoverFilters.highMute = settings->highMute;
    
    // Update filter counts based on order
    uint8_t filterCount = crossoverFilters.filterOrder / 2;
    crossoverFilters.subLowPass.filterCount = filterCount;
    crossoverFilters.lowLowPass.filterCount = filterCount;
    crossoverFilters.lowHighPass.filterCount = filterCount;
    crossoverFilters.midLowPass.filterCount = filterCount;
    crossoverFilters.midHighPass.filterCount = filterCount;
    crossoverFilters.highHighPass.filterCount = filterCount;
    
    // Recalculate filter coefficients
    CalculateFilterCoefficients();
    
    #ifdef DEBUG
    printf("Crossover settings updated\r\n");
    printf("Frequency Points: %.1f Hz, %.1f Hz, %.1f Hz\r\n", 
           crossoverFilters.lowCutoff,
           crossoverFilters.midCutoff,
           crossoverFilters.highCutoff);
    #endif
}

/**
  * @brief  Get current crossover settings
  * @param  settings: Pointer to settings structure to fill
  * @retval None
  */
void Crossover_GetSettings(CrossoverSettings_t* settings)
{
    // Copy current settings to output structure
    memcpy(settings, &currentSettings, sizeof(CrossoverSettings_t));
}

/**
  * @brief  Set sample rate for the crossover filters
  * @param  sampleRate: New sample rate in Hz
  * @retval None
  */
void Crossover_SetSampleRate(float sampleRate)
{
    // Update sample rate
    crossoverFilters.sampleRate = sampleRate;
    
    // Recalculate filter coefficients with new sample rate
    CalculateFilterCoefficients();
    
    #ifdef DEBUG
    printf("Crossover sample rate updated to %.1f Hz\r\n", sampleRate);
    #endif
}

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Calculate coefficients for all crossover filters
  * @param  None
  * @retval None
  */
static void CalculateFilterCoefficients(void)
{
    uint8_t i;
    float q;
    
    // Reset all filters before recalculating
    ResetAllFilters();
    
    // For Butterworth filter cascade
    if (crossoverFilters.filterType == 0) {
        // Q values for Butterworth filters depend on filter order
        float qValues2[] = {0.7071f};                       // 2nd order
        float qValues4[] = {0.5412f, 1.3066f};              // 4th order
        float qValues8[] = {0.5098f, 0.6013f, 0.9000f, 1.7412f}; // 8th order
        
        float* qValues;
        uint8_t numFilters = crossoverFilters.filterOrder / 2;
        
        // Select appropriate Q values based on filter order
        switch (crossoverFilters.filterOrder) {
            case 2:  qValues = qValues2; break;
            case 4:  qValues = qValues4; break;
            case 8:  qValues = qValues8; break;
            default: qValues = qValues4; break;  // Default to 4th order
        }
        
        // Calculate Butterworth filter coefficients for each stage
        for (i = 0; i < numFilters; i++) {
            q = qValues[i];
            
            // Subwoofer low-pass
            CalculateButterworthCoefficients(&crossoverFilters.subLowPass.filters[i], 
                                           crossoverFilters.lowCutoff, q, 0);  // 0 = LP
            
            // Low band high-pass and low-pass
            CalculateButterworthCoefficients(&crossoverFilters.lowHighPass.filters[i], 
                                           crossoverFilters.lowCutoff, q, 1);  // 1 = HP
            CalculateButterworthCoefficients(&crossoverFilters.lowLowPass.filters[i], 
                                           crossoverFilters.midCutoff, q, 0);  // 0 = LP
            
            // Mid band high-pass and low-pass
            CalculateButterworthCoefficients(&crossoverFilters.midHighPass.filters[i], 
                                           crossoverFilters.midCutoff, q, 1);  // 1 = HP
            CalculateButterworthCoefficients(&crossoverFilters.midLowPass.filters[i], 
                                           crossoverFilters.highCutoff, q, 0); // 0 = LP
            
            // High band high-pass
            CalculateButterworthCoefficients(&crossoverFilters.highHighPass.filters[i], 
                                           crossoverFilters.highCutoff, q, 1); // 1 = HP
        }
    }
    // For Linkwitz-Riley filter cascade
    else {
        uint8_t numFilters = crossoverFilters.filterOrder / 4; // LR filters are always even order
        
        // Each Linkwitz-Riley filter is a cascade of identical Butterworth filters
        for (i = 0; i < numFilters; i++) {
            // Subwoofer low-pass
            CalculateLinkwitzRileyCoefficients(&crossoverFilters.subLowPass.filters[i], 
                                            crossoverFilters.lowCutoff, 0);  // 0 = LP
            
            // Low band high-pass and low-pass
            CalculateLinkwitzRileyCoefficients(&crossoverFilters.lowHighPass.filters[i], 
                                            crossoverFilters.lowCutoff, 1);  // 1 = HP
            CalculateLinkwitzRileyCoefficients(&crossoverFilters.lowLowPass.filters[i], 
                                            crossoverFilters.midCutoff, 0);  // 0 = LP
            
            // Mid band high-pass and low-pass
            CalculateLinkwitzRileyCoefficients(&crossoverFilters.midHighPass.filters[i], 
                                            crossoverFilters.midCutoff, 1);  // 1 = HP
            CalculateLinkwitzRileyCoefficients(&crossoverFilters.midLowPass.filters[i], 
                                            crossoverFilters.highCutoff, 0); // 0 = LP
            
            // High band high-pass
            CalculateLinkwitzRileyCoefficients(&crossoverFilters.highHighPass.filters[i], 
                                            crossoverFilters.highCutoff, 1); // 1 = HP
        }
    }
}

/**
  * @brief  Calculate Butterworth filter coefficients for a second-order IIR filter
  * @param  filter: Pointer to filter structure
  * @param  frequency: Cutoff frequency in Hz
  * @param  q: Q factor for the filter
  * @param  type: Filter type (0: low-pass, 1: high-pass)
  * @retval None
  */
static void CalculateButterworthCoefficients(BiquadFilter_t* filter, float frequency, float q, uint8_t type)
{
    float omega = 2.0f * PI * frequency / crossoverFilters.sampleRate;
    float alpha = sinf(omega) / (2.0f * q);
    float cosw = cosf(omega);
    float a0, a1, a2, b0, b1, b2;
    
    // Low-pass filter coefficients
    if (type == 0) {
        b0 = (1.0f - cosw) / 2.0f;
        b1 = 1.0f - cosw;
        b2 = (1.0f - cosw) / 2.0f;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cosw;
        a2 = 1.0f - alpha;
    }
    // High-pass filter coefficients
    else {
        b0 = (1.0f + cosw) / 2.0f;
        b1 = -(1.0f + cosw);
        b2 = (1.0f + cosw) / 2.0f;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cosw;
        a2 = 1.0f - alpha;
    }
    
    // Normalize by a0
    filter->b0 = b0 / a0;
    filter->b1 = b1 / a0;
    filter->b2 = b2 / a0;
    filter->a1 = a1 / a0;
    filter->a2 = a2 / a0;
}

/**
  * @brief  Calculate Linkwitz-Riley filter coefficients
  * @param  filter: Pointer to filter structure
  * @param  frequency: Cutoff frequency in Hz
  * @param  type: Filter type (0: low-pass, 1: high-pass)
  * @retval None
  */
static void CalculateLinkwitzRileyCoefficients(BiquadFilter_t* filter, float frequency, uint8_t type)
{
    // Linkwitz-Riley filters are butterworth filters with Q=0.7071 (SQRT(1/2))
    CalculateButterworthCoefficients(filter, frequency, 0.7071f, type);
}

/**
  * @brief  Process a sample through a single biquad filter
  * @param  filter: Pointer to biquad filter structure
  * @param  input: Input sample
  * @retval Filtered output sample
  */
static float ProcessBiquad(BiquadFilter_t* filter, float input)
{
    // Direct Form II implementation
    float output = input - filter->a1 * filter->x1 - filter->a2 * filter->x2;
    float result = filter->b0 * output + filter->b1 * filter->x1 + filter->b2 * filter->x2;
    
    // Update delay elements
    filter->x2 = filter->x1;
    filter->x1 = output;
    
    return result;
}

/**
  * @brief  Process a sample through a filter chain
  * @param  chain: Pointer to filter chain structure
  * @param  input: Input sample
  * @retval Filtered output sample
  */
static float ProcessFilterChain(FilterChain_t* chain, float input)
{
    float output = input;
    uint8_t i;
    
    // Process through each filter in the chain
    for (i = 0; i < chain->filterCount; i++) {
        output = ProcessBiquad(&chain->filters[i], output);
    }
    
    return output;
}

/**
  * @brief  Reset a single biquad filter's state
  * @param  filter: Pointer to filter structure
  * @retval None
  */
static void ResetFilter(BiquadFilter_t* filter)
{
    filter->x1 = 0.0f;
    filter->x2 = 0.0f;
    filter->y1 = 0.0f;
    filter->y2 = 0.0f;
}

/**
  * @brief  Reset all filters in the crossover
  * @param  None
  * @retval None
  */
static void ResetAllFilters(void)
{
    uint8_t i;
    uint8_t maxFilters = MAX_FILTER_ORDER / 2;
    
    // Reset all filter states
    for (i = 0; i < maxFilters; i++) {
        ResetFilter(&subLowPassFilters[i]);
        ResetFilter(&lowLowPassFilters[i]);
        ResetFilter(&lowHighPassFilters[i]);
        ResetFilter(&midLowPassFilters[i]);
        ResetFilter(&midHighPassFilters[i]);
        ResetFilter(&highHighPassFilters[i]);
    }
    
    // Reset all internal buffers
    memset(subBuffer, 0, sizeof(subBuffer));
    memset(lowBuffer, 0, sizeof(lowBuffer));
    memset(midBuffer, 0, sizeof(midBuffer));
    memset(highBuffer, 0, sizeof(highBuffer));
}

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
