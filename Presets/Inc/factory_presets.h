 /**
  ******************************************************************************
  * @file           : factory_presets.h
  * @brief          : Header file for factory audio presets
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
#ifndef __FACTORY_PRESETS_H
#define __FACTORY_PRESETS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crossover.h"
#include "compressor.h"
#include "limiter.h"
#include "delay.h"

/* Exported constants --------------------------------------------------------*/
/* Preset indices (also defined in main.c) */
#define PRESET_DEFAULT    0  /**< Default (Flat) preset */
#define PRESET_ROCK       1  /**< Rock preset */
#define PRESET_JAZZ       2  /**< Jazz preset */
#define PRESET_DANGDUT    3  /**< Dangdut preset */
#define PRESET_POP        4  /**< Pop preset */
#define NUM_FACTORY_PRESETS 5  /**< Total number of factory presets */

/* System Settings Type Definition (should match main.c) */
typedef struct {
    CrossoverSettings_t crossover;
    CompressorSettings_t compressor;
    LimiterSettings_t limiter;
    DelaySettings_t delay;
    // Additional module settings can be added here
} SystemSettings_t;

/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief Initialize the factory presets subsystem
  * @retval None
  */
void FactoryPresets_Init(void);

/**
  * @brief Get a factory preset by index
  * @param presetIndex The preset index to retrieve (0-4)
  * @param settings Pointer to SystemSettings_t structure to fill with preset data
  * @retval 0 if successful, non-zero on error
  */
uint8_t FactoryPresets_GetPreset(uint8_t presetIndex, SystemSettings_t* settings);

/**
  * @brief Get the name of a factory preset
  * @param presetIndex The preset index (0-4)
  * @return Pointer to preset name string
  */
const char* FactoryPresets_GetPresetName(uint8_t presetIndex);

#ifdef __cplusplus
}
#endif

#endif /* __FACTORY_PRESETS_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
