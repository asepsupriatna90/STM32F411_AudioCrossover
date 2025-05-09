 /**
  ******************************************************************************
  * @file           : button_handler.h
  * @brief          : Header for button_handler.c file.
  *                   This file contains the common defines and functions
  *                   for button handling and debouncing.
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
#ifndef __BUTTON_HANDLER_H
#define __BUTTON_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  Button identifiers
  */
typedef enum {
  BUTTON_MENU = 0,         // Menu button
  BUTTON_BACK,             // Back button
  BUTTON_ENCODER,          // Encoder push button
  BUTTON_PRESET_1,         // Quick preset access 1
  BUTTON_PRESET_2,         // Quick preset access 2
  BUTTON_PRESET_3,         // Quick preset access 3
  MAX_BUTTONS              // Total number of buttons
} ButtonID_t;

/**
  * @brief  Button states
  */
typedef enum {
  BUTTON_STATE_RELEASED = 0,
  BUTTON_STATE_PRESSED,
  BUTTON_STATE_HELD,
  BUTTON_STATE_DOUBLE_CLICKED
} ButtonState_t;

/**
  * @brief  Button event type
  */
typedef struct {
  ButtonID_t    button;    // Which button triggered the event
  ButtonState_t state;     // Current state of the button
  uint32_t      holdTime;  // How long the button has been held (if applicable)
} ButtonEvent_t;

/* Exported constants --------------------------------------------------------*/
#define BUTTON_DEBOUNCE_TIME          20    // Debounce time in milliseconds
#define BUTTON_HOLD_TIME              1000  // Time in ms to consider a button as held
#define BUTTON_DOUBLE_CLICK_TIME      300   // Max time between clicks for double click detection
#define BUTTON_QUEUE_SIZE             8     // Size of the button event queue

/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
void ButtonHandler_Init(void);
void ButtonHandler_Sample(void);
uint8_t ButtonHandler_GetEvent(ButtonEvent_t* event);
uint8_t ButtonHandler_IsPressed(ButtonID_t button);
uint8_t ButtonHandler_IsHeld(ButtonID_t button);
void ButtonHandler_ClearEvents(void);

#ifdef __cplusplus
}
#endif

#endif /* __BUTTON_HANDLER_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
