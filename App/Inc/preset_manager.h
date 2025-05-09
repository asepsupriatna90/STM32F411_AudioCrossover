 /**
  ******************************************************************************
  * @file           : preset_manager.h
  * @brief          : Header for preset_manager.c file.
  *                   This file contains the common defines and functions prototypes
  *                   for preset storage and management.
  * @author         : Panel Kontrol DSP STM32F411 Project
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
#ifndef __PRESET_MANAGER_H
#define __PRESET_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "flash_storage.h"
#include "crossover.h"
#include "compressor.h"
#include "limiter.h"
#include "delay.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  System settings structure (all DSP settings combined)
  * @note   This structure should match the SystemSettings_t in main.c
  */
typedef struct {
    CrossoverSettings_t crossover;
    CompressorSettings_t compressor;
    LimiterSettings_t limiter;
    DelaySettings_t delay;
    // Add any additional module settings here
} PresetSettings_t;

/**
  * @brief  Preset metadata structure
  */
typedef struct {
    uint8_t presetId;
    char name[16];    // Preset name (null-terminated string)
    uint32_t checksum; // Simple checksum for data validation
    uint32_t timestamp; // Creation/modification timestamp
} PresetMetadata_t;

/**
  * @brief  Complete preset structure with metadata and settings
  */
typedef struct {
    PresetMetadata_t metadata;
    PresetSettings_t settings;
} Preset_t;

/* Exported constants --------------------------------------------------------*/
/* Maximum number of user presets that can be stored */
#define MAX_USER_PRESETS              10
/* Starting preset ID for user presets (after factory presets) */
#define USER_PRESET_START_ID          5
/* Total number of presets (factory + user) */
#define TOTAL_PRESET_COUNT            (USER_PRESET_START_ID + MAX_USER_PRESETS)

/* Special preset ID values */
#define PRESET_ID_INVALID             0xFF
#define PRESET_ID_CURRENT             0xFE

/* Preset status constants */
#define PRESET_STATUS_OK              0
#define PRESET_STATUS_EMPTY           1
#define PRESET_STATUS_INVALID         2
#define PRESET_STATUS_ERROR           3

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void PresetManager_Init(void);
uint8_t PresetManager_SavePreset(uint8_t presetId, const PresetSettings_t* settings);
uint8_t PresetManager_LoadPreset(uint8_t presetId, PresetSettings_t* settings);
uint8_t PresetManager_DeletePreset(uint8_t presetId);
uint8_t PresetManager_GetPresetInfo(uint8_t presetId, PresetMetadata_t* metadata);
uint8_t PresetManager_RenamePreset(uint8_t presetId, const char* newName);
uint8_t PresetManager_GetPresetCount(void);
uint8_t PresetManager_GetNextEmptySlot(void);
void PresetManager_SetCurrentPreset(uint8_t presetId);
uint8_t PresetManager_GetCurrentPreset(void);

#ifdef __cplusplus
}
#endif

#endif /* __PRESET_MANAGER_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
