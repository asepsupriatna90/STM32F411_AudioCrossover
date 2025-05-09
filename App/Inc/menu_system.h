 /**
  ******************************************************************************
  * @file           : menu_system.h
  * @brief          : Menu system for Audio Crossover DSP Control Panel
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
#ifndef __MENU_SYSTEM_H
#define __MENU_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "lcd_driver.h"
#include "button_handler.h"
#include "rotary_encoder.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  Menu item structure for menu system
  */
typedef struct MenuItemStruct {
  char* text;                        /* Display text for this menu item */
  uint8_t hasSubMenu;                /* Flag if item has submenu */
  uint8_t subMenuSize;               /* Number of items in submenu if any */
  struct MenuItemStruct* subMenu;    /* Pointer to submenu items */
  void (*actionFunction)(void);      /* Function to call when menu item is selected */
  uint8_t parameterIndex;            /* Parameter index for parameter editing */
} MenuItem_t;

/**
  * @brief  Menu state structure
  */
typedef struct {
  MenuItem_t* currentMenu;           /* Current menu array */
  uint8_t currentMenuSize;           /* Size of current menu */
  uint8_t currentMenuIndex;          /* Selected item in current menu */
  uint8_t menuLevel;                 /* Current menu nesting level */
  MenuItem_t* menuStack[8];          /* Menu navigation history (for back function) */
  uint8_t menuSizeStack[8];          /* Size of each menu in history */
  uint8_t menuIndexStack[8];         /* Index within each menu in history */
  uint8_t isEditing;                 /* Flag for parameter editing mode */
  int32_t currentValue;              /* Current value when editing a parameter */
  int32_t minValue;                  /* Minimum value when editing */
  int32_t maxValue;                  /* Maximum value when editing */
  uint8_t valueStep;                 /* Step size for value increment/decrement */
  void (*valueChangedCallback)(uint8_t, int32_t); /* Callback when value is changed */
} MenuState_t;

/* Exported constants --------------------------------------------------------*/
/* Menu levels */
#define MENU_MAX_LEVEL           7   /* Maximum menu nesting depth */

/* Menu states */
#define MENU_STATE_NAVIGATE      0   /* Normal menu navigation mode */
#define MENU_STATE_EDIT_VALUE    1   /* Editing a parameter value */
#define MENU_STATE_CONFIRMATION  2   /* Confirmation dialog */

/* Menu actions */
#define MENU_ACTION_NONE         0   /* No action */
#define MENU_ACTION_SELECT       1   /* Select current item */
#define MENU_ACTION_BACK         2   /* Go back to previous menu */
#define MENU_ACTION_UP           3   /* Navigate up */
#define MENU_ACTION_DOWN         4   /* Navigate down */
#define MENU_ACTION_SAVE         5   /* Save value */
#define MENU_ACTION_CANCEL       6   /* Cancel editing */

/* Crossover parameters */
#define PARAM_CROSSOVER_SUB_FREQ     0
#define PARAM_CROSSOVER_LOW_FREQ     1
#define PARAM_CROSSOVER_MID_FREQ     2
#define PARAM_CROSSOVER_SUB_GAIN     3
#define PARAM_CROSSOVER_LOW_GAIN     4
#define PARAM_CROSSOVER_MID_GAIN     5
#define PARAM_CROSSOVER_HIGH_GAIN    6
#define PARAM_CROSSOVER_FILTER_TYPE  7

/* Compressor parameters */
#define PARAM_COMP_THRESHOLD         10
#define PARAM_COMP_RATIO             11
#define PARAM_COMP_ATTACK            12
#define PARAM_COMP_RELEASE           13
#define PARAM_COMP_MAKEUP_GAIN       14

/* Limiter parameters */
#define PARAM_LIMITER_THRESHOLD      20
#define PARAM_LIMITER_RELEASE        21

/* Delay parameters */
#define PARAM_DELAY_SUB              30
#define PARAM_DELAY_LOW              31
#define PARAM_DELAY_MID              32
#define PARAM_DELAY_HIGH             33

/* Phase parameters */
#define PARAM_PHASE_SUB              40
#define PARAM_PHASE_LOW              41
#define PARAM_PHASE_MID              42
#define PARAM_PHASE_HIGH             43

/* Preset indices */
#define PRESET_DEFAULT               0
#define PRESET_ROCK                  1
#define PRESET_JAZZ                  2
#define PRESET_DANGDUT               3
#define PRESET_POP                   4
#define NUM_FACTORY_PRESETS          5
#define MAX_USER_PRESETS             5
#define MAX_PRESETS                  (NUM_FACTORY_PRESETS + MAX_USER_PRESETS)

/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
void Menu_Init(void);
void Menu_ShowMain(void);
void Menu_HandleAction(uint8_t action);
void Menu_ProcessRotaryEvent(RotaryEvent_t* event);
void Menu_ProcessButtonEvent(ButtonEvent_t* event);
void Menu_Display(void);
void Menu_ReturnToPrevious(void);
void Menu_StartParameterEdit(uint8_t paramIndex, int32_t initialValue, 
                             int32_t minValue, int32_t maxValue, uint8_t step, 
                             void (*callback)(uint8_t, int32_t));
void Menu_SaveParameter(void);
void Menu_CancelParameterEdit(void);
void Menu_ShowStatus(const char* line1, const char* line2);
void Menu_ShowConfirmation(const char* message, void (*confirmAction)(void), void (*cancelAction)(void));
void Menu_ForceRefresh(void);

/* Menu action callbacks */
void Menu_CrossoverAction(void);
void Menu_CompressorAction(void);
void Menu_LimiterAction(void);
void Menu_DelayAction(void);
void Menu_PhaseAction(void);
void Menu_PresetAction(void);
void Menu_SystemAction(void);
void Menu_LoadPresetAction(void);
void Menu_SavePresetAction(void);
void Menu_DeletePresetAction(void);
void Menu_ResetToDefaultsAction(void);

#ifdef __cplusplus
}
#endif

#endif /* __MENU_SYSTEM_H */
