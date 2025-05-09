 /**
  ******************************************************************************
  * @file           : menu_system.c
  * @brief          : Menu navigation and display implementation for Audio Crossover 
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
#include "ui_manager.h"
#include "audio_preset.h"
#include "crossover.h"
#include "compressor.h"
#include "limiter.h"
#include "delay.h"
#include "factory_presets.h"
#include "preset_manager.h"

/* Private define ------------------------------------------------------------*/
#define MAX_MENU_ITEMS          10  // Maximum number of items in a single menu
#define MAX_MENU_DEPTH          5   // Maximum depth of nested menus
#define SCROLL_TIMEOUT          500 // ms before scrolling long text
#define SCROLL_SPEED            300 // ms between each scroll step

/* Private typedef -----------------------------------------------------------*/
typedef enum {
  MENU_STATE_BROWSING,          // Browsing menu items
  MENU_STATE_EDITING,           // Editing a parameter
  MENU_STATE_CONFIRM,           // Confirmation screen
  MENU_STATE_MESSAGE            // Message display
} MenuState_t;

typedef enum {
  MENU_ACTION_NONE,             // No action
  MENU_ACTION_SELECT,           // Select current item
  MENU_ACTION_BACK,             // Go back to previous menu
  MENU_ACTION_EDIT,             // Edit a parameter
  MENU_ACTION_SAVE,             // Save a parameter
  MENU_ACTION_CANCEL            // Cancel editing
} MenuAction_t;

typedef struct {
  char* name;                   // Menu item name
  void (*callback)(void);       // Function to call when selected
  void* parameter;              // Parameter to edit
  float minValue;               // Minimum value for parameter
  float maxValue;               // Maximum value for parameter
  float step;                   // Step size for parameter adjustment
  char* unit;                   // Unit for parameter (Hz, dB, ms, etc.)
  uint8_t decimalPlaces;        // Number of decimal places to display
} MenuItem_t;

typedef struct {
  char* title;                  // Menu title
  MenuItem_t items[MAX_MENU_ITEMS]; // Menu items
  uint8_t numItems;             // Number of items in this menu
  uint8_t currentItem;          // Currently selected item
  uint8_t scrollPosition;       // Scroll position for long menus
} Menu_t;

/* Private variables ---------------------------------------------------------*/
static Menu_t menuStack[MAX_MENU_DEPTH];  // Stack of menus for navigation
static uint8_t menuDepth = 0;             // Current depth in menu stack
static MenuState_t menuState = MENU_STATE_BROWSING; // Current menu state
static float editValue;                   // Value being edited
static char messageText[32];              // Text for message display
static uint32_t messageTimeout = 0;       // Timeout for message display
static uint32_t lastScrollTime = 0;       // Last time menu text was scrolled
static uint8_t textScrollPos = 0;         // Text scroll position for long items

// Forward declarations of menu creation functions
static void CreateMainMenu(void);
static void CreateCrossoverMenu(void);
static void CreateCrossoverBandMenu(uint8_t band);
static void CreateCompressorMenu(void);
static void CreateLimiterMenu(void);
static void CreateDelayMenu(void);
static void CreatePresetsMenu(void);
static void CreateSystemMenu(void);

// Forward declarations of menu callback functions
static void ShowCrossoverMenu(void);
static void ShowCrossoverBandMenu(void);
static void ShowCompressorMenu(void);
static void ShowLimiterMenu(void);
static void ShowDelayMenu(void);
static void ShowPresetsMenu(void);
static void ShowSystemMenu(void);
static void LoadPreset(void);
static void SavePreset(void);
static void ResetSettings(void);
static void ShowAbout(void);

/* Private function prototypes -----------------------------------------------*/
static void Menu_DrawCurrentMenu(void);
static void Menu_HandleInput(MenuAction_t action, int16_t value);
static void Menu_UpdateEditValue(int16_t direction);
static void Menu_FormatParameterValue(char* buffer, float value, uint8_t decimalPlaces, char* unit);
static void Menu_ShowConfirmation(char* message, void (*confirmCallback)(void));
static void Menu_ShowMessage(char* message, uint32_t timeout);
static void Menu_NavigateBack(void);

/**
  * @brief  Initialize the menu system
  * @retval None
  */
void Menu_Init(void)
{
  // Initialize menu stack with main menu
  menuDepth = 0;
  menuState = MENU_STATE_BROWSING;
  CreateMainMenu();
}

/**
  * @brief  Process menu system update
  * @retval None
  */
void Menu_Update(void)
{
  static uint32_t lastUpdateTime = 0;
  uint32_t currentTime = HAL_GetTick();
  
  // Handle message timeout
  if (menuState == MENU_STATE_MESSAGE && currentTime > messageTimeout) {
    menuState = MENU_STATE_BROWSING;
    Menu_DrawCurrentMenu();
  }
  
  // Handle text scrolling for long menu items
  if (menuState == MENU_STATE_BROWSING && currentTime - lastScrollTime > SCROLL_SPEED) {
    Menu_t* currentMenu = &menuStack[menuDepth];
    MenuItem_t* currentItem = &currentMenu->items[currentMenu->currentItem];
    
    // Only scroll if item name is longer than display width
    if (currentItem && strlen(currentItem->name) > 15) {
      textScrollPos++;
      if (textScrollPos > strlen(currentItem->name) - 15) {
        textScrollPos = 0;
        lastScrollTime = currentTime + SCROLL_TIMEOUT; // Pause before starting again
      } else {
        lastScrollTime = currentTime;
      }
      
      // Update only the second line to avoid flicker
      LCD_SetCursor(0, 1);
      LCD_Print("                "); // Clear line
      LCD_SetCursor(0, 1);
      LCD_Print(&currentItem->name[textScrollPos]);
    }
  }
}

/**
  * @brief  Show the main menu
  * @retval None
  */
void Menu_ShowMain(void)
{
  menuDepth = 0;
  menuState = MENU_STATE_BROWSING;
  CreateMainMenu();
  Menu_DrawCurrentMenu();
}

/**
  * @brief  Return to previous menu level
  * @retval None
  */
void Menu_ReturnToPrevious(void)
{
  if (menuDepth > 0) {
    menuDepth--;
  }
  menuState = MENU_STATE_BROWSING;
  Menu_DrawCurrentMenu();
}

/**
  * @brief  Handle button event from UI
  * @param  event: Button event information
  * @retval None
  */
void Menu_HandleButtonEvent(ButtonEvent_t* event)
{
  MenuAction_t action = MENU_ACTION_NONE;
  
  // Convert button events to menu actions
  switch (event->buttonId) {
    case BUTTON_SELECT:
      if (event->eventType == BUTTON_PRESSED) {
        action = MENU_ACTION_SELECT;
      }
      break;
      
    case BUTTON_BACK:
      if (event->eventType == BUTTON_PRESSED) {
        action = MENU_ACTION_BACK;
      }
      break;
      
    case BUTTON_ENCODER:
      if (event->eventType == BUTTON_PRESSED) {
        if (menuState == MENU_STATE_BROWSING) {
          action = MENU_ACTION_SELECT;
        } else if (menuState == MENU_STATE_EDITING) {
          action = MENU_ACTION_SAVE;
        }
      } else if (event->eventType == BUTTON_LONG_PRESSED) {
        if (menuState == MENU_STATE_EDITING) {
          action = MENU_ACTION_CANCEL;
        }
      }
      break;
      
    default:
      break;
  }
  
  // Process the action
  if (action != MENU_ACTION_NONE) {
    Menu_HandleInput(action, 0);
  }
}

/**
  * @brief  Handle rotary encoder event from UI
  * @param  event: Rotary encoder event information
  * @retval None
  */
void Menu_HandleRotaryEvent(RotaryEvent_t* event)
{
  if (menuState == MENU_STATE_BROWSING) {
    // In browsing mode, change selected menu item
    Menu_t* currentMenu = &menuStack[menuDepth];
    int8_t newItem = currentMenu->currentItem + event->direction;
    
    // Wrap around menu
    if (newItem < 0) {
      newItem = currentMenu->numItems - 1;
    } else if (newItem >= currentMenu->numItems) {
      newItem = 0;
    }
    
    // Update selection and redraw
    currentMenu->currentItem = newItem;
    textScrollPos = 0; // Reset text scrolling
    lastScrollTime = HAL_GetTick() + SCROLL_TIMEOUT;
    Menu_DrawCurrentMenu();
  } else if (menuState == MENU_STATE_EDITING) {
    // In editing mode, change parameter value
    Menu_UpdateEditValue(event->direction);
  }
}

/**
  * @brief  Draw the current menu on the LCD
  * @retval None
  */
static void Menu_DrawCurrentMenu(void)
{
  char buffer[17]; // 16 characters + null terminator
  
  // Clear display
  LCD_Clear();
  
  // Check current state
  switch (menuState) {
    case MENU_STATE_BROWSING:
      {
        Menu_t* currentMenu = &menuStack[menuDepth];
        
        // Display menu title on first line
        LCD_SetCursor(0, 0);
        LCD_Print(currentMenu->title);
        
        // Display currently selected item on second line
        if (currentMenu->numItems > 0) {
          LCD_SetCursor(0, 1);
          LCD_Print(currentMenu->items[currentMenu->currentItem].name);
        } else {
          LCD_SetCursor(0, 1);
          LCD_Print("No items");
        }
      }
      break;
      
    case MENU_STATE_EDITING:
      {
        Menu_t* currentMenu = &menuStack[menuDepth];
        MenuItem_t* currentItem = &currentMenu->items[currentMenu->currentItem];
        
        // Display parameter name on first line
        LCD_SetCursor(0, 0);
        LCD_Print(currentItem->name);
        
        // Display parameter value on second line
        LCD_SetCursor(0, 1);
        Menu_FormatParameterValue(buffer, editValue, currentItem->decimalPlaces, currentItem->unit);
        LCD_Print(buffer);
      }
      break;
      
    case MENU_STATE_CONFIRM:
      {
        // Display confirmation message
        LCD_SetCursor(0, 0);
        LCD_Print("Confirm?");
        LCD_SetCursor(0, 1);
        LCD_Print(messageText);
      }
      break;
      
    case MENU_STATE_MESSAGE:
      {
        // Display message
        LCD_SetCursor(0, 0);
        LCD_Print(messageText);
      }
      break;
  }
}

/**
  * @brief  Handle menu navigation and selection
  * @param  action: Menu action to perform
  * @param  value: Value for the action (if any)
  * @retval None
  */
static void Menu_HandleInput(MenuAction_t action, int16_t value)
{
  Menu_t* currentMenu = &menuStack[menuDepth];
  
  switch (menuState) {
    case MENU_STATE_BROWSING:
      switch (action) {
        case MENU_ACTION_SELECT:
          if (currentMenu->numItems > 0) {
            MenuItem_t* currentItem = &currentMenu->items[currentMenu->currentItem];
            
            // Check if this item has a parameter to edit
            if (currentItem->parameter != NULL) {
              // Enter editing mode
              menuState = MENU_STATE_EDITING;
              // Get current parameter value
              editValue = *((float*)currentItem->parameter);
              Menu_DrawCurrentMenu();
            } else if (currentItem->callback != NULL) {
              // Call the menu item's callback function
              currentItem->callback();
            }
          }
          break;
          
        case MENU_ACTION_BACK:
          Menu_NavigateBack();
          break;
          
        default:
          break;
      }
      break;
      
    case MENU_STATE_EDITING:
      switch (action) {
        case MENU_ACTION_SAVE:
          {
            MenuItem_t* currentItem = &currentMenu->items[currentMenu->currentItem];
            // Save the edited value to the parameter
            *((float*)currentItem->parameter) = editValue;
            // Return to browsing mode
            menuState = MENU_STATE_BROWSING;
            Menu_DrawCurrentMenu();
          }
          break;
          
        case MENU_ACTION_CANCEL:
          // Cancel editing and return to browsing mode
          menuState = MENU_STATE_BROWSING;
          Menu_DrawCurrentMenu();
          break;
          
        case MENU_ACTION_BACK:
          // Same as cancel in edit mode
          menuState = MENU_STATE_BROWSING;
          Menu_DrawCurrentMenu();
          break;
          
        default:
          break;
      }
      break;
      
    case MENU_STATE_CONFIRM:
      // Not implemented yet, will be used for confirmation dialogs
      break;
      
    case MENU_STATE_MESSAGE:
      // Any key dismisses a message
      menuState = MENU_STATE_BROWSING;
      Menu_DrawCurrentMenu();
      break;
  }
}

/**
  * @brief  Update parameter value during editing
  * @param  direction: Direction of change (+1 or -1)
  * @retval None
  */
static void Menu_UpdateEditValue(int16_t direction)
{
  Menu_t* currentMenu = &menuStack[menuDepth];
  MenuItem_t* currentItem = &currentMenu->items[currentMenu->currentItem];
  
  // Calculate new value with step size
  float newValue = editValue + (currentItem->step * direction);
  
  // Clamp to min/max range
  if (newValue < currentItem->minValue) {
    newValue = currentItem->minValue;
  } else if (newValue > currentItem->maxValue) {
    newValue = currentItem->maxValue;
  }
  
  // Update value and redraw
  editValue = newValue;
  Menu_DrawCurrentMenu();
}

/**
  * @brief  Format parameter value with proper decimal places and unit
  * @param  buffer: Buffer to store formatted string
  * @param  value: Value to format
  * @param  decimalPlaces: Number of decimal places
  * @param  unit: Unit string (can be NULL)
  * @retval None
  */
static void Menu_FormatParameterValue(char* buffer, float value, uint8_t decimalPlaces, char* unit)
{
  // Format based on decimal places
  switch (decimalPlaces) {
    case 0:
      sprintf(buffer, "%d", (int)value);
      break;
    case 1:
      sprintf(buffer, "%.1f", value);
      break;
    case 2:
      sprintf(buffer, "%.2f", value);
      break;
    case 3:
      sprintf(buffer, "%.3f", value);
      break;
    default:
      sprintf(buffer, "%.2f", value);
      break;
  }
  
  // Add unit if provided
  if (unit != NULL) {
    strcat(buffer, " ");
    strcat(buffer, unit);
  }
}

/**
  * @brief  Show confirmation dialog
  * @param  message: Confirmation message
  * @param  confirmCallback: Function to call if confirmed
  * @retval None
  */
static void Menu_ShowConfirmation(char* message, void (*confirmCallback)(void))
{
  // Store message
  strncpy(messageText, message, sizeof(messageText) - 1);
  messageText[sizeof(messageText) - 1] = '\0';
  
  // TODO: Store callback for later use when implementing confirmations
  
  // Show confirmation dialog
  menuState = MENU_STATE_CONFIRM;
  Menu_DrawCurrentMenu();
}

/**
  * @brief  Show temporary message
  * @param  message: Message to display
  * @param  timeout: Message timeout in milliseconds
  * @retval None
  */
static void Menu_ShowMessage(char* message, uint32_t timeout)
{
  // Store message and timeout
  strncpy(messageText, message, sizeof(messageText) - 1);
  messageText[sizeof(messageText) - 1] = '\0';
  messageTimeout = HAL_GetTick() + timeout;
  
  // Show message
  menuState = MENU_STATE_MESSAGE;
  Menu_DrawCurrentMenu();
}

/**
  * @brief  Navigate back to previous menu
  * @retval None
  */
static void Menu_NavigateBack(void)
{
  if (menuDepth > 0) {
    menuDepth--;
    Menu_DrawCurrentMenu();
  }
}

/*----------------------------------------------------------------------------*/
/* Menu Creation Functions                                                    */
/*----------------------------------------------------------------------------*/

/**
  * @brief  Create main menu
  * @retval None
  */
static void CreateMainMenu(void)
{
  Menu_t* menu = &menuStack[menuDepth];
  
  menu->title = "Main Menu";
  menu->numItems = 0;
  menu->currentItem = 0;
  menu->scrollPosition = 0;
  
  // Add menu items
  menu->items[menu->numItems].name = "Crossover";
  menu->items[menu->numItems].callback = ShowCrossoverMenu;
  menu->items[menu->numItems].parameter = NULL;
  menu->numItems++;
  
  menu->items[menu->numItems].name = "Compressor";
  menu->items[menu->numItems].callback = ShowCompressorMenu;
  menu->items[menu->numItems].parameter = NULL;
  menu->numItems++;
  
  menu->items[menu->numItems].name = "Limiter";
  menu->items[menu->numItems].callback = ShowLimiterMenu;
  menu->items[menu->numItems].parameter = NULL;
  menu->numItems++;
  
  menu->items[menu->numItems].name = "Delay/Phase";
  menu->items[menu->numItems].callback = ShowDelayMenu;
  menu->items[menu->numItems].parameter = NULL;
  menu->numItems++;
  
  menu->items[menu->numItems].name = "Presets";
  menu->items[menu->numItems].callback = ShowPresetsMenu;
  menu->items[menu->numItems].parameter = NULL;
  menu->numItems++;
  
  menu->items[menu->numItems].name = "System";
  menu->items[menu->numItems].callback = ShowSystemMenu;
  menu->items[menu->numItems].parameter = NULL;
  menu->numItems++;
}

/**
  * @brief  Create crossover menu
  * @retval None
  */
static void CreateCrossoverMenu(void)
{
  Menu_t* menu = &menuStack[menuDepth];
  
  menu->title = "Crossover";
  menu->numItems = 0;
  menu->currentItem = 0;
  menu->scrollPosition = 0;
  
  // Add menu items for each band
  menu->items[menu->numItems].name = "Sub Band";
  menu->items[menu->numItems].callback = ShowCrossoverBandMenu;
  menu->items[menu->numItems].parameter = (void*)CROSSOVER_BAND_SUB;
  menu->numItems++;
  
  menu->items[menu->numItems].name = "Low Band";
  menu->items[menu->numItems].callback = ShowCrossoverBandMenu;
  menu->items[menu->numItems].parameter = (void*)CROSSOVER_BAND_LOW;
  menu->numItems++;
  
  menu->items[menu->numItems].name = "Mid Band";
  menu->items[menu->numItems].callback = ShowCrossoverBandMenu;
  menu->items[menu->numItems].parameter = (void*)CROSSOVER_BAND_MID;
  menu->numItems++;
  
  menu->items[menu->numItems].name = "High Band";
  menu->items[menu->numItems].callback = ShowCrossoverBandMenu;
  menu->items[menu->numItems].parameter = (void*)CROSSOVER_BAND_HIGH;
  menu->numItems++;
  
  menu->items[menu->numItems].name = "Filter Type";
  menu->items[menu->numItems].callback = NULL;
  menu->items[menu->numItems].parameter = &Crossover_GetSettings()->filterType;
  menu->items[menu->numItems].minValue = 0;
  menu->items[menu->numItems].maxValue = 1; // 0=Butterworth, 1=Linkwitz-Riley
  menu->items[menu->numItems].step = 1;
  menu->items[menu->numItems].unit = NULL;
  menu->items[menu->numItems].decimalPlaces = 0;
  menu->numItems++;
}

/**
  * @brief  Create crossover band menu
  * @param  band: Band index (0=Sub, 1=Low, 2=Mid, 3=High)
  * @retval None
  */
static void CreateCrossoverBandMenu(uint8_t band)
{
  Menu_t* menu = &menuStack[menuDepth];
  CrossoverSettings_t* settings = Crossover_GetSettings();
  char bandNames[4][10] = {"Sub", "Low", "Mid", "High"};
  
  // Create menu title with band name
  static char title[20];
  sprintf(title, "%s Band", bandNames[band]);
  menu->title = title;
  
  menu->numItems = 0;
  menu->currentItem = 0;
  menu->scrollPosition = 0;
  
  // Add menu items based on band
  if (band == CROSSOVER_BAND_SUB) {
    // Sub band only has high cutoff
    menu->items[menu->numItems].name = "High Cutoff";
    menu->items[menu->numItems].callback = NULL;
    menu->items[menu->numItems].parameter = &settings->bands[band].highCutoff;
    menu->items[menu->numItems].minValue = 20.0f;
    menu->items[menu->numItems].maxValue = 200.0f;
    menu->items[menu->numItems].step = 1.0f;
    menu->items[menu->numItems].unit = "Hz";
    menu->items[menu->numItems].decimalPlaces = 0;
    menu->numItems++;
  } else if (band == CROSSOVER_BAND_HIGH) {
    // High band only has low cutoff
    menu->items[menu->numItems].name = "Low Cutoff";
    menu->items[menu->numItems].callback = NULL;
    menu->items[menu->numItems].parameter = &settings->bands[band].lowCutoff;
    menu->items[menu->numItems].minValue = 2000.0f;
    menu->items[menu->numItems].maxValue = 20000.0f;
    menu->items[menu->numItems].step = 100.0f;
    menu->items[menu->numItems].unit = "Hz";
    menu->items[menu->numItems].decimalPlaces = 0;
    menu->numItems++;
  } else {
    // Mid bands have both low and high cutoff
    menu->items[menu->numItems].name = "Low Cutoff";
    menu->items[menu->numItems].callback = NULL;
    menu->items[menu->numItems].parameter = &settings->bands[band].lowCutoff;
    menu->items[menu->numItems].minValue = (band == CROSSOVER_BAND_LOW) ? 100.0f : 500.0f;
    menu->items[menu->numItems].maxValue = (band == CROSSOVER_BAND_LOW) ? 1000.0f : 4000.0f;
    menu->items[menu->numItems].step = (band == CROSSOVER_BAND_LOW) ? 10.0f : 50.0f;
    menu->items[menu->numItems].unit = "Hz";
    menu->items[menu->numItems].decimalPlaces = 0;
    menu->numItems++;
    
    menu->items[menu->numItems].name = "High Cutoff";
    menu->items[menu->numItems].callback = NULL;
    menu->items[menu->numItems].parameter = &settings->bands[band].highCutoff;
    menu->items[menu->numItems].minValue = (band == CROSSOVER_BAND_LOW) ? 200.0f : 2000.0f;
    menu->items[menu->numItems].maxValue = (band == CROSSOVER_BAND_LOW) ? 2000.0f : 8000.0f;
    menu->items[menu->numItems].step = (band == CROSSOVER_BAND_LOW) ? 50.0f : 100.0f;
    menu->items[menu->numItems].unit = "Hz";
    menu->items[menu->numItems].decimalPlaces = 0;
    menu->numItems++;
  }
  
  // All bands have gain
  menu->items[menu->numItems].name = "Gain";
  menu->items[menu->numItems].callback = NULL;
  menu->items[menu->numItems].parameter = &settings->bands[band].gain;
  menu->items[menu->numItems].minValue = -20.0f;
  menu->items[menu->numItems].maxValue = 20.0f;
  menu->items[menu->numItems].step = 0.5f;
  menu->items[menu->numItems].unit = "dB";
  menu->items[menu->numItems].decimalPlaces = 1;
  menu->numItems++;
  
  // All bands have mute option
  menu->items[menu->numItems].name = "Mute";
  menu->items[menu->numItems].callback = NULL;
  menu->items[menu->numItems].parameter = &settings->bands[band].mute;
  menu->items[menu->numItems].minValue = 0.0f;
  menu->items[menu->numItems].maxValue = 1.0f;
  menu->items[menu->numItems].step = 1.0f;
  menu->items[menu->numItems].unit = NULL;
  menu->items[menu->numItems].decimalPlaces = 0;
  menu->numItems++;
}

/**
  * @brief  Create compressor menu
  * @retval None
  */
static void CreateCompressorMenu(void)
{
  Menu_t* menu = &menuStack[menuDepth];
  CompressorSettings_t* settings = Compressor_GetSettings();
  
  menu->title = "Compressor";
  menu->numItems = 0;
  menu->currentItem = 0;
  menu->scrollPosition = 0;
  
  // Add menu items
  menu->items[menu->numItems].name = "Enabled";
  menu->items[menu->numItems].callback = NULL;
  menu->items[menu->numItems].parameter = &settings->enabled;
  menu->items[menu->numItems].minValue = 0.0f;
  menu->items[menu->numItems].maxValue = 1.0f;
  menu->items[menu->numItems].step = 1.0f;
  menu->items[menu->numItems].unit = NULL;
  menu->items[menu->numItems].decimalPlaces = 0;
  menu->numItems++;
  
  menu->items[menu->numItems].name = "Threshold";
  menu->items[menu->numItems].callback = NULL;
  menu->items[menu->numItems].parameter = &settings->threshold;
  menu->items[menu->numItems].minValue = -60.0f;
  menu->items[menu->numItems].maxValue = 0.0f;
  menu->items[menu->numItems].step = 0.5f;
  menu->items[menu->numItems].unit = "dB";
  menu->items[menu->numItems].decimalPlaces = 1;
  menu->numItems++;
  
  menu->items[menu->numItems].name = "Ratio";
  menu->items[menu->numItems].callback = NULL;
  menu->items[menu->numItems].parameter = &settings->ratio;
  menu->items[menu->numItems].minValue = 1.0f;
  menu->items[menu->numItems].maxValue = 20.0f;
  menu->items[menu->numItems].step = 0.1f;
  menu->items[menu->numItems].unit = ":1";
  menu->items[menu->numItems].decimalPlaces = 1;
  menu->numItems++;
  
  menu->items[menu->numItems].name = "Attack";
  menu->items[menu->numItems].callback = NULL;
  menu->items[menu->numItems].parameter = &settings->attack;
  menu->items[menu->numItems].minValue = 0.1f;
  menu->items[menu->numItems].maxValue = 100.0f;
  menu->items[menu->numItems].step = 0.1f;
  menu->items[menu->numItems].unit = "ms";
  menu->items[menu->numItems].decimalPlaces = 1;
  menu->numItems++;
  
  menu->items[menu->numItems].name = "Release";
  menu->items[menu->numItems].callback = NULL;
  menu->items[menu->numItems].parameter = &settings->release;
  menu->items[menu->numItems].minValue = 10.0f;
  menu->items[menu->numItems].maxValue = 1000.0f;
  menu->items[menu->numItems].step = 10.0f;
  menu->items[menu->numItems].unit = "ms";
  menu->items[menu->numItems].decimalPlaces = 0;
  menu->numItems++;
  
  menu->items[menu->numItems].name = "Makeup Gain";
  menu->items[menu->numItems].callback = NULL;
  menu->items[menu->numItems].parameter = &settings->makeupGain;
  menu->items[menu->numItems].minValue = 0.0f;
  menu->items[menu->numItems].maxValue = 20.0f;
  menu->items[menu->numItems].step = 0.5f;
