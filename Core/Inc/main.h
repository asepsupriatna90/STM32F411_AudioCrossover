 /**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines and includes for the
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  Audio buffer structure
  */
typedef struct {
    int16_t data[256]; /* Size defined by AUDIO_BUFFER_SIZE in main.c */
} AudioBuffer_t;

/**
  * @brief  System settings structure that contains all DSP settings
  */
typedef struct {
    struct CrossoverSettings_t {
        /* Crossover frequency points */
        float lowCutoff;   /* Frequency between Sub and Low bands */
        float midCutoff;   /* Frequency between Low and Mid bands */
        float highCutoff;  /* Frequency between Mid and High bands */
        
        /* Gain for each band in dB */
        float subGain;
        float lowGain;
        float midGain;
        float highGain;
        
        /* Filter types (0: Butterworth, 1: Linkwitz-Riley) */
        uint8_t filterType;
        
        /* Filter orders */
        uint8_t filterOrder;
        
        /* Band mute status (1: muted, 0: active) */
        uint8_t subMute;
        uint8_t lowMute;
        uint8_t midMute;
        uint8_t highMute;
    } crossover;
    
    struct CompressorSettings_t {
        /* Settings for each band */
        struct {
            float threshold;  /* dB, typically -60 to 0 */
            float ratio;      /* ratio, typically 1 to 20 */
            float attack;     /* ms, typically 0.1 to 100 */
            float release;    /* ms, typically 10 to 1000 */
            float makeupGain; /* dB, typically 0 to 20 */
            uint8_t enabled;  /* 1: enabled, 0: bypassed */
        } sub, low, mid, high;
    } compressor;
    
    struct LimiterSettings_t {
        /* Settings for each band */
        struct {
            float threshold;  /* dB, typically -20 to 0 */
            float release;    /* ms, typically 10 to 1000 */
            uint8_t enabled;  /* 1: enabled, 0: bypassed */
        } sub, low, mid, high;
    } limiter;
    
    struct DelaySettings_t {
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
    } delay;
    
    /* Add any additional module settings here */
} SystemSettings_t;

/* Exported constants --------------------------------------------------------*/
/* System state definitions */
#define SYSTEM_STATE_NORMAL          0
#define SYSTEM_STATE_INITIALIZING    1
#define SYSTEM_STATE_SAVE_SETTINGS   2
#define SYSTEM_STATE_LOAD_PRESET     3

/* Preset indices */
#define PRESET_DEFAULT    0
#define PRESET_ROCK       1
#define PRESET_JAZZ       2
#define PRESET_DANGDUT    3
#define PRESET_POP        4
#define NUM_FACTORY_PRESETS 5

/* User preset starting index */
#define USER_PRESET_START  10
#define MAX_USER_PRESETS   10

/* UI constants */
#define UI_REFRESH_INTERVAL 10  /* refresh UI every 10 ticks */

/* Audio buffer size */
#define AUDIO_BUFFER_SIZE 256  /* Must be a multiple of 2 and 4 for stereo processing */

/* Error LED */
#define ERROR_LED_Pin GPIO_PIN_13
#define ERROR_LED_GPIO_Port GPIOC

/* Debug output control */
#ifdef DEBUG
#define DEBUG_PRINT(x) printf(x)
#define DEBUG_PRINTF(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTF(format, ...)
#endif

/* Exported macros -----------------------------------------------------------*/
/* Safe min and max macros */
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

/* dB to linear and linear to dB conversion */
#define DB_TO_LINEAR(x) (powf(10.0f, (x) / 20.0f))
#define LINEAR_TO_DB(x) (20.0f * log10f(MAX((x), 0.00001f)))

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* External variables --------------------------------------------------------*/
/* HAL handlers declared in their respective files */
extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s2;
extern I2S_HandleTypeDef hi2s3;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_spi2_rx;
extern DMA_HandleTypeDef hdma_spi3_tx;

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
