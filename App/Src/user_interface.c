 /**
  ******************************************************************************
  * @file           : user_interface.c
  * @brief          : User interface manager implementation
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
#include "ui_manager.h"
#include "lcd_driver.h"
#include "rotary_encoder.h"
#include "button_handler.h"
#include "menu_system.h"
#include "audio_preset.h"
#include "crossover.h"
#include "compressor.h"
#include "limiter.h"
#include "delay.h"
#include "factory_presets.h"
#include "preset_manager.h"
#include "flash_storage.h"
#include <stdlib.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define UI_STATE_NORMAL          0
#define UI_STATE_EDIT_VALUE      1
#define UI_STATE_CONFIRM_ACTION  2
#define UI_STATE_MENU_SCROLLING  3

#define EDIT_TIMEOUT           5000  // 5 seconds timeout for edit mode
#define REFRESH_INTERVAL       100   // UI refresh interval in ms
#define BUTTON_HOLD_TIME       1500  // Time to hold button for alternative action

/* Private variables ---------------------------------------------------------*/
static uint8_t uiState = UI_STATE_NORMAL;
static uint8_t needsRefresh = 1;
static uint32_t lastInteractionTime = 0;
static int32_t editValue = 0;
static int32_t editValueMin = 0;
static int32_t editValueMax = 0;
static int32_t editValueStep = 1;
static void (*editCallback)(int32_t) = NULL;
static uint8_t currentMenuIndex = 0;
static char confirmMessage[17] = "";
static void (*confirmCallback)(uint8_t) = NULL;
static uint8_t buttonHoldCounter[NUM_BUTTONS] = {0};
static uint8_t volumeAdjustMode = 0;
static uint8_t currentBand = 0;
static uint8_t currentPreset = 0;
static uint32_t lastRefreshTime = 0;

/* Private function prototypes -----------------------------------------------*/
static void HandleNormalModeRotary(RotaryEvent_t *event);
static void HandleEditModeRotary(RotaryEvent_t *event);
static void HandleConfirmModeButton(ButtonEvent_t *event);
static void HandleNormalModeButton(ButtonEvent_t *event);
static void HandleEditModeButton(ButtonEvent_t *event);
static void HandleMenuScrollingModeRotary(RotaryEvent_t *event);
static void HandleMenuScrollingModeButton(ButtonEvent_t *event);
static void UpdateVolumeUI(void);
static void RefreshUI(void);
static void TimeoutEditMode(void);
static void SaveCurrentPreset(void);
static void ShowParameterEdit(const char* paramName, int32_t value, 
                              int32_t min, int32_t max, int32_t step, 
                              void (*callback)(int32_t));
static void ShowConfirmDialog(const char* message, void (*callback)(uint8_t));
static void FormatValue(char* buffer, int32_t value, uint8_t valueType);

/* Value type formatting constants */
#define VALUE_TYPE_INTEGER     0
#define VALUE_TYPE_DECIMAL     1
#define VALUE_TYPE_FREQUENCY   2
#define VALUE_TYPE_DB          3
#define VALUE_TYPE_MS          4
#define VALUE_TYPE_PERCENT     5
#define VALUE_TYPE_RATIO       6
#define VALUE_TYPE_DEGREE      7

/**
  * @brief  Initialize the UI manager
  * @retval None
  */
void UI_Init(void)
{
  uiState = UI_STATE_NORMAL;
  needsRefresh = 1;
  lastInteractionTime = 0;
  volumeAdjustMode = 0;
  currentBand = 0;
  currentPreset = 0;
  
  /* Reset button hold counters */
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    buttonHoldCounter[i] = 0;
  }
  
  /* Initial UI refresh */
  RefreshUI();
}

/**
  * @brief  Notify UI that it needs refresh
  * @retval None
  */
void UI_NeedsRefresh(void)
{
  needsRefresh = 1;
}

/**
  * @brief  Handle rotary encoder events
  * @param  event Pointer to rotary encoder event
  * @retval None
  */
void UI_HandleRotaryEvent(RotaryEvent_t *event)
{
  if (event == NULL) return;
  
  /* Record interaction time for timeout handling */
  lastInteractionTime = HAL_GetTick();
  
  /* Handle rotary events based on current UI state */
  switch (uiState) {
    case UI_STATE_NORMAL:
      HandleNormalModeRotary(event);
      break;
      
    case UI_STATE_EDIT_VALUE:
      HandleEditModeRotary(event);
      break;
      
    case UI_STATE_MENU_SCROLLING:
      HandleMenuScrollingModeRotary(event);
      break;
      
    case UI_STATE_CONFIRM_ACTION:
      /* Rotary events not used in confirm mode */
      break;
      
    default:
      break;
  }
  
  /* Force UI refresh after rotary interaction */
  needsRefresh = 1;
}

/**
  * @brief  Handle button events
  * @param  event Pointer to button event
  * @retval None
  */
void UI_HandleButtonEvent(ButtonEvent_t *event)
{
  if (event == NULL) return;
  
  /* Record interaction time for timeout handling */
  lastInteractionTime = HAL_GetTick();
  
  /* Track button hold time */
  if (event->state == BUTTON_PRESSED) {
    buttonHoldCounter[event->button] = 1;
  } else if (event->state == BUTTON_RELEASED) {
    buttonHoldCounter[event->button] = 0;
  }
  
  /* Handle button events based on current UI state */
  switch (uiState) {
    case UI_STATE_NORMAL:
      HandleNormalModeButton(event);
      break;
      
    case UI_STATE_EDIT_VALUE:
      HandleEditModeButton(event);
      break;
      
    case UI_STATE_CONFIRM_ACTION:
      HandleConfirmModeButton(event);
      break;
      
    case UI_STATE_MENU_SCROLLING:
      HandleMenuScrollingModeButton(event);
      break;
      
    default:
      break;
  }
  
  /* Force UI refresh after button interaction */
  needsRefresh = 1;
}

/**
  * @brief  Update UI state machine
  * @retval None
  */
void UI_Update(void)
{
  uint32_t currentTime = HAL_GetTick();
  
  /* Check for edit mode timeout */
  if (uiState == UI_STATE_EDIT_VALUE) {
    if ((currentTime - lastInteractionTime) > EDIT_TIMEOUT) {
      TimeoutEditMode();
    }
  }
  
  /* Check for button hold actions */
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    if (buttonHoldCounter[i] > 0) {
      buttonHoldCounter[i]++;
      
      /* Trigger hold action after hold threshold */
      if (buttonHoldCounter[i] >= (BUTTON_HOLD_TIME / REFRESH_INTERVAL)) {
        /* Process hold action based on button */
        ButtonEvent_t holdEvent;
        holdEvent.button = i;
        holdEvent.state = BUTTON_HELD;
        UI_HandleButtonEvent(&holdEvent);
        
        /* Reset counter to prevent multiple triggers */
        buttonHoldCounter[i] = 0;
      }
    }
  }
  
  /* Update UI at refresh interval */
  if ((currentTime - lastRefreshTime >= REFRESH_INTERVAL) && needsRefresh) {
    RefreshUI();
    lastRefreshTime = currentTime;
    needsRefresh = 0;
  }
}

/**
  * @brief  Set the active audio band for editing (Sub, Low, Mid, High)
  * @param  band Band index to set active
  * @retval None
  */
void UI_SetActiveBand(uint8_t band)
{
  currentBand = band;
  needsRefresh = 1;
}

/**
  * @brief  Get the currently active audio band
  * @retval Active band index
  */
uint8_t UI_GetActiveBand(void)
{
  return currentBand;
}

/**
  * @brief Handle rotary events in normal UI mode
  * @param event Pointer to rotary encoder event
  * @retval None
  */
static void HandleNormalModeRotary(RotaryEvent_t *event)
{
  /* In normal mode, rotary adjusts volume if in volume adjust mode */
  if (volumeAdjustMode) {
    CrossoverSettings_t settings;
    Crossover_GetSettings(&settings);
    
    /* Adjust gain for current band */
    if (event->direction == ROTARY_CW) {
      settings.bandGain[currentBand] += 0.5f;
      if (settings.bandGain[currentBand] > 12.0f) {
        settings.bandGain[currentBand] = 12.0f;
      }
    } else {
      settings.bandGain[currentBand] -= 0.5f;
      if (settings.bandGain[currentBand] < -60.0f) {
        settings.bandGain[currentBand] = -60.0f;
      }
    }
    
    /* Apply new settings */
    Crossover_SetSettings(&settings);
    
    /* Update volume display */
    UpdateVolumeUI();
  } else {
    /* Otherwise adjust menu selection */
    if (event->direction == ROTARY_CW) {
      Menu_Next();
    } else {
      Menu_Previous();
    }
  }
}

/**
  * @brief Handle rotary events in value edit UI mode
  * @param event Pointer to rotary encoder event
  * @retval None
  */
static void HandleEditModeRotary(RotaryEvent_t *event)
{
  /* Adjust the value being edited */
  if (event->direction == ROTARY_CW) {
    editValue += editValueStep;
    if (editValue > editValueMax) {
      editValue = editValueMax;
    }
  } else {
    editValue -= editValueStep;
    if (editValue < editValueMin) {
      editValue = editValueMin;
    }
  }
  
  /* Display the updated value */
  char valueStr[17];
  LCD_SetCursor(0, 1);
  LCD_PrintChar('>');
  
  /* Format the value appropriately */
  FormatValue(valueStr, editValue, VALUE_TYPE_INTEGER);
  LCD_Print(valueStr);
  
  /* Clear the rest of the line */
  for (uint8_t i = strlen(valueStr) + 1; i < 16; i++) {
    LCD_PrintChar(' ');
  }
}

/**
  * @brief Handle button events in normal UI mode
  * @param event Pointer to button event
  * @retval None
  */
static void HandleNormalModeButton(ButtonEvent_t *event)
{
  /* Process button press events */
  if (event->state == BUTTON_PRESSED) {
    switch (event->button) {
      case BUTTON_OK:
        /* Select current menu item */
        Menu_Select();
        break;
        
      case BUTTON_BACK:
        /* Go back to previous menu */
        Menu_Back();
        break;
        
      case BUTTON_PRESET:
        /* Show preset menu */
        Menu_ShowPresetMenu();
        break;
        
      case BUTTON_BAND:
        /* Cycle through bands */
        currentBand = (currentBand + 1) % 4;  // 4 bands: Sub, Low, Mid, High
        
        /* Show band selection UI */
        LCD_Clear();
        LCD_SetCursor(0, 0);
        LCD_Print("Band Selected:");
        LCD_SetCursor(0, 1);
        
        switch (currentBand) {
          case 0:
            LCD_Print("Sub Band");
            break;
          case 1:
            LCD_Print("Low Band");
            break;
          case 2:
            LCD_Print("Mid Band");
            break;
          case 3:
            LCD_Print("High Band");
            break;
        }
        
        /* Return to previous menu after a brief delay */
        HAL_Delay(1000);
        Menu_RefreshCurrent();
        break;
        
      case BUTTON_MUTE:
        /* Toggle mute for current band */
        CrossoverSettings_t settings;
        Crossover_GetSettings(&settings);
        settings.bandMute[currentBand] = !settings.bandMute[currentBand];
        Crossover_SetSettings(&settings);
        
        /* Show mute status UI */
        LCD_Clear();
        LCD_SetCursor(0, 0);
        
        switch (currentBand) {
          case 0:
            LCD_Print("Sub Band:");
            break;
          case 1:
            LCD_Print("Low Band:");
            break;
          case 2:
            LCD_Print("Mid Band:");
            break;
          case 3:
            LCD_Print("High Band:");
            break;
        }
        
        LCD_SetCursor(0, 1);
        LCD_Print(settings.bandMute[currentBand] ? "MUTED" : "UNMUTED");
        
        /* Return to previous menu after a brief delay */
        HAL_Delay(1000);
        Menu_RefreshCurrent();
        break;
        
      case BUTTON_VOLUME:
        /* Toggle volume adjustment mode */
        volumeAdjustMode = !volumeAdjustMode;
        
        if (volumeAdjustMode) {
          /* Show volume adjustment UI */
          UpdateVolumeUI();
        } else {
          /* Return to menu */
          Menu_RefreshCurrent();
        }
        break;
        
      default:
        break;
    }
  } else if (event->state == BUTTON_HELD) {
    /* Handle button hold actions */
    switch (event->button) {
      case BUTTON_PRESET:
        /* Quick save current settings to active preset */
        SaveCurrentPreset();
        break;
        
      case BUTTON_VOLUME:
        /* Reset gain of current band to 0 dB */
        CrossoverSettings_t settings;
        Crossover_GetSettings(&settings);
        settings.bandGain[currentBand] = 0.0f;
        Crossover_SetSettings(&settings);
        
        if (volumeAdjustMode) {
          UpdateVolumeUI();
        }
        break;
        
      default:
        break;
    }
  }
}

/**
  * @brief Handle button events in edit mode
  * @param event Pointer to button event
  * @retval None
  */
static void HandleEditModeButton(ButtonEvent_t *event)
{
  if (event->state == BUTTON_PRESSED) {
    switch (event->button) {
      case BUTTON_OK:
        /* Confirm edit */
        if (editCallback != NULL) {
          editCallback(editValue);
        }
        
        /* Return to normal mode */
        uiState = UI_STATE_NORMAL;
        Menu_RefreshCurrent();
        break;
        
      case BUTTON_BACK:
        /* Cancel edit */
        uiState = UI_STATE_NORMAL;
        Menu_RefreshCurrent();
        break;
        
      default:
        break;
    }
  }
}

/**
  * @brief Handle button events in confirmation dialog
  * @param event Pointer to button event
  * @retval None
  */
static void HandleConfirmModeButton(ButtonEvent_t *event)
{
  if (event->state == BUTTON_PRESSED) {
    switch (event->button) {
      case BUTTON_OK:
        /* Confirm action */
        if (confirmCallback != NULL) {
          confirmCallback(1);  // 1 for confirmed
        }
        
        /* Return to normal mode */
        uiState = UI_STATE_NORMAL;
        Menu_RefreshCurrent();
        break;
        
      case BUTTON_BACK:
        /* Cancel action */
        if (confirmCallback != NULL) {
          confirmCallback(0);  // 0 for canceled
        }
        
        /* Return to normal mode */
        uiState = UI_STATE_NORMAL;
        Menu_RefreshCurrent();
        break;
        
      default:
        break;
    }
  }
}

/**
  * @brief Handle rotary events in menu scrolling mode
  * @param event Pointer to rotary encoder event
  * @retval None
  */
static void HandleMenuScrollingModeRotary(RotaryEvent_t *event)
{
  if (event->direction == ROTARY_CW) {
    currentMenuIndex++;
    if (currentMenuIndex >= Menu_GetItemCount()) {
      currentMenuIndex = 0;
    }
  } else {
    if (currentMenuIndex > 0) {
      currentMenuIndex--;
    } else {
      currentMenuIndex = Menu_GetItemCount() - 1;
    }
  }
  
  /* Display current menu selection */
  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_PrintChar('>');
  LCD_Print(Menu_GetItemText(currentMenuIndex));
  
  if (currentMenuIndex < Menu_GetItemCount() - 1) {
    LCD_SetCursor(0, 1);
    LCD_Print(Menu_GetItemText(currentMenuIndex + 1));
  }
}

/**
  * @brief Handle button events in menu scrolling mode
  * @param event Pointer to button event
  * @retval None
  */
static void HandleMenuScrollingModeButton(ButtonEvent_t *event)
{
  if (event->state == BUTTON_PRESSED) {
    switch (event->button) {
      case BUTTON_OK:
        /* Select current menu item */
        Menu_SelectItem(currentMenuIndex);
        
        /* Return to normal mode */
        uiState = UI_STATE_NORMAL;
        break;
        
      case BUTTON_BACK:
        /* Cancel menu scrolling */
        uiState = UI_STATE_NORMAL;
        Menu_RefreshCurrent();
        break;
        
      default:
        break;
    }
  }
}

/**
  * @brief Update the volume adjustment UI
  * @retval None
  */
static void UpdateVolumeUI(void)
{
  CrossoverSettings_t settings;
  Crossover_GetSettings(&settings);
  
  LCD_Clear();
  LCD_SetCursor(0, 0);
  
  /* Display current band name */
  switch (currentBand) {
    case 0:
      LCD_Print("Sub Volume:");
      break;
    case 1:
      LCD_Print("Low Volume:");
      break;
    case 2:
      LCD_Print("Mid Volume:");
      break;
    case 3:
      LCD_Print("High Volume:");
      break;
  }
  
  /* Display the gain value */
  LCD_SetCursor(0, 1);
  
  char valueStr[17];
  int32_t gainValue = (int32_t)(settings.bandGain[currentBand] * 10);
  FormatValue(valueStr, gainValue, VALUE_TYPE_DB);
  LCD_Print(valueStr);
  
  /* Show mute status if applicable */
  if (settings.bandMute[currentBand]) {
    LCD_Print(" (MUTED)");
  }
}

/**
  * @brief Refresh the UI based on current state
  * @retval None
  */
static void RefreshUI(void)
{
  /* Do nothing if in volume mode - it has its own refresh */
  if (volumeAdjustMode && uiState == UI_STATE_NORMAL) {
    UpdateVolumeUI();
    return;
  }
  
  /* Refresh menu in normal mode */
  if (uiState == UI_STATE_NORMAL) {
    Menu_Refresh();
  }
}

/**
  * @brief Handle timeout of edit mode
  * @retval None
  */
static void TimeoutEditMode(void)
{
  /* Return to normal mode without saving changes */
  uiState = UI_STATE_NORMAL;
  Menu_RefreshCurrent();
}

/**
  * @brief Show parameter edit screen
  * @param paramName Name of the parameter being edited
  * @param value Current value of the parameter
  * @param min Minimum allowed value
  * @param max Maximum allowed value
  * @param step Step size for adjustment
  * @param callback Function to call when edit is complete
  * @retval None
  */
void UI_ShowParameterEdit(const char* paramName, int32_t value, 
                          int32_t min, int32_t max, int32_t step, 
                          void (*callback)(int32_t))
{
  /* Set edit mode parameters */
  editValue = value;
  editValueMin = min;
  editValueMax = max;
  editValueStep = step;
  editCallback = callback;
  
  /* Set UI state to edit mode */
  uiState = UI_STATE_EDIT_VALUE;
  
  /* Show edit UI */
  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_Print(paramName);
  LCD_SetCursor(0, 1);
  LCD_PrintChar('>');
  
  /* Format and show the value */
  char valueStr[17];
  FormatValue(valueStr, value, VALUE_TYPE_INTEGER);
  LCD_Print(valueStr);
  
  /* Record current time for timeout handling */
  lastInteractionTime = HAL_GetTick();
}

/**
  * @brief Show confirmation dialog
  * @param message Confirmation message to display
  * @param callback Function to call with result (1=confirm, 0=cancel)
  * @retval None
  */
void UI_ShowConfirmDialog(const char* message, void (*callback)(uint8_t))
{
  /* Store confirmation parameters */
  strncpy(confirmMessage, message, 16);
  confirmMessage[16] = '\0';
  confirmCallback = callback;
  
  /* Set UI state to confirm mode */
  uiState = UI_STATE_CONFIRM_ACTION;
  
  /* Show confirmation UI */
  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_Print(confirmMessage);
  LCD_SetCursor(0, 1);
  LCD_Print("[OK] / [BACK]");
}

/**
  * @brief Enter menu scrolling mode
  * @retval None
  */
void UI_EnterMenuScrolling(void)
{
  /* Set UI state to menu scrolling mode */
  uiState = UI_STATE_MENU_SCROLLING;
  currentMenuIndex = 0;
  
  /* Show initial menu items */
  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_PrintChar('>');
  LCD_Print(Menu_GetItemText(0));
  
  if (Menu_GetItemCount() > 1) {
    LCD_SetCursor(0, 1);
    LCD_Print(Menu_GetItemText(1));
  }
}

/**
  * @brief Format value for display based on type
  * @param buffer Output string buffer
  * @param value Value to format
  * @param valueType Type of value (determines formatting)
  * @retval None
  */
static void FormatValue(char* buffer, int32_t value, uint8_t valueType)
{
  switch (valueType) {
    case VALUE_TYPE_INTEGER:
      sprintf(buffer, "%ld", value);
      break;
      
    case VALUE_TYPE_DECIMAL:
      /* Assuming value is scaled by 10 */
      sprintf(buffer, "%ld.%ld", value / 10, abs(value % 10));
      break;
      
    case VALUE_TYPE_FREQUENCY:
      if (value < 1000) {
        sprintf(buffer, "%ld Hz", value);
      } else {
        sprintf(buffer, "%.1f kHz", value / 1000.0f);
      }
      break;
      
    case VALUE_TYPE_DB:
      /* Assuming value is scaled by 10 */
      sprintf(buffer, "%ld.%ld dB", value / 10, abs(value % 10));
      break;
      
    case VALUE_TYPE_MS:
      sprintf(buffer, "%ld ms", value);
      break;
      
    case VALUE_TYPE_PERCENT:
      sprintf(buffer, "%ld%%", value);
      break;
      
    case VALUE_TYPE_RATIO:
      /* Assuming value is ratio * 10 */
      sprintf(buffer, "%ld.%ld:1", value / 10, value % 10);
      break;
      
    case VALUE_TYPE_DEGREE:
      sprintf(buffer, "%ld°", value);
      break;
      
    default:
      sprintf(buffer, "%ld", value);
      break;
  }
}

/**
  * @brief Save current settings to the active preset
  * @retval None
  */
static void SaveCurrentPreset(void)
{
  /* Show saving indicator */
  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_Print("Saving to");
  LCD_SetCursor(0, 1);
  LCD_Print("Preset ");
  LCD_PrintNumber(currentPreset);
  
  /* Create system settings structure */
  SystemSettings_t settings;
  
  /* Get settings from all audio modules */
  Crossover_GetSettings(&settings.crossover);
  Compressor_GetSettings(&settings.compressor);
  Limiter_GetSettings(&settings.limiter);
  Delay_GetSettings(&settings.delay);
  
  /* Save settings to preset */
  PresetManager_SavePreset(currentPreset, &settings);
  
  /* Success indicator */
  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_Print("Preset saved!");
  HAL_Delay(1000);
  
  /* Return to previous screen */
  Menu_RefreshCurrent();
}

/**
  * @brief Set the current preset number
  * @param preset Preset number to set as current
  * @retval None
  */
void UI_SetCurrentPreset(uint8_t preset)
{
  currentPreset = preset;
}

/**
  * @brief Get the current preset number
  * @retval Current preset number
  */
uint8_t UI_GetCurrentPreset(void)
{
  return currentPreset;
}

/**
  * @brief Show parameter edit screen (public version)
  * @param paramName Name of the parameter being edited
  * @param value Current value of the parameter
  * @param min Minimum allowed value
  * @param max Maximum allowed value
  * @param step Step size for adjustment
  * @param callback Function to call when edit is complete
  * @retval None
  */
void UI_EditParameter(const char* paramName, int32_t value, 
                      int32_t min, int32_t max, int32_t step, 
                      void (*callback)(int32_t))
{
  UI_ShowParameterEdit(paramName, value, min, max, step, callback);
}

/**
  * @brief Show confirmation dialog (public version)
  * @param message Confirmation message to display
  * @param callback Function to call with result (1=confirm, 0=cancel)
  * @retval None
  */
void UI_Confirm(const char* message, void (*callback)(uint8_t))
{
  UI_ShowConfirmDialog(message, callback);
}

/* Typedefs needed for implementation but not in header files */
typedef struct {
    CrossoverSettings_t crossover;
    CompressorSettings_t compressor;
    LimiterSettings_t limiter;
    DelaySettings_t delay;
} SystemSettings_t;

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
