 /**
  ******************************************************************************
  * @file           : button_handler.c
  * @brief          : Button handling and debouncing implementation
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
#include "button_handler.h"
#include "gpio.h"
#include "tim.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct {
  GPIO_TypeDef*   port;    // GPIO port
  uint16_t        pin;     // GPIO pin
  GPIO_PinState   activeState; // Active state (typically GPIO_PIN_RESET for pull-up)
  uint32_t        debounceCounter;  // For debounce handling
  uint8_t         stableState;      // Current stable state after debounce
  uint8_t         previousState;    // Previous stable state
  uint32_t        pressTime;        // Time when button was pressed
  uint32_t        releaseTime;      // Time when button was released
  uint8_t         clickCount;       // For double-click detection
} ButtonConfig_t;

typedef struct {
  ButtonEvent_t   events[BUTTON_QUEUE_SIZE];
  uint8_t         head;
  uint8_t         tail;
  uint8_t         count;
} ButtonEventQueue_t;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define IS_QUEUE_EMPTY(q)      ((q).count == 0)
#define IS_QUEUE_FULL(q)       ((q).count >= BUTTON_QUEUE_SIZE)

/* Private variables ---------------------------------------------------------*/
static ButtonConfig_t buttons[MAX_BUTTONS];
static ButtonEventQueue_t eventQueue;
static volatile uint32_t buttonTick = 0;

/* Private function prototypes -----------------------------------------------*/
static void PushEvent(ButtonID_t button, ButtonState_t state, uint32_t holdTime);
static GPIO_PinState ReadButtonPin(ButtonID_t button);
static void ProcessButtonState(ButtonID_t button);

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Initialize button handler
  * @retval None
  */
void ButtonHandler_Init(void)
{
  /* Configure button definitions */
  
  /* Menu button */
  buttons[BUTTON_MENU].port = MENU_BTN_GPIO_Port;
  buttons[BUTTON_MENU].pin = MENU_BTN_Pin;
  buttons[BUTTON_MENU].activeState = GPIO_PIN_RESET;  // Assuming pull-up
  
  /* Back button */
  buttons[BUTTON_BACK].port = BACK_BTN_GPIO_Port;
  buttons[BUTTON_BACK].pin = BACK_BTN_Pin;
  buttons[BUTTON_BACK].activeState = GPIO_PIN_RESET;  // Assuming pull-up
  
  /* Encoder push button */
  buttons[BUTTON_ENCODER].port = ENC_BTN_GPIO_Port;
  buttons[BUTTON_ENCODER].pin = ENC_BTN_Pin;
  buttons[BUTTON_ENCODER].activeState = GPIO_PIN_RESET;  // Assuming pull-up
  
  /* Preset 1 button */
  buttons[BUTTON_PRESET_1].port = PRESET1_BTN_GPIO_Port;
  buttons[BUTTON_PRESET_1].pin = PRESET1_BTN_Pin;
  buttons[BUTTON_PRESET_1].activeState = GPIO_PIN_RESET;  // Assuming pull-up
  
  /* Preset 2 button */
  buttons[BUTTON_PRESET_2].port = PRESET2_BTN_GPIO_Port;
  buttons[BUTTON_PRESET_2].pin = PRESET2_BTN_Pin;
  buttons[BUTTON_PRESET_2].activeState = GPIO_PIN_RESET;  // Assuming pull-up
  
  /* Preset 3 button */
  buttons[BUTTON_PRESET_3].port = PRESET3_BTN_GPIO_Port;
  buttons[BUTTON_PRESET_3].pin = PRESET3_BTN_Pin;
  buttons[BUTTON_PRESET_3].activeState = GPIO_PIN_RESET;  // Assuming pull-up
  
  /* Initialize all button states */
  for (int i = 0; i < MAX_BUTTONS; i++) {
    buttons[i].debounceCounter = 0;
    buttons[i].stableState = (ReadButtonPin(i) == buttons[i].activeState) ? 1 : 0;
    buttons[i].previousState = buttons[i].stableState;
    buttons[i].pressTime = 0;
    buttons[i].releaseTime = 0;
    buttons[i].clickCount = 0;
  }
  
  /* Initialize event queue */
  eventQueue.head = 0;
  eventQueue.tail = 0;
  eventQueue.count = 0;
  
  /* Initialize tick counter */
  buttonTick = 0;
}

/**
  * @brief  Sample button states and process debouncing
  *         This should be called periodically (e.g., every 1ms)
  * @retval None
  */
void ButtonHandler_Sample(void)
{
  /* Increment tick counter */
  buttonTick++;
  
  /* Check each button */
  for (int i = 0; i < MAX_BUTTONS; i++) {
    /* Read current pin state */
    GPIO_PinState pinState = ReadButtonPin(i);
    uint8_t currentState = (pinState == buttons[i].activeState) ? 1 : 0;
    
    /* Debounce logic */
    if (currentState != buttons[i].stableState) {
      /* Pin state is different from stable state, increment counter */
      buttons[i].debounceCounter++;
      
      if (buttons[i].debounceCounter >= BUTTON_DEBOUNCE_TIME) {
        /* Stable state change detected */
        buttons[i].previousState = buttons[i].stableState;
        buttons[i].stableState = currentState;
        buttons[i].debounceCounter = 0;
        
        /* Process button state change */
        ProcessButtonState(i);
      }
    } else {
      /* Reset debounce counter if pin state is same as stable state */
      buttons[i].debounceCounter = 0;
    }
    
    /* Handle hold state */
    if (buttons[i].stableState && 
        (buttonTick - buttons[i].pressTime) >= BUTTON_HOLD_TIME) {
      /* Button is being held */
      if (buttons[i].previousState != BUTTON_STATE_HELD) {
        buttons[i].previousState = BUTTON_STATE_HELD;
        /* Generate held event */
        PushEvent(i, BUTTON_STATE_HELD, buttonTick - buttons[i].pressTime);
      }
    }
    
    /* Check for double click timeout */
    if (buttons[i].clickCount == 1 && 
        (buttonTick - buttons[i].releaseTime) > BUTTON_DOUBLE_CLICK_TIME) {
      /* Single click confirmed (double click timeout) */
      buttons[i].clickCount = 0;
    }
  }
}

/**
  * @brief  Get next button event from the queue
  * @param  event: Pointer to event structure to fill
  * @retval 1 if event available, 0 if queue is empty
  */
uint8_t ButtonHandler_GetEvent(ButtonEvent_t* event)
{
  if (IS_QUEUE_EMPTY(eventQueue)) {
    return 0;  // No events available
  }
  
  /* Get event from queue */
  *event = eventQueue.events[eventQueue.tail];
  
  /* Update queue tail */
  eventQueue.tail = (eventQueue.tail + 1) % BUTTON_QUEUE_SIZE;
  eventQueue.count--;
  
  return 1;
}

/**
  * @brief  Check if a specific button is currently pressed
  * @param  button: Button ID to check
  * @retval 1 if pressed, 0 if released
  */
uint8_t ButtonHandler_IsPressed(ButtonID_t button)
{
  if (button >= MAX_BUTTONS) {
    return 0;
  }
  
  return buttons[button].stableState;
}

/**
  * @brief  Check if a specific button is currently held
  * @param  button: Button ID to check
  * @retval 1 if held, 0 if not
  */
uint8_t ButtonHandler_IsHeld(ButtonID_t button)
{
  if (button >= MAX_BUTTONS) {
    return 0;
  }
  
  if (buttons[button].stableState && 
      (buttonTick - buttons[button].pressTime) >= BUTTON_HOLD_TIME) {
    return 1;
  }
  
  return 0;
}

/**
  * @brief  Clear all pending button events
  * @retval None
  */
void ButtonHandler_ClearEvents(void)
{
  eventQueue.head = 0;
  eventQueue.tail = 0;
  eventQueue.count = 0;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Process button state changes
  * @param  button: Button ID
  * @retval None
  */
static void ProcessButtonState(ButtonID_t button)
{
  if (buttons[button].stableState == 1) {
    /* Button pressed */
    buttons[button].pressTime = buttonTick;
    PushEvent(button, BUTTON_STATE_PRESSED, 0);
    
    /* Check for double click */
    if (buttons[button].clickCount == 1 && 
        (buttonTick - buttons[button].releaseTime) <= BUTTON_DOUBLE_CLICK_TIME) {
      /* Double click detected */
      buttons[button].clickCount = 0;
      PushEvent(button, BUTTON_STATE_DOUBLE_CLICKED, 0);
    }
  } else {
    /* Button released */
    uint32_t holdTime = buttonTick - buttons[button].pressTime;
    buttons[button].releaseTime = buttonTick;
    
    /* Only report release if not already in HELD state */
    if (buttons[button].previousState != BUTTON_STATE_HELD) {
      PushEvent(button, BUTTON_STATE_RELEASED, holdTime);
      buttons[button].clickCount = 1;  // Start double-click detection
    } else {
      /* Reset click count after a hold */
      buttons[button].clickCount = 0;
    }
  }
}

/**
  * @brief  Push a button event to the queue
  * @param  button: Button ID
  * @param  state: Button state
  * @param  holdTime: How long the button was held (if applicable)
  * @retval None
  */
static void PushEvent(ButtonID_t button, ButtonState_t state, uint32_t holdTime)
{
  /* Check if queue is full */
  if (IS_QUEUE_FULL(eventQueue)) {
    /* Queue is full, discard oldest event */
    eventQueue.tail = (eventQueue.tail + 1) % BUTTON_QUEUE_SIZE;
    eventQueue.count--;
  }
  
  /* Add event to queue */
  eventQueue.events[eventQueue.head].button = button;
  eventQueue.events[eventQueue.head].state = state;
  eventQueue.events[eventQueue.head].holdTime = holdTime;
  
  /* Update queue head */
  eventQueue.head = (eventQueue.head + 1) % BUTTON_QUEUE_SIZE;
  eventQueue.count++;
}

/**
  * @brief  Read button pin state
  * @param  button: Button ID
  * @retval Current GPIO pin state
  */
static GPIO_PinState ReadButtonPin(ButtonID_t button)
{
  if (button >= MAX_BUTTONS) {
    return GPIO_PIN_RESET;  // Default safe state
  }
  
  return HAL_GPIO_ReadPin(buttons[button].port, buttons[button].pin);
}

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
