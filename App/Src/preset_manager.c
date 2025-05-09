 /**
  ******************************************************************************
  * @file           : preset_manager.c
  * @brief          : Preset storage and management implementation
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

/* Includes ------------------------------------------------------------------*/
#include "preset_manager.h"
#include <string.h>
#include "flash_storage.h"
#include "factory_presets.h"
#include "audio_preset.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define PRESET_VALID_MARKER           0xABCD1234
#define STRING_MAX_LENGTH             15
#define PRESET_STORAGE_BASE_ADDR      0x0800C000  // Starting address in Flash for preset storage
#define PRESET_STORAGE_SECTOR_SIZE    0x4000      // Size of flash sector (16KB for STM32F411)
#define PRESET_STORAGE_MAX_SIZE       (PRESET_STORAGE_SECTOR_SIZE * 2) // Use 2 sectors for storage

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static uint8_t currentPresetId = 0; // Default preset
static PresetMetadata_t presetMetadata[MAX_USER_PRESETS];
static uint8_t presetInfoInitialized = 0;

/* Private function prototypes -----------------------------------------------*/
static uint32_t CalculateChecksum(const PresetSettings_t* settings);
static uint32_t GetPresetAddress(uint8_t presetId);
static void SanitizePresetName(char* name);
static uint8_t IsPresetValid(const Preset_t* preset);
static void InitPresetMetadataCache(void);

/**
  * @brief  Initialize the preset manager
  * @param  None
  * @retval None
  */
void PresetManager_Init(void)
{
  // Initialize the flash storage subsystem
  Flash_Init();
  
  // Initialize metadata cache
  InitPresetMetadataCache();
  
  // Set current preset to default
  currentPresetId = 0;
  
  #ifdef DEBUG
  printf("Preset Manager initialized\r\n");
  #endif
}

/**
  * @brief  Initialize the preset metadata cache from flash
  * @param  None
  * @retval None
  */
static void InitPresetMetadataCache(void)
{
  // If already initialized, skip
  if (presetInfoInitialized)
    return;
  
  // Initialize all metadata as invalid
  for (uint8_t i = 0; i < MAX_USER_PRESETS; i++) {
    presetMetadata[i].presetId = PRESET_ID_INVALID;
    memset(presetMetadata[i].name, 0, sizeof(presetMetadata[i].name));
    presetMetadata[i].checksum = 0;
    presetMetadata[i].timestamp = 0;
  }
  
  // Load metadata for each potential user preset
  for (uint8_t i = 0; i < MAX_USER_PRESETS; i++) {
    uint8_t presetId = USER_PRESET_START_ID + i;
    Preset_t tempPreset;
    
    // Read preset header from flash
    uint32_t address = GetPresetAddress(presetId);
    if (address != 0) {
      Flash_Read(address, (uint8_t*)&tempPreset, sizeof(PresetMetadata_t));
      
      // If preset is valid, cache its metadata
      if (tempPreset.metadata.presetId == presetId) {
        memcpy(&presetMetadata[i], &tempPreset.metadata, sizeof(PresetMetadata_t));
      }
    }
  }
  
  presetInfoInitialized = 1;
}

/**
  * @brief  Save a preset to flash memory
  * @param  presetId: ID of the preset to save
  * @param  settings: Pointer to the settings structure to save
  * @retval Status code (PRESET_STATUS_OK if successful)
  */
uint8_t PresetManager_SavePreset(uint8_t presetId, const PresetSettings_t* settings)
{
  // Check if presetId is valid for user presets
  if (presetId < USER_PRESET_START_ID || presetId >= TOTAL_PRESET_COUNT) {
    return PRESET_STATUS_INVALID;
  }
  
  // Create a complete preset structure with metadata
  Preset_t preset;
  
  // Set preset ID
  preset.metadata.presetId = presetId;
  
  // Set default name if this is a new preset
  uint8_t metadataIndex = presetId - USER_PRESET_START_ID;
  if (presetMetadata[metadataIndex].presetId == PRESET_ID_INVALID) {
    snprintf(preset.metadata.name, STRING_MAX_LENGTH + 1, "User Preset %d", presetId);
  } else {
    // Keep existing name
    strncpy(preset.metadata.name, presetMetadata[metadataIndex].name, STRING_MAX_LENGTH + 1);
  }
  
  // Copy the settings
  memcpy(&preset.settings, settings, sizeof(PresetSettings_t));
  
  // Calculate checksum
  preset.metadata.checksum = CalculateChecksum(settings);
  
  // Set timestamp (system tick count or RTC if available)
  preset.metadata.timestamp = HAL_GetTick();
  
  // Calculate storage address
  uint32_t address = GetPresetAddress(presetId);
  if (address == 0) {
    return PRESET_STATUS_ERROR;
  }
  
  // Erase flash sector if needed (simplified - actual implementation might need
  // to handle sector management more carefully to avoid wearing out the flash)
  if ((presetId - USER_PRESET_START_ID) % (PRESET_STORAGE_SECTOR_SIZE / sizeof(Preset_t)) == 0) {
    Flash_EraseSector(address);
  }
  
  // Write to flash
  uint8_t status = Flash_Write(address, (uint8_t*)&preset, sizeof(Preset_t));
  if (status != FLASH_STATUS_OK) {
    return PRESET_STATUS_ERROR;
  }
  
  // Update metadata cache
  memcpy(&presetMetadata[metadataIndex], &preset.metadata, sizeof(PresetMetadata_t));
  
  return PRESET_STATUS_OK;
}

/**
  * @brief  Load a preset from flash memory or factory presets
  * @param  presetId: ID of the preset to load
  * @param  settings: Pointer to the settings structure to fill
  * @retval Status code (PRESET_STATUS_OK if successful)
  */
uint8_t PresetManager_LoadPreset(uint8_t presetId, PresetSettings_t* settings)
{
  // If presetId refers to current settings
  if (presetId == PRESET_ID_CURRENT) {
    presetId = currentPresetId;
  }
  
  // Handle factory presets (0 to USER_PRESET_START_ID-1)
  if (presetId < USER_PRESET_START_ID) {
    // Load from factory presets
    return FactoryPresets_GetPreset(presetId, settings);
  }
  
  // Handle user presets
  if (presetId < TOTAL_PRESET_COUNT) {
    // Calculate storage address
    uint32_t address = GetPresetAddress(presetId);
    if (address == 0) {
      return PRESET_STATUS_ERROR;
    }
    
    // Read preset from flash
    Preset_t preset;
    uint8_t status = Flash_Read(address, (uint8_t*)&preset, sizeof(Preset_t));
    if (status != FLASH_STATUS_OK) {
      return PRESET_STATUS_ERROR;
    }
    
    // Validate preset
    if (!IsPresetValid(&preset) || preset.metadata.presetId != presetId) {
      return PRESET_STATUS_INVALID;
    }
    
    // Copy settings
    memcpy(settings, &preset.settings, sizeof(PresetSettings_t));
    
    // Set this as current preset
    currentPresetId = presetId;
    
    return PRESET_STATUS_OK;
  }
  
  return PRESET_STATUS_INVALID;
}

/**
  * @brief  Delete a user preset
  * @param  presetId: ID of the preset to delete
  * @retval Status code (PRESET_STATUS_OK if successful)
  */
uint8_t PresetManager_DeletePreset(uint8_t presetId)
{
  // Only user presets can be deleted
  if (presetId < USER_PRESET_START_ID || presetId >= TOTAL_PRESET_COUNT) {
    return PRESET_STATUS_INVALID;
  }
  
  // Calculate storage address
  uint32_t address = GetPresetAddress(presetId);
  if (address == 0) {
    return PRESET_STATUS_ERROR;
  }
  
  // Create an invalid preset marker to overwrite the existing one
  Preset_t emptyPreset;
  memset(&emptyPreset, 0xFF, sizeof(Preset_t));  // 0xFF is the erased state for flash
  emptyPreset.metadata.presetId = PRESET_ID_INVALID;
  
  // Write the invalid marker
  uint8_t status = Flash_Write(address, (uint8_t*)&emptyPreset, sizeof(PresetMetadata_t));
  if (status != FLASH_STATUS_OK) {
    return PRESET_STATUS_ERROR;
  }
  
  // Update metadata cache
  uint8_t metadataIndex = presetId - USER_PRESET_START_ID;
  presetMetadata[metadataIndex].presetId = PRESET_ID_INVALID;
  memset(presetMetadata[metadataIndex].name, 0, sizeof(presetMetadata[metadataIndex].name));
  presetMetadata[metadataIndex].checksum = 0;
  presetMetadata[metadataIndex].timestamp = 0;
  
  // If this was the current preset, switch to default
  if (currentPresetId == presetId) {
    currentPresetId = 0;
  }
  
  return PRESET_STATUS_OK;
}

/**
  * @brief  Get information about a preset
  * @param  presetId: ID of the preset
  * @param  metadata: Pointer to metadata structure to fill
  * @retval Status code (PRESET_STATUS_OK if successful)
  */
uint8_t PresetManager_GetPresetInfo(uint8_t presetId, PresetMetadata_t* metadata)
{
  // Handle factory presets
  if (presetId < USER_PRESET_START_ID) {
    // Get factory preset name
    const char* name = FactoryPresets_GetName(presetId);
    if (name == NULL) {
      return PRESET_STATUS_INVALID;
    }
    
    // Fill metadata
    metadata->presetId = presetId;
    strncpy(metadata->name, name, STRING_MAX_LENGTH);
    metadata->name[STRING_MAX_LENGTH] = '\0';
    metadata->checksum = PRESET_VALID_MARKER;  // Factory presets are always valid
    metadata->timestamp = 0;  // Factory presets have no timestamp
    
    return PRESET_STATUS_OK;
  }
  
  // Handle user presets
  if (presetId < TOTAL_PRESET_COUNT) {
    uint8_t metadataIndex = presetId - USER_PRESET_START_ID;
    
    // Check if preset exists
    if (presetMetadata[metadataIndex].presetId == PRESET_ID_INVALID) {
      return PRESET_STATUS_EMPTY;
    }
    
    // Copy metadata
    memcpy(metadata, &presetMetadata[metadataIndex], sizeof(PresetMetadata_t));
    
    return PRESET_STATUS_OK;
  }
  
  return PRESET_STATUS_INVALID;
}

/**
  * @brief  Rename a user preset
  * @param  presetId: ID of the preset to rename
  * @param  newName: New name for the preset
  * @retval Status code (PRESET_STATUS_OK if successful)
  */
uint8_t PresetManager_RenamePreset(uint8_t presetId, const char* newName)
{
  // Only user presets can be renamed
  if (presetId < USER_PRESET_START_ID || presetId >= TOTAL_PRESET_COUNT) {
    return PRESET_STATUS_INVALID;
  }
  
  uint8_t metadataIndex = presetId - USER_PRESET_START_ID;
  
  // Check if preset exists
  if (presetMetadata[metadataIndex].presetId == PRESET_ID_INVALID) {
    return PRESET_STATUS_EMPTY;
  }
  
  // Calculate storage address
  uint32_t address = GetPresetAddress(presetId);
  if (address == 0) {
    return PRESET_STATUS_ERROR;
  }
  
  // Read full preset from flash
  Preset_t preset;
  uint8_t status = Flash_Read(address, (uint8_t*)&preset, sizeof(Preset_t));
  if (status != FLASH_STATUS_OK) {
    return PRESET_STATUS_ERROR;
  }
  
  // Update name
  char sanitizedName[STRING_MAX_LENGTH + 1];
  strncpy(sanitizedName, newName, STRING_MAX_LENGTH);
  sanitizedName[STRING_MAX_LENGTH] = '\0';
  SanitizePresetName(sanitizedName);
  
  strncpy(preset.metadata.name, sanitizedName, STRING_MAX_LENGTH + 1);
  
  // Erase and write back to flash
  if ((presetId - USER_PRESET_START_ID) % (PRESET_STORAGE_SECTOR_SIZE / sizeof(Preset_t)) == 0) {
    Flash_EraseSector(address);
  }
  
  status = Flash_Write(address, (uint8_t*)&preset, sizeof(Preset_t));
  if (status != FLASH_STATUS_OK) {
    return PRESET_STATUS_ERROR;
  }
  
  // Update metadata cache
  strncpy(presetMetadata[metadataIndex].name, sanitizedName, STRING_MAX_LENGTH + 1);
  
  return PRESET_STATUS_OK;
}

/**
  * @brief  Get the number of presets (both factory and user)
  * @param  None
  * @retval Number of presets available
  */
uint8_t PresetManager_GetPresetCount(void)
{
  uint8_t count = USER_PRESET_START_ID;  // Factory presets
  
  // Count user presets
  for (uint8_t i = 0; i < MAX_USER_PRESETS; i++) {
    if (presetMetadata[i].presetId != PRESET_ID_INVALID) {
      count++;
    }
  }
  
  return count;
}

/**
  * @brief  Find the next empty preset slot
  * @param  None
  * @retval Preset ID for the next empty slot or PRESET_ID_INVALID if none
  */
uint8_t PresetManager_GetNextEmptySlot(void)
{
  for (uint8_t i = 0; i < MAX_USER_PRESETS; i++) {
    if (presetMetadata[i].presetId == PRESET_ID_INVALID) {
      return USER_PRESET_START_ID + i;
    }
  }
  
  return PRESET_ID_INVALID;
}

/**
  * @brief  Set the current active preset ID
  * @param  presetId: ID to set as current
  * @retval None
  */
void PresetManager_SetCurrentPreset(uint8_t presetId)
{
  if (presetId < TOTAL_PRESET_COUNT) {
    currentPresetId = presetId;
  }
}

/**
  * @brief  Get the current active preset ID
  * @param  None
  * @retval Current preset ID
  */
uint8_t PresetManager_GetCurrentPreset(void)
{
  return currentPresetId;
}

/**
  * @brief  Calculate a simple checksum for preset validation
  * @param  settings: Pointer to settings structure
  * @retval Calculated checksum
  */
static uint32_t CalculateChecksum(const PresetSettings_t* settings)
{
  uint32_t checksum = PRESET_VALID_MARKER;
  uint8_t* data = (uint8_t*)settings;
  uint32_t size = sizeof(PresetSettings_t);
  
  for (uint32_t i = 0; i < size; i++) {
    checksum += data[i];
  }
  
  return checksum;
}

/**
  * @brief  Calculate the flash storage address for a preset
  * @param  presetId: ID of the preset
  * @retval Flash address or 0 if invalid
  */
static uint32_t GetPresetAddress(uint8_t presetId)
{
  // Only calculate for user presets
  if (presetId < USER_PRESET_START_ID || presetId >= TOTAL_PRESET_COUNT) {
    return 0;
  }
  
  uint8_t userPresetIndex = presetId - USER_PRESET_START_ID;
  uint32_t offset = userPresetIndex * sizeof(Preset_t);
  
  // Make sure we don't exceed the allocated storage
  if (offset >= PRESET_STORAGE_MAX_SIZE) {
    return 0;
  }
  
  return PRESET_STORAGE_BASE_ADDR + offset;
}

/**
  * @brief  Sanitize preset name to ensure it's valid
  * @param  name: Preset name to sanitize (in-place)
  * @retval None
  */
static void SanitizePresetName(char* name)
{
  // Replace invalid characters with underscore
  for (uint8_t i = 0; i < STRING_MAX_LENGTH && name[i] != '\0'; i++) {
    // Allow alphanumeric, space, and some punctuation
    if (!((name[i] >= 'A' && name[i] <= 'Z') ||
          (name[i] >= 'a' && name[i] <= 'z') ||
          (name[i] >= '0' && name[i] <= '9') ||
          name[i] == ' ' || name[i] == '_' || name[i] == '-')) {
      name[i] = '_';
    }
  }
  
  // Ensure name is not empty
  if (name[0] == '\0') {
    strcpy(name, "User_Preset");
  }
}

/**
  * @brief  Check if a preset is valid
  * @param  preset: Pointer to preset to validate
  * @retval 1 if valid, 0 if invalid
  */
static uint8_t IsPresetValid(const Preset_t* preset)
{
  // Check preset ID range
  if (preset->metadata.presetId < USER_PRESET_START_ID || 
      preset->metadata.presetId >= TOTAL_PRESET_COUNT) {
    return 0;
  }
  
  // Verify checksum
  uint32_t calculatedChecksum = CalculateChecksum(&preset->settings);
  if (calculatedChecksum != preset->metadata.checksum) {
    return 0;
  }
  
  return 1;
}

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
