/**
  ******************************************************************************
  * @file           : menu_system.c
  * @brief          : Menu System implementation
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
#include "menu_system.h"
#include "lcd_driver.h"
#include "button_handler.h"
#include "rotary_encoder.h"
#include "audio_preset.h"
#include "crossover.h"
#include "compressor.h"
#include "limiter.h"
#include "delay.h"
#include "ui_manager.h"
#include "factory_presets.h"
#include "preset_manager.h"

/* Private defines ------------------------------------------------------------*/
#define MAX_MENU_DEPTH           5
#define MAX_MENU_ITEMS          10
#define MENU_DISPLAY_ROWS        2
#define MENU_TITLE_ROW           0
#define MENU_ITEM_ROW            1
#define MENU_CURSOR              ">"
#define MENU_ITEM_MAX_LENGTH    15

/* Crossover band definitions */
#define BAND_SUB                 0
#define BAND_LOW                 1
#define BAND_MID                 2
#define BAND_HIGH                3
#define MAX_BANDS                4

/* Menu states */
#define MENU_STATE_BROWSING      0
#define MENU_STATE_EDITING       1
#define MENU_STATE_CONFIRMATION  2

/* Confirmation options */
#define CONFIRM_YES              0
#define CONFIRM_NO               1

/* Private macros -------------------------------------------------------------*/
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/* Private types -------------------------------------------------------------*/
/**
  * @brief Menu item structure
  */
typedef struct {
    char text[MENU_ITEM_MAX_LENGTH + 1];
    uint8_t id;
    void (*callback)(uint8_t);
} MenuItem_t;

/**
  * @brief Menu structure
  */
typedef struct {
    char title[MENU_ITEM_MAX_LENGTH + 1];
    MenuItem_t items[MAX_MENU_ITEMS];
    uint8_t numItems;
    uint8_t currentItem;
    uint8_t topDisplayedItem;
} Menu_t;

/**
  * @brief Parameter edit structure
  */
typedef struct {
    char name[MENU_ITEM_MAX_LENGTH + 1];
    int32_t value;
    int32_t minValue;
    int32_t maxValue;
    int32_t step;
    int32_t originalValue;
    uint8_t precision;        // Number of decimal places to display
    uint8_t paramId;
    uint8_t moduleId;
    uint8_t bandId;
    void (*updateCallback)(uint8_t, uint8_t, int32_t);
} Parameter_t;

/* Private variables ---------------------------------------------------------*/
static Menu_t menuStack[MAX_MENU_DEPTH];
static uint8_t menuDepth = 0;
static uint8_t menuState = MENU_STATE_BROWSING;
static Parameter_t currentParameter;
static uint8_t confirmationOption = CONFIRM_NO;
static uint8_t confirmationAction = 0;
static uint8_t selectedPreset = 0;

/* Forward declarations of menu builders */
static void BuildMainMenu(void);
static void BuildCrossoverMenu(void);
static void BuildCrossoverBandMenu(uint8_t band);
static void BuildCompressorMenu(void);
static void BuildLimiterMenu(void);
static void BuildDelayPhaseMenu(void);
static void BuildDelayBandMenu(uint8_t band);
static void BuildPhaseBandMenu(uint8_t band);
static void BuildPresetMenu(void);
static void BuildLoadPresetMenu(void);
static void BuildSavePresetMenu(void);

/* Forward declarations of menu callbacks */
static void MainMenuCallback(uint8_t itemId);
static void CrossoverMenuCallback(uint8_t itemId);
static void CrossoverBandMenuCallback(uint8_t itemId);
static void CompressorMenuCallback(uint8_t itemId);
static void LimiterMenuCallback(uint8_t itemId);
static void DelayPhaseMenuCallback(uint8_t itemId);
static void DelayBandMenuCallback(uint8_t itemId);
static void PhaseBandMenuCallback(uint8_t itemId);
static void PresetMenuCallback(uint8_t itemId);
static void LoadPresetMenuCallback(uint8_t itemId);
static void SavePresetMenuCallback(uint8_t itemId);

/* Forward declarations of parameter editing functions */
static void EditParameter(const char* name, int32_t value, int32_t minValue, int32_t maxValue, 
                          int32_t step, uint8_t precision, uint8_t moduleId, uint8_t paramId, uint8_t bandId,
                          void (*updateCallback)(uint8_t, uint8_t, int32_t));
static void DisplayParameterEdit(void);
static void ApplyParameterEdit(void);
static void CancelParameterEdit(void);

/* Forward declarations of confirmation functions */
static void ShowConfirmation(const char* message, uint8_t action);
static void HandleConfirmation(void);

/* Forward declarations of parameter update callbacks */
static void UpdateCrossoverParameter(uint8_t band, uint8_t paramId, int32_t value);
static void UpdateCompressorParameter(uint8_t band, uint8_t paramId, int32_t value);
static void UpdateLimiterParameter(uint8_t band, uint8_t paramId, int32_t value);
static void UpdateDelayParameter(uint8_t band, uint8_t paramId, int32_t value);
static void UpdatePhaseParameter(uint8_t band, uint8_t paramId, int32_t value);

/* Menu IDs for main menu */
#define MENU_MAIN_CROSSOVER      0
#define MENU_MAIN_COMPRESSOR     1
#define MENU_MAIN_LIMITER        2
#define MENU_MAIN_DELAY_PHASE    3
#define MENU_MAIN_PRESETS        4
#define MENU_MAIN_ABOUT          5

/* Menu IDs for crossover menu */
#define MENU_CROSSOVER_SUB       0
#define MENU_CROSSOVER_LOW       1
#define MENU_CROSSOVER_MID       2
#define MENU_CROSSOVER_HIGH      3

/* Menu IDs for compressor menu */
#define MENU_COMPRESSOR_THRESHOLD    0
#define MENU_COMPRESSOR_RATIO        1
#define MENU_COMPRESSOR_ATTACK       2
#define MENU_COMPRESSOR_RELEASE      3
#define MENU_COMPRESSOR_MAKEUP       4

/* Menu IDs for limiter menu */
#define MENU_LIMITER_THRESHOLD       0
#define MENU_LIMITER_RELEASE         1

/* Menu IDs for delay/phase menu */
#define MENU_DELAY_PHASE_SUB_DELAY   0
#define MENU_DELAY_PHASE_LOW_DELAY   1
#define MENU_DELAY_PHASE_MID_DELAY   2
#define MENU_DELAY_PHASE_HIGH_DELAY  3
#define MENU_DELAY_PHASE_SUB_PHASE   4
#define MENU_DELAY_PHASE_LOW_PHASE   5
#define MENU_DELAY_PHASE_MID_PHASE   6
#define MENU_DELAY_PHASE_HIGH_PHASE  7

/* Menu IDs for preset menu */
#define MENU_PRESET_LOAD             0
#define MENU_PRESET_SAVE             1

/* Confirmation action IDs */
#define CONFIRM_ACTION_SAVE_PRESET   0
#define CONFIRM_ACTION_LOAD_PRESET   1

/* Module IDs for parameter editing */
#define MODULE_CROSSOVER             0
#define MODULE_COMPRESSOR            1
#define MODULE_LIMITER               2
#define MODULE_DELAY                 3
#define MODULE_PHASE                 4

/* Parameter IDs for crossover */
#define PARAM_CROSSOVER_FREQUENCY    0
#define PARAM_CROSSOVER_TYPE         1
#define PARAM_CROSSOVER_GAIN         2
#define PARAM_CROSSOVER_MUTE         3

/* Parameter IDs for compressor */
#define PARAM_COMPRESSOR_THRESHOLD   0
#define PARAM_COMPRESSOR_RATIO       1
#define PARAM_COMPRESSOR_ATTACK      2
#define PARAM_COMPRESSOR_RELEASE     3
#define PARAM_COMPRESSOR_MAKEUP      4

/* Parameter IDs for limiter */
#define PARAM_LIMITER_THRESHOLD      0
#define PARAM_LIMITER_RELEASE        1

/* Parameter IDs for delay */
#define PARAM_DELAY_TIME             0

/* Parameter IDs for phase */
#define PARAM_PHASE_INVERT           0

/**
  * @brief  Initialize menu system
  * @retval None
  */
void Menu_Init(void)
{
    /* Reset menu state */
    menuDepth = 0;
    menuState = MENU_STATE_BROWSING;
    
    /* Build initial main menu */
    BuildMainMenu();
}

/**
  * @brief  Show main menu
  * @retval None
  */
void Menu_ShowMain(void)
{
    /* Reset to main menu */
    menuDepth = 0;
    BuildMainMenu();
    Menu_Display();
}

/**
  * @brief  Return to previous menu
  * @retval None
  */
void Menu_ReturnToPrevious(void)
{
    if (menuDepth > 0) {
        menuDepth--;
    }
    Menu_Display();
}

/**
  * @brief  Display current menu
  * @retval None
  */
void Menu_Display(void)
{
    Menu_t *currentMenu = &menuStack[menuDepth];
    
    /* Clear display */
    LCD_Clear();
    
    /* Display menu title */
    LCD_SetCursor(0, MENU_TITLE_ROW);
    LCD_Print(currentMenu->title);
    
    /* Display current menu item */
    if (currentMenu->numItems > 0) {
        LCD_SetCursor(0, MENU_ITEM_ROW);
        LCD_Print(MENU_CURSOR);
        LCD_Print(currentMenu->items[currentMenu->currentItem].text);
    } else {
        LCD_SetCursor(0, MENU_ITEM_ROW);
        LCD_Print("No items");
    }
}

/**
  * @brief  Handle rotary encoder movement in menu
  * @param  direction: Direction of rotation (1 = clockwise, -1 = counter-clockwise)
  * @retval None
  */
void Menu_HandleRotary(int8_t direction)
{
    Menu_t *currentMenu = &menuStack[menuDepth];
    
    /* Handle based on menu state */
    switch (menuState) {
        case MENU_STATE_BROWSING:
            if (direction > 0) {
                /* Move to next item */
                if (currentMenu->currentItem < currentMenu->numItems - 1) {
                    currentMenu->currentItem++;
                } else {
                    currentMenu->currentItem = 0; /* Wrap around */
                }
            } else {
                /* Move to previous item */
                if (currentMenu->currentItem > 0) {
                    currentMenu->currentItem--;
                } else {
                    currentMenu->currentItem = currentMenu->numItems - 1; /* Wrap around */
                }
            }
            Menu_Display();
            break;
            
        case MENU_STATE_EDITING:
            /* Adjust parameter value */
            currentParameter.value += direction * currentParameter.step;
            
            /* Clamp to min/max */
            if (currentParameter.value > currentParameter.maxValue) {
                currentParameter.value = currentParameter.maxValue;
            }
            if (currentParameter.value < currentParameter.minValue) {
                currentParameter.value = currentParameter.minValue;
            }
            
            /* Update display */
            DisplayParameterEdit();
            
            /* Call update callback to reflect change in real-time */
            if (currentParameter.updateCallback) {
                currentParameter.updateCallback(currentParameter.bandId, 
                                                currentParameter.paramId,
                                                currentParameter.value);
            }
            break;
            
        case MENU_STATE_CONFIRMATION:
            /* Toggle between yes/no */
            confirmationOption = (confirmationOption == CONFIRM_YES) ? CONFIRM_NO : CONFIRM_YES;
            
            /* Update display */
            LCD_SetCursor(0, 1);
            if (confirmationOption == CONFIRM_YES) {
                LCD_Print("> Yes   No    ");
            } else {
                LCD_Print("  Yes > No    ");
            }
            break;
    }
}

/**
  * @brief  Handle button press in menu
  * @param  buttonId: ID of button pressed
  * @retval None
  */
void Menu_HandleButton(uint8_t buttonId)
{
    Menu_t *currentMenu = &menuStack[menuDepth];
    
    /* Handle based on menu state */
    switch (menuState) {
        case MENU_STATE_BROWSING:
            /* Process based on button type */
            switch (buttonId) {
                case BUTTON_ENCODER:  /* Select current item */
                    if (currentMenu->numItems > 0 && currentMenu->items[currentMenu->currentItem].callback) {
                        currentMenu->items[currentMenu->currentItem].callback(
                            currentMenu->items[currentMenu->currentItem].id);
                    }
                    break;
                    
                case BUTTON_BACK:     /* Return to previous menu */
                    if (menuDepth > 0) {
                        menuDepth--;
                        Menu_Display();
                    }
                    break;
                    
                case BUTTON_HOME:     /* Return to main menu */
                    Menu_ShowMain();
                    break;
                    
                default:
                    /* Ignore other buttons */
                    break;
            }
            break;
            
        case MENU_STATE_EDITING:
            /* Process based on button type */
            switch (buttonId) {
                case BUTTON_ENCODER:  /* Confirm edit */
                    ApplyParameterEdit();
                    menuState = MENU_STATE_BROWSING;
                    Menu_Display();
                    break;
                    
                case BUTTON_BACK:     /* Cancel edit */
                    CancelParameterEdit();
                    menuState = MENU_STATE_BROWSING;
                    Menu_Display();
                    break;
                    
                default:
                    /* Ignore other buttons */
                    break;
            }
            break;
            
        case MENU_STATE_CONFIRMATION:
            /* Process based on button type */
            switch (buttonId) {
                case BUTTON_ENCODER:  /* Confirm selection */
                    HandleConfirmation();
                    menuState = MENU_STATE_BROWSING;
                    Menu_Display();
                    break;
                    
                case BUTTON_BACK:     /* Cancel confirmation */
                    menuState = MENU_STATE_BROWSING;
                    Menu_Display();
                    break;
                    
                default:
                    /* Ignore other buttons */
                    break;
            }
            break;
    }
}

/**
  * @brief  Build the main menu
  * @retval None
  */
static void BuildMainMenu(void)
{
    Menu_t *menu = &menuStack[0];
    uint8_t index = 0;
    
    /* Set menu title */
    strncpy(menu->title, "Main Menu", MENU_ITEM_MAX_LENGTH);
    
    /* Add menu items */
    strncpy(menu->items[index].text, "Crossover", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_MAIN_CROSSOVER;
    menu->items[index].callback = MainMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Compressor", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_MAIN_COMPRESSOR;
    menu->items[index].callback = MainMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Limiter", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_MAIN_LIMITER;
    menu->items[index].callback = MainMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Delay/Phase", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_MAIN_DELAY_PHASE;
    menu->items[index].callback = MainMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Presets", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_MAIN_PRESETS;
    menu->items[index].callback = MainMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "About", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_MAIN_ABOUT;
    menu->items[index].callback = MainMenuCallback;
    index++;
    
    /* Set menu properties */
    menu->numItems = index;
    menu->currentItem = 0;
    menu->topDisplayedItem = 0;
}

/**
  * @brief  Build the crossover menu
  * @retval None
  */
static void BuildCrossoverMenu(void)
{
    Menu_t *menu = &menuStack[menuDepth];
    uint8_t index = 0;
    
    /* Set menu title */
    strncpy(menu->title, "Crossover", MENU_ITEM_MAX_LENGTH);
    
    /* Add menu items */
    strncpy(menu->items[index].text, "Sub Band", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_CROSSOVER_SUB;
    menu->items[index].callback = CrossoverMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Low Band", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_CROSSOVER_LOW;
    menu->items[index].callback = CrossoverMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Mid Band", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_CROSSOVER_MID;
    menu->items[index].callback = CrossoverMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "High Band", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_CROSSOVER_HIGH;
    menu->items[index].callback = CrossoverMenuCallback;
    index++;
    
    /* Set menu properties */
    menu->numItems = index;
    menu->currentItem = 0;
    menu->topDisplayedItem = 0;
}

/**
  * @brief  Build the crossover band menu
  * @param  band: Band index (0=Sub, 1=Low, 2=Mid, 3=High)
  * @retval None
  */
static void BuildCrossoverBandMenu(uint8_t band)
{
    Menu_t *menu = &menuStack[menuDepth];
    uint8_t index = 0;
    char bandNames[MAX_BANDS][5] = {"Sub", "Low", "Mid", "High"};
    
    /* Set menu title based on band */
    snprintf(menu->title, MENU_ITEM_MAX_LENGTH, "%s Band XO", bandNames[band]);
    
    /* Add menu items */
    strncpy(menu->items[index].text, "Frequency", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = PARAM_CROSSOVER_FREQUENCY;
    menu->items[index].callback = CrossoverBandMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Filter Type", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = PARAM_CROSSOVER_TYPE;
    menu->items[index].callback = CrossoverBandMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Gain", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = PARAM_CROSSOVER_GAIN;
    menu->items[index].callback = CrossoverBandMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Mute", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = PARAM_CROSSOVER_MUTE;
    menu->items[index].callback = CrossoverBandMenuCallback;
    index++;
    
    /* Set menu properties */
    menu->numItems = index;
    menu->currentItem = 0;
    menu->topDisplayedItem = 0;
}

/**
  * @brief  Build the compressor menu
  * @retval None
  */
static void BuildCompressorMenu(void)
{
    Menu_t *menu = &menuStack[menuDepth];
    uint8_t index = 0;
    
    /* Set menu title */
    strncpy(menu->title, "Compressor", MENU_ITEM_MAX_LENGTH);
    
    /* Add menu items */
    strncpy(menu->items[index].text, "Threshold", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_COMPRESSOR_THRESHOLD;
    menu->items[index].callback = CompressorMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Ratio", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_COMPRESSOR_RATIO;
    menu->items[index].callback = CompressorMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Attack", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_COMPRESSOR_ATTACK;
    menu->items[index].callback = CompressorMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Release", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_COMPRESSOR_RELEASE;
    menu->items[index].callback = CompressorMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Makeup Gain", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_COMPRESSOR_MAKEUP;
    menu->items[index].callback = CompressorMenuCallback;
    index++;
    
    /* Set menu properties */
    menu->numItems = index;
    menu->currentItem = 0;
    menu->topDisplayedItem = 0;
}

/**
  * @brief  Build the limiter menu
  * @retval None
  */
static void BuildLimiterMenu(void)
{
    Menu_t *menu = &menuStack[menuDepth];
    uint8_t index = 0;
    
    /* Set menu title */
    strncpy(menu->title, "Limiter", MENU_ITEM_MAX_LENGTH);
    
    /* Add menu items */
    strncpy(menu->items[index].text, "Threshold", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_LIMITER_THRESHOLD;
    menu->items[index].callback = LimiterMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Release", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_LIMITER_RELEASE;
    menu->items[index].callback = LimiterMenuCallback;
    index++;
    
    /* Set menu properties */
    menu->numItems = index;
    menu->currentItem = 0;
    menu->topDisplayedItem = 0;
}

/**
  * @brief  Build the delay/phase menu
  * @retval None
  */
static void BuildDelayPhaseMenu(void)
{
    Menu_t *menu = &menuStack[menuDepth];
    uint8_t index = 0;
    
    /* Set menu title */
    strncpy(menu->title, "Delay/Phase", MENU_ITEM_MAX_LENGTH);
    
    /* Add menu items */
    strncpy(menu->items[index].text, "Sub Delay", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_DELAY_PHASE_SUB_DELAY;
    menu->items[index].callback = DelayPhaseMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Low Delay", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_DELAY_PHASE_LOW_DELAY;
    menu->items[index].callback = DelayPhaseMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Mid Delay", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_DELAY_PHASE_MID_DELAY;
    menu->items[index].callback = DelayPhaseMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "High Delay", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_DELAY_PHASE_HIGH_DELAY;
    menu->items[index].callback = DelayPhaseMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Sub Phase", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_DELAY_PHASE_SUB_PHASE;
    menu->items[index].callback = DelayPhaseMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Low Phase", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_DELAY_PHASE_LOW_PHASE;
    menu->items[index].callback = DelayPhaseMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Mid Phase", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_DELAY_PHASE_MID_PHASE;
    menu->items[index].callback = DelayPhaseMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "High Phase", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_DELAY_PHASE_HIGH_PHASE;
    menu->items[index].callback = DelayPhaseMenuCallback;
    index++;
    
    /* Set menu properties */
    menu->numItems = index;
    menu->currentItem = 0;
    menu->topDisplayedItem = 0;
}

/**
  * @brief  Build the preset menu
  * @retval None
  */
static void BuildPresetMenu(void)
{
    Menu_t *menu = &menuStack[menuDepth];
    uint8_t index = 0;
    
    /* Set menu title */
    strncpy(menu->title, "Presets", MENU_ITEM_MAX_LENGTH);
    
    /* Add menu items */
    strncpy(menu->items[index].text, "Load Preset", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_PRESET_LOAD;
    menu->items[index].callback = PresetMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Save Preset", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = MENU_PRESET_SAVE;
    menu->items[index].callback = PresetMenuCallback;
    index++;
    
    /* Set menu properties */
    menu->numItems = index;
    menu->currentItem = 0;
    menu->topDisplayedItem = 0;
}

/**
  * @brief  Build the load preset menu
  * @retval None
  */
static void BuildLoadPresetMenu(void)
{
    Menu_t *menu = &menuStack[menuDepth];
    uint8_t index = 0;
    
    /* Set menu title */
    strncpy(menu->title, "Load Preset", MENU_ITEM_MAX_LENGTH);
    
    /* Add factory presets */
    strncpy(menu->items[index].text, "Default (Flat)", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = 0;
    menu->items[index].callback = LoadPresetMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Rock", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = 1;
    menu->items[index].callback = LoadPresetMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Jazz", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = 2;
    menu->items[index].callback = LoadPresetMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Dangdut", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = 3;
    menu->items[index].callback = LoadPresetMenuCallback;
    index++;
    
    strncpy(menu->items[index].text, "Pop", MENU_ITEM_MAX_LENGTH);
    menu->items[index].id = 4;
    menu->items[index].callback = LoadPresetMenuCallback;
    index++;
    
    /* Add user presets */
    for (uint8_t i = 0; i < PresetManager_GetNumUserPresets(); i++) {
        snprintf(menu->items[index].text, MENU_ITEM_MAX_LENGTH, "User %d", i+1);
        menu->items[index].id = i + 5;  // Start user presets after factory presets
        menu->items[index].callback = LoadPresetMenuCallback;
        index++;
        
        if (index >= MAX_MENU_ITEMS) {
            break;  // Prevent overflow
        }
    }
    
    /* Set menu properties */
    menu->numItems = index;
    menu->currentItem = 0;
    menu->topDisplayedItem = 0;
}

/**
  * @brief  Build the save preset menu
  * @retval None
  */
static void BuildSavePresetMenu(void)
{
    Menu_t *menu = &menuStack[menuDepth];
    uint8_t index = 0;
    uint8_t maxUserPresets = 5; // Maximum number of user presets
    
    /* Set menu title */
    strncpy(menu->title, "Save Preset", MENU_ITEM_MAX_LENGTH);
    
    /* Add user preset slots */
    for (uint8_t i = 0; i < maxUserPresets; i++) {
        if (i < PresetManager_GetNumUserPresets()) {
            snprintf(menu->items[index].text, MENU_ITEM_MAX_LENGTH, "Replace User %d", i+1);
        } else {
            snprintf(menu->items[index].text, MENU_ITEM_MAX_LENGTH, "New User %d", i+1);
        }
        menu->items[index].id = i;
        menu->items[index].callback = SavePresetMenuCallback;
        index++;
    }
    
    /* Set menu properties */
    menu->numItems = index;
    menu->currentItem = 0;
    menu->topDisplayedItem = 0;
}

/**
  * @brief  Handle main menu selection
  * @param  itemId: Selected item ID
  * @retval None
  */
static void MainMenuCallback(uint8_t itemId)
{
    /* Increment menu depth */
    menuDepth++;
    
    /* Build submenu based on selected item */
    switch (itemId) {
        case MENU_MAIN_CROSSOVER:
            BuildCrossoverMenu();
            break;
            
        case MENU_MAIN_COMPRESSOR:
            BuildCompressorMenu();
            break;
            
        case MENU_MAIN_LIMITER:
            BuildLimiterMenu();
            break;
            
        case MENU_MAIN_DELAY_PHASE:
            BuildDelayPhaseMenu();
            break;
            
        case MENU_MAIN_PRESETS:
            BuildPresetMenu();
            break;
            
        case MENU_MAIN_ABOUT:
            /* Display about information */
            LCD_Clear();
            LCD_SetCursor(0, 0);
            LCD_Print("Audio Crossover");
            LCD_SetCursor(0, 1);
            LCD_Print("Ver. 1.0");
            menuDepth--; /* Stay in current menu */
            return;
            
        default:
            /* Unknown item, return to current menu */
            menuDepth--;
            break;
    }
    
    /* Display new menu */
    Menu_Display();
}

/**
  * @brief  Handle crossover menu selection
  * @param  itemId: Selected item ID
  * @retval None
  */
static void CrossoverMenuCallback(uint8_t itemId)
{
    /* Increment menu depth */
    menuDepth++;
    
    /* Build band submenu based on selected item */
    switch (itemId) {
        case MENU_CROSSOVER_SUB:
            BuildCrossoverBandMenu(BAND_SUB);
            break;
            
        case MENU_CROSSOVER_LOW:
            BuildCrossoverBandMenu(BAND_LOW);
            break;
            
        case MENU_CROSSOVER_MID:
            BuildCrossoverBandMenu(BAND_MID);
            break;
            
        case MENU_CROSSOVER_HIGH:
            BuildCrossoverBandMenu(BAND_HIGH);
            break;
            
        default:
            /* Unknown item, return to current menu */
            menuDepth--;
            break;
    }
    
    /* Display new menu */
    Menu_Display();
}

/**
  * @brief  Handle crossover band menu selection
  * @param  itemId: Selected item ID
  * @retval None
  */
static void CrossoverBandMenuCallback(uint8_t itemId)
{
    /* Get band from previous menu */
    uint8_t band = 0;
    switch (menuStack[menuDepth-1].items[menuStack[menuDepth-1].currentItem].id) {
        case MENU_CROSSOVER_SUB: band = BAND_SUB; break;
        case MENU_CROSSOVER_LOW: band = BAND_LOW; break;
        case MENU_CROSSOVER_MID: band = BAND_MID; break;
        case MENU_CROSSOVER_HIGH: band = BAND_HIGH; break;
        default: return; /* Invalid band */
    }
    
    /* Handle based on parameter selected */
    switch (itemId) {
        case PARAM_CROSSOVER_FREQUENCY:
            {
                int32_t minFreq = 20;
                int32_t maxFreq = 20000;
                int32_t step = 10;
                
                /* Adjust range based on band */
                if (band == BAND_SUB) {
                    maxFreq = 200;
                } else if (band == BAND_LOW) {
                    minFreq = 100;
                    maxFreq = 2000;
                } else if (band == BAND_MID) {
                    minFreq = 500;
                    maxFreq = 8000;
                } else if (band == BAND_HIGH) {
                    minFreq = 2000;
                }
                
                /* Get current frequency */
                int32_t currentFreq = Crossover_GetFrequency(band);
                
                /* Edit parameter */
                EditParameter("Frequency (Hz)", currentFreq, minFreq, maxFreq, step, 0,
                              MODULE_CROSSOVER, PARAM_CROSSOVER_FREQUENCY, band,
                              UpdateCrossoverParameter);
            }
            break;
            
        case PARAM_CROSSOVER_TYPE:
            {
                /* Get current filter type */
                int32_t currentType = Crossover_GetFilterType(band);
                
                /* Edit parameter (filter types: 0=Butterworth, 1=Linkwitz-Riley) */
                EditParameter("Filter Type", currentType, 0, 1, 1, 0,
                              MODULE_CROSSOVER, PARAM_CROSSOVER_TYPE, band,
                              UpdateCrossoverParameter);
            }
            break;
            
        case PARAM_CROSSOVER_GAIN:
            {
                /* Get current gain */
                int32_t currentGain = Crossover_GetGain(band);
                
                /* Edit parameter (gain in 0.1 dB steps) */
                EditParameter("Gain (dB)", currentGain, -200, 120, 1, 1,
                              MODULE_CROSSOVER, PARAM_CROSSOVER_GAIN, band,
                              UpdateCrossoverParameter);
            }
            break;
            
        case PARAM_CROSSOVER_MUTE:
            {
                /* Get current mute state */
                int32_t currentMute = Crossover_GetMute(band);
                
                /* Edit parameter (0=Unmuted, 1=Muted) */
                EditParameter("Mute", currentMute, 0, 1, 1, 0,
                              MODULE_CROSSOVER, PARAM_CROSSOVER_MUTE, band,
                              UpdateCrossoverParameter);
            }
            break;
            
        default:
            /* Unknown parameter */
            break;
    }
}

/**
  * @brief  Handle compressor menu selection
  * @param  itemId: Selected item ID
  * @retval None
  */
static void CompressorMenuCallback(uint8_t itemId)
{
    /* Handle based on parameter selected */
    switch (itemId) {
        case MENU_COMPRESSOR_THRESHOLD:
            {
                /* Get current threshold */
                int32_t currentThreshold = Compressor_GetThreshold();
                
                /* Edit parameter (threshold in 0.1 dB steps) */
                EditParameter("Threshold (dB)", currentThreshold, -600, 0, 1, 1,
                              MODULE_COMPRESSOR, PARAM_COMPRESSOR_THRESHOLD, 0,
                              UpdateCompressorParameter);
            }
            break;
            
        case MENU_COMPRESSOR_RATIO:
            {
                /* Get current ratio */
                int32_t currentRatio = Compressor_GetRatio();
                
                /* Edit parameter (ratio in 0.1 steps) */
                EditParameter("Ratio (x:1)", currentRatio, 10, 100, 1, 1,
                              MODULE_COMPRESSOR, PARAM_COMPRESSOR_RATIO, 0,
                              UpdateCompressorParameter);
            }
            break;
            
        case MENU_COMPRESSOR_ATTACK:
            {
                /* Get current attack time */
                int32_t currentAttack = Compressor_GetAttack();
                
                /* Edit parameter (attack in ms) */
                EditParameter("Attack (ms)", currentAttack, 1, 200, 1, 0,
                              MODULE_COMPRESSOR, PARAM_COMPRESSOR_ATTACK, 0,
                              UpdateCompressorParameter);
            }
            break;
            
        case MENU_COMPRESSOR_RELEASE:
            {
                /* Get current release time */
                int32_t currentRelease = Compressor_GetRelease();
                
                /* Edit parameter (release in ms) */
                EditParameter("Release (ms)", currentRelease, 10, 1000, 10, 0,
                              MODULE_COMPRESSOR, PARAM_COMPRESSOR_RELEASE, 0,
                              UpdateCompressorParameter);
            }
            break;
            
        case MENU_COMPRESSOR_MAKEUP:
            {
                /* Get current makeup gain */
                int32_t currentMakeup = Compressor_GetMakeupGain();
                
                /* Edit parameter (makeup gain in 0.1 dB steps) */
                EditParameter("Makeup (dB)", currentMakeup, 0, 200, 1, 1,
                              MODULE_COMPRESSOR, PARAM_COMPRESSOR_MAKEUP, 0,
                              UpdateCompressorParameter);
            }
            break;
            
        default:
            /* Unknown parameter */
            break;
    }
}

/**
  * @brief  Handle limiter menu selection
  * @param  itemId: Selected item ID
  * @retval None
  */
static void LimiterMenuCallback(uint8_t itemId)
{
    /* Handle based on parameter selected */
    switch (itemId) {
        case MENU_LIMITER_THRESHOLD:
            {
                /* Get current threshold */
                int32_t currentThreshold = Limiter_GetThreshold();
                
                /* Edit parameter (threshold in 0.1 dB steps) */
                EditParameter("Threshold (dB)", currentThreshold, -300, 0, 1, 1,
                              MODULE_LIMITER, PARAM_LIMITER_THRESHOLD, 0,
                              UpdateLimiterParameter);
            }
            break;
            
        case MENU_LIMITER_RELEASE:
            {
                /* Get current release time */
                int32_t currentRelease = Limiter_GetRelease();
                
                /* Edit parameter (release in ms) */
                EditParameter("Release (ms)", currentRelease, 10, 1000, 10, 0,
                              MODULE_LIMITER, PARAM_LIMITER_RELEASE, 0,
                              UpdateLimiterParameter);
            }
            break;
            
        default:
            /* Unknown parameter */
            break;
    }
}

/**
  * @brief  Handle delay/phase menu selection
  * @param  itemId: Selected item ID
  * @retval None
  */
static void DelayPhaseMenuCallback(uint8_t itemId)
{
    /* Increment menu depth */
    menuDepth++;
    
    /* Build submenu based on selected item */
    switch (itemId) {
        case MENU_DELAY_PHASE_SUB_DELAY:
            BuildDelayBandMenu(BAND_SUB);
            break;
            
        case MENU_DELAY_PHASE_LOW_DELAY:
            BuildDelayBandMenu(BAND_LOW);
            break;
            
        case MENU_DELAY_PHASE_MID_DELAY:
            BuildDelayBandMenu(BAND_MID);
            break;
            
        case MENU_DELAY_PHASE_HIGH_DELAY:
            BuildDelayBandMenu(BAND_HIGH);
            break;
            
        case MENU_DELAY_PHASE_SUB_PHASE:
            BuildPhaseBandMenu(BAND_SUB);
            break;
            
        case MENU_DELAY_PHASE_LOW_PHASE:
            BuildPhaseBandMenu(BAND_LOW);
            break;
            
        case MENU_DELAY_PHASE_MID_PHASE:
            BuildPhaseBandMenu(BAND_MID);
            break;
            
        case MENU_DELAY_PHASE_HIGH_PHASE:
            BuildPhaseBandMenu(BAND_HIGH);
            break;
            
        default:
            /* Unknown item, return to current menu */
            menuDepth--;
            break;
    }
    
    /* Display new menu */
    Menu_Display();
}

/**
  * @brief  Build the delay band menu
  * @param  band: Band index (0=Sub, 1=Low, 2=Mid, 3=High)
  * @retval None
  */
static void BuildDelayBandMenu(uint8_t band)
{
    char bandNames[MAX_BANDS][5] = {"Sub", "Low", "Mid", "High"};
    
    /* Get current delay */
    int32_t currentDelay = Delay_GetTime(band);
    
    /* Edit parameter directly (delay in ms) */
    EditParameter("Delay (ms)", currentDelay, 0, 100, 1, 0,
                  MODULE_DELAY, PARAM_DELAY_TIME, band,
                  UpdateDelayParameter);
    
    /* Decrement menu depth since we're not actually building a menu */
    menuDepth--;
}

/**
  * @brief  Build the phase band menu
  * @param  band: Band index (0=Sub, 1=Low, 2=Mid, 3=High)
  * @retval None
  */
static void BuildPhaseBandMenu(uint8_t band)
{
    char bandNames[MAX_BANDS][5] = {"Sub", "Low", "Mid", "High"};
    
    /* Get current phase */
    int32_t currentPhase = Delay_GetPhaseInvert(band);
    
    /* Edit parameter directly (0=Normal, 1=Inverted) */
    EditParameter("Phase Invert", currentPhase, 0, 1, 1, 0,
                  MODULE_PHASE, PARAM_PHASE_INVERT, band,
                  UpdatePhaseParameter);
    
    /* Decrement menu depth since we're not actually building a menu */
    menuDepth--;
}

/**
  * @brief  Handle preset menu selection
  * @param  itemId: Selected item ID
  * @retval None
  */
static void PresetMenuCallback(uint8_t itemId)
{
    /* Increment menu depth */
    menuDepth++;
    
    /* Build submenu based on selected item */
    switch (itemId) {
        case MENU_PRESET_LOAD:
            BuildLoadPresetMenu();
            break;
            
        case MENU_PRESET_SAVE:
            BuildSavePresetMenu();
            break;
            
        default:
            /* Unknown item, return to current menu */
            menuDepth--;
            break;
    }
    
    /* Display new menu */
    Menu_Display();
}

/**
  * @brief  Handle load preset menu selection
  * @param  itemId: Selected item ID
  * @retval None
  */
static void LoadPresetMenuCallback(uint8_t itemId)
{
    /* Store selected preset */
    selectedPreset = itemId;
    
    /* Show confirmation dialog */
    ShowConfirmation("Load preset?", CONFIRM_ACTION_LOAD_PRESET);
}

/**
  * @brief  Handle save preset menu selection
  * @param  itemId: Selected item ID
  * @retval None
  */
static void SavePresetMenuCallback(uint8_t itemId)
{
    /* Store selected preset */
    selectedPreset = itemId;
    
    /* Show confirmation dialog */
    ShowConfirmation("Save preset?", CONFIRM_ACTION_SAVE_PRESET);
}

/**
  * @brief  Edit a parameter
  * @param  name: Parameter name
  * @param  value: Current value
  * @param  minValue: Minimum value
  * @param  maxValue: Maximum value
  * @param  step: Step value for adjustment
  * @param  precision: Number of decimal places to display
  * @param  moduleId: ID of module (crossover, compressor, etc.)
  * @param  paramId: ID of parameter within module
  * @param  bandId: Band ID (for band-specific parameters)
  * @param  updateCallback: Callback to update parameter value
  * @retval None
  */
static void EditParameter(const char* name, int32_t value, int32_t minValue, int32_t maxValue, 
                          int32_t step, uint8_t precision, uint8_t moduleId, uint8_t paramId, uint8_t bandId,
                          void (*updateCallback)(uint8_t, uint8_t, int32_t))
{
    /* Save parameter information */
    strncpy(currentParameter.name, name, MENU_ITEM_MAX_LENGTH);
    currentParameter.value = value;
    currentParameter.minValue = minValue;
    currentParameter.maxValue = maxValue;
    currentParameter.step = step;
    currentParameter.originalValue = value;
    currentParameter.precision = precision;
    currentParameter.moduleId = moduleId;
    currentParameter.paramId = paramId;
    currentParameter.bandId = bandId;
    currentParameter.updateCallback = updateCallback;
    
    /* Switch to editing state */
    menuState = MENU_STATE_EDITING;
    
    /* Display parameter edit screen */
    DisplayParameterEdit();
}

/**
  * @brief  Display parameter edit screen
  * @retval None
  */
static void DisplayParameterEdit(void)
{
    char valueStr[16];
    
    /* Clear display */
    LCD_Clear();
    
    /* Display parameter name */
    LCD_SetCursor(0, 0);
    LCD_Print(currentParameter.name);
    
    /* Format value string based on precision */
    if (currentParameter.precision == 0) {
        /* Integer value */
        snprintf(valueStr, sizeof(valueStr), "%ld", currentParameter.value);
    } else if (currentParameter.precision == 1) {
        /* One decimal place */
        snprintf(valueStr, sizeof(valueStr), "%ld.%01ld", 
                 currentParameter.value / 10,
                 abs(currentParameter.value) % 10);
    } else {
        /* Two decimal places */
        snprintf(valueStr, sizeof(valueStr), "%ld.%02ld", 
                 currentParameter.value / 100,
                 abs(currentParameter.value) % 100);
    }
    
    /* Display value */
    LCD_SetCursor(0, 1);
    LCD_Print(valueStr);
}

/**
  * @brief  Apply parameter edit
  * @retval None
  */
static void ApplyParameterEdit(void)
{
    /* Call update callback with final value */
    if (currentParameter.updateCallback) {
        currentParameter.updateCallback(currentParameter.bandId, 
                                        currentParameter.paramId,
                                        currentParameter.value);
    }
}

/**
  * @brief  Cancel parameter edit
  * @retval None
  */
static void CancelParameterEdit(void)
{
    /* Call update callback with original value to revert */
    if (currentParameter.updateCallback) {
        currentParameter.updateCallback(currentParameter.bandId, 
                                        currentParameter.paramId,
                                        currentParameter.originalValue);
    }
}

/**
  * @brief  Show confirmation dialog
  * @param  message: Confirmation message
  * @param  action: Action ID for confirmation
  * @retval None
  */
static void ShowConfirmation(const char* message, uint8_t action)
{
    /* Save confirmation action */
    confirmationAction = action;
    confirmationOption = CONFIRM_YES;
    
    /* Switch to confirmation state */
    menuState = MENU_STATE_CONFIRMATION;
    
    /* Display confirmation dialog */
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_Print(message);
    LCD_SetCursor(0, 1);
    LCD_Print("> Yes   No    ");
}

/**
  * @brief  Handle confirmation result
  * @retval None
  */
static void HandleConfirmation(void)
{
    /* Process based on confirmation result */
    if (confirmationOption == CONFIRM_YES) {
        /* Handle based on action */
        switch (confirmationAction) {
            case CONFIRM_ACTION_SAVE_PRESET:
                /* Save current settings to selected preset */
                PresetManager_SaveUserPreset(selectedPreset);
                break;
                
            case CONFIRM_ACTION_LOAD_PRESET:
                /* Load selected preset */
                if (selectedPreset < 5) {
                    /* Factory preset */
                    FactoryPresets_Load(selectedPreset);
                } else {
                    /* User preset */
                    PresetManager_LoadUserPreset(selectedPreset - 5);
                }
                break;
                
            default:
                /* Unknown action */
                break;
        }
    }
}

/**
  * @brief  Update crossover parameter
  * @param  band: Band index
  * @param  paramId: Parameter ID
  * @param  value: New parameter value
  * @retval None
  */
static void UpdateCrossoverParameter(uint8_t band, uint8_t paramId, int32_t value)
{
    /* Update parameter based on ID */
    switch (paramId) {
        case PARAM_CROSSOVER_FREQUENCY:
            Crossover_SetFrequency(band, value);
            break;
            
        case PARAM_CROSSOVER_TYPE:
            Crossover_SetFilterType(band, value);
            break;
            
        case PARAM_CROSSOVER_GAIN:
            Crossover_SetGain(band, value);
            break;
            
        case PARAM_CROSSOVER_MUTE:
            Crossover_SetMute(band, value);
            break;
            
        default:
            /* Unknown parameter */
            break;
    }
}

/**
  * @brief  Update compressor parameter
  * @param  band: Not used for compressor (global)
  * @param  paramId: Parameter ID
  * @param  value: New parameter value
  * @retval None
  */
static void UpdateCompressorParameter(uint8_t band, uint8_t paramId, int32_t value)
{
    /* Update parameter based on ID */
    switch (paramId) {
        case PARAM_COMPRESSOR_THRESHOLD:
            Compressor_SetThreshold(value);
            break;
            
        case PARAM_COMPRESSOR_RATIO:
            Compressor_SetRatio(value);
            break;
            
        case PARAM_COMPRESSOR_ATTACK:
            Compressor_SetAttack(value);
            break;
            
        case PARAM_COMPRESSOR_RELEASE:
            Compressor_SetRelease(value);
            break;
            
        case PARAM_COMPRESSOR_MAKEUP:
            Compressor_SetMakeupGain(value);
            break;
            
        default:
            /* Unknown parameter */
            break;
    }
}

/**
  * @brief  Update limiter parameter
  * @param  band: Not used for limiter (global)
  * @param  paramId: Parameter ID
  * @param  value: New parameter value
  * @retval None
  */
static void UpdateLimiterParameter(uint8_t band, uint8_t paramId, int32_t value)
{
    /* Update parameter based on ID */
    switch (paramId) {
        case PARAM_LIMITER_THRESHOLD:
            Limiter_SetThreshold(value);
            break;
            
        case PARAM_LIMITER_RELEASE:
            Limiter_SetRelease(value);
            break;
            
        default:
            /* Unknown parameter */
            break;
    }
}

/**
  * @brief  Update delay parameter
  * @param  band: Band index
  * @param  paramId: Parameter ID
  * @param  value: New parameter value
  * @retval None
  */
static void UpdateDelayParameter(uint8_t band, uint8_t paramId, int32_t value)
{
    /* Update parameter based on ID */
    switch (paramId) {
        case PARAM_DELAY_TIME:
            Delay_SetTime(band, value);
            break;
            
        default:
            /* Unknown parameter */
            break;
    }
}

/**
  * @brief  Update phase parameter
  * @param  band: Band index
  * @param  paramId: Parameter ID
  * @param  value: New parameter value
  * @retval None
  */
static void UpdatePhaseParameter(uint8_t band, uint8_t paramId, int32_t value)
{
    /* Update parameter based on ID */
    switch (paramId) {
        case PARAM_PHASE_INVERT:
            Delay_SetPhaseInvert(band, value);
            break;
            
        default:
            /* Unknown parameter */
            break;
    }
}

/* End of file */