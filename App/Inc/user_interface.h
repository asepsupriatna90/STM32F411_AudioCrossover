 /**
  ******************************************************************************
  * @file           : user_interface.h
  * @brief          : Header for user_interface.c file.
  *                   This file contains the declarations for the user interface
  *                   manager for the Audio Crossover DSP Control Panel.
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
#ifndef __USER_INTERFACE_H
#define __USER_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lcd_driver.h"
#include "rotary_encoder.h"
#include "button_handler.h"
#include "menu_system.h"

/* Exported types ------------------------------------------------------------*/
/* UI parameter edit mode states */
typedef enum {
  UI_EDIT_NONE,           /* Not in edit mode */
  UI_EDIT_CROSSOVER,      /* Editing crossover parameters */
  UI_EDIT_COMPRESSOR,     /* Editing compressor parameters */
  UI_EDIT_LIMITER,        /* Editing limiter parameters */
  UI_EDIT_DELAY,          /* Editing delay parameters */
  UI_EDIT_PRESET          /* Editing/selecting presets */
} UI_EditMode_t;

/* Parameter selection structure */
typedef struct {
  uint8_t mainCategory;    /* Main menu category */
  uint8_t subCategory;     /* Sub-menu category */
  uint8_t paramIndex;      /* Parameter index within category */
  uint8_t bandIndex;       /* Audio band index (0:Sub, 1:Low, 2:Mid, 3:High) */
} UI_Selection_t;

/* Exported constants --------------------------------------------------------*/
/* Band indices */
#define BAND_SUB                 0
#define BAND_LOW                 1
#define BAND_MID                 2
#define BAND_HIGH                3
#define NUM_BANDS                4

/* Screen update timing */
#define UI_SCREEN_UPDATE_MS      100    /* Screen update interval in ms */

/* Menu categories */
#define MENU_MAIN                0
#define MENU_CROSSOVER           1
#define MENU_COMPRESSOR          2
#define MENU_LIMITER             3
#define MENU_DELAY               4
#define MENU_PRESETS             5
#define MENU_SYSTEM              6

/* Band names */
extern const char* BandNames[NUM_BANDS];

/* Exported functions prototypes ---------------------------------------------*/
void UI_Init(void);
void UI_Update(void);
void UI_NeedsRefresh(void);
void UI_HandleRotaryEvent(RotaryEvent_t* event);
void UI_HandleButtonEvent(ButtonEvent_t* event);
void UI_DisplayValue(float value, const char* unit, uint8_t precision);
void UI_DisplayMainScreen(void);
void UI_DisplayStatusScreen(void);
void UI_SetEditMode(UI_EditMode_t mode);
UI_EditMode_t UI_GetEditMode(void);
void UI_SetSystemState(uint8_t state);
void UI_NotifyPresetLoaded(uint8_t presetIndex);
void UI_NotifySettingsSaved(uint8_t presetIndex);
void UI_ShowMessage(const char* line1, const char* line2, uint16_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* __USER_INTERFACE_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
