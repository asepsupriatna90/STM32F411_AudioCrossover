 /**
  ******************************************************************************
  * @file           : rotary_encoder.c
  * @brief          : Rotary encoder handling implementation
  * @author         : Audio Crossover DSP Project
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 Audio Crossover Project.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "rotary_encoder.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ROTARY_DEBOUNCE_TIME     5     /* Debounce time in ms */
#define ROTARY_LONGPRESS_TIME    1000  /* Long press time in ms */
#define ROTARY_QUEUE_SIZE        8     /* Event queue size */

/* Encoder pin definitions - must match with GPIO configuration */
#define ENCODER_CLK_PIN          GPIO_PIN_0  /* Clock pin (A) */
#define ENCODER_DATA_PIN         GPIO_PIN_1  /* Data pin (B) */
#define ENCODER_BUTTON_PIN       GPIO_PIN_2  /* Button pin */

#define ENCODER_GPIO_PORT        GPIOB      /* GPIO port for all encoder pins */

/* Private macro -------------------------------------------------------------*/
#define READ_ENCODER_CLK()   HAL_GPIO_ReadPin(ENCODER_GPIO_PORT, ENCODER_CLK_PIN)
#define READ_ENCODER_DATA()  HAL_GPIO_ReadPin(ENCODER_GPIO_PORT, ENCODER_DATA_PIN)
#define READ_ENCODER_BUTTON() HAL_GPIO_ReadPin(ENCODER_GPIO_PORT, ENCODER_BUTTON_PIN)

/* Private variables ---------------------------------------------------------*/
static volatile uint8_t lastClk = 0;
static volatile uint8_t lastData = 0;
static volatile uint8_t lastButtonState = 0;
static volatile uint32_t buttonPressTime = 0;
static volatile uint32_t lastDebounceTime = 0;
static volatile uint8_t isButtonLongPress = 0;
static volatile uint8_t isEnabled = 1;
static RotaryMode_t currentMode = ROTARY_MODE_NORMAL;

/* Event queue */
static RotaryEvent_t eventQueue[ROTARY_QUEUE_SIZE];
static volatile uint8_t queueHead = 0;
static volatile uint8_t queueTail = 0;
static volatile uint8_t queueCount = 0;

/* External variables --------------------------------------------------------*/
extern volatile uint32_t systemTick;  /* System tick counter from main.c */

/* Private function prototypes -----------------------------------------------*/
static void EnqueueEvent(int8_t direction, uint8_t buttonPressed, uint8_t buttonReleased, uint8_t buttonHeld);
static uint8_t ApplySensitivity(int8_t direction);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize the rotary encoder
  * @retval None
  */
void RotaryEncoder_Init(void)
{
  /* Read initial state of encoder pins */
  lastClk = READ_ENCODER_CLK();
  lastData = READ_ENCODER_DATA();
  lastButtonState = READ_ENCODER_BUTTON();
  
  /* Clear event queue */
  queueHead = 0;
  queueTail = 0;
  queueCount = 0;
  
  /* Set default mode */
  currentMode = ROTARY_MODE_NORMAL;
  
  /* Enable encoder processing */
  isEnabled = 1;
}

/**
  * @brief  Sample the rotary encoder pins for state changes
  * @note   This should be called from a timer ISR or similar at ~1kHz rate
  * @retval None
  */
void RotaryEncoder_Sample(void)
{
  uint8_t currentClk, currentData, currentButton;
  int8_t direction = 0;
  uint8_t buttonPressed = 0;
  uint8_t buttonReleased = 0;
  uint8_t buttonHeld = 0;
  
  /* If encoder processing is disabled, do nothing */
  if (!isEnabled) {
    return;
  }
  
  /* Read current pin states */
  currentClk = READ_ENCODER_CLK();
  currentData = READ_ENCODER_DATA();
  currentButton = READ_ENCODER_BUTTON();
  
  /* Process rotary encoder rotation */
  if (currentClk != lastClk) {
    /* Falling edge detected on clock pin */
    if (currentClk == 0) {
      /* Determine direction based on data pin state */
      direction = (currentData != 0) ? -1 : 1;
      
      /* Apply sensitivity based on current mode */
      if (ApplySensitivity(direction)) {
        /* Enqueue the event if it passes sensitivity filter */
        EnqueueEvent(direction, 0, 0, 0);
      }
    }
    lastClk = currentClk;
  }
  
  /* Process button state with debouncing */
  if (currentButton != lastButtonState) {
    lastDebounceTime = systemTick;
  }
  
  /* If debounce time has passed, process button state change */
  if ((systemTick - lastDebounceTime) > ROTARY_DEBOUNCE_TIME) {
    /* Button press detected */
    if (lastButtonState && !currentButton) {
      buttonPressed = 1;
      buttonPressTime = systemTick;
      isButtonLongPress = 0;
    }
    /* Button release detected */
    else if (!lastButtonState && currentButton) {
      buttonReleased = 1;
      /* If it wasn't a long press, it's a short press */
      if (!isButtonLongPress) {
        /* Enqueue button press + release as a single event */
        EnqueueEvent(0, 1, 1, 0);
      } else {
        /* Just enqueue the release event for long press */
        EnqueueEvent(0, 0, 1, 0);
      }
    }
  }
  
  /* Check for long press */
  if (!currentButton && !isButtonLongPress) {
    if ((systemTick - buttonPressTime) > ROTARY_LONGPRESS_TIME) {
      isButtonLongPress = 1;
      buttonHeld = 1;
      /* Enqueue long press event */
      EnqueueEvent(0, 0, 0, 1);
    }
  }
  
  /* Update last button state */
  lastButtonState = currentButton;
}

/**
  * @brief  Get the latest rotary encoder event if available
  * @param  event Pointer to event structure to fill
  * @retval 1 if event available, 0 if no event
  */
uint8_t RotaryEncoder_GetEvent(RotaryEvent_t* event)
{
  /* Check if queue is empty */
  if (queueCount == 0) {
    return 0;
  }
  
  /* Get event from queue */
  *event = eventQueue[queueTail];
  
  /* Update queue tail with atomic operation */
  __disable_irq();
  queueTail = (queueTail + 1) % ROTARY_QUEUE_SIZE;
  queueCount--;
  __enable_irq();
  
  return 1;
}

/**
  * @brief  Set the rotary encoder sensitivity mode
  * @param  mode The desired sensitivity mode
  * @retval None
  */
void RotaryEncoder_SetMode(RotaryMode_t mode)
{
  currentMode = mode;
}

/**
  * @brief  Get the current button state
  * @retval 1 if button is currently pressed, 0 if released
  */
uint8_t RotaryEncoder_GetButtonState(void)
{
  return !READ_ENCODER_BUTTON();  /* Button is active low */
}

/**
  * @brief  Reset rotary encoder accumulator
  * @note   Useful when changing menu screens to avoid unwanted actions
  * @retval None
  */
void RotaryEncoder_Reset(void)
{
  /* Clear event queue */
  __disable_irq();
  queueHead = 0;
  queueTail = 0;
  queueCount = 0;
  __enable_irq();
}

/**
  * @brief  Enable or disable rotary encoder processing
  * @param  state 1 to enable, 0 to disable
  * @retval None
  */
void RotaryEncoder_SetEnabled(uint8_t state)
{
  isEnabled = state;
  if (!state) {
    RotaryEncoder_Reset();  /* Clear queue when disabling */
  }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Add an event to the event queue
  * @param  direction Rotation direction (-1, 0, 1)
  * @param  buttonPressed Button press flag
  * @param  buttonReleased Button release flag
  * @param  buttonHeld Button long press flag
  * @retval None
  */
static void EnqueueEvent(int8_t direction, uint8_t buttonPressed, uint8_t buttonReleased, uint8_t buttonHeld)
{
  /* If queue is full, oldest event is dropped */
  if (queueCount == ROTARY_QUEUE_SIZE) {
    queueTail = (queueTail + 1) % ROTARY_QUEUE_SIZE;
    queueCount--;
  }
  
  /* Fill event structure */
  eventQueue[queueHead].direction = direction;
  eventQueue[queueHead].buttonPressed = buttonPressed;
  eventQueue[queueHead].buttonReleased = buttonReleased;
  eventQueue[queueHead].buttonHeld = buttonHeld;
  
  /* Update queue head with atomic operation */
  __disable_irq();
  queueHead = (queueHead + 1) % ROTARY_QUEUE_SIZE;
  queueCount++;
  __enable_irq();
}

/**
  * @brief  Apply sensitivity settings to encoder movement
  * @param  direction Raw direction from encoder (-1 or 1)
  * @retval 1 if event should be processed, 0 if filtered out
  */
static void ApplySensitivity(int8_t direction)
{
  static int8_t accumulator = 0;
  static uint8_t counter = 0;
  
  /* Apply sensitivity based on mode */
  switch (currentMode) {
    case ROTARY_MODE_FINE:
      /* Fine mode - require multiple steps for one event */
      accumulator += direction;
      if (accumulator >= 4) {
        accumulator -= 4;
        return 1;  /* Generate a positive event */
      } else if (accumulator <= -4) {
        accumulator += 4;
        return -1;  /* Generate a negative event */
      }
      return 0;  /* No event yet */
      
    case ROTARY_MODE_COARSE:
      /* Coarse mode - amplify movement */
      return direction * 5;  /* Multiply effect by 5 */
      
    case ROTARY_MODE_NORMAL:
    default:
      /* Normal mode - pass through but stabilize */
      counter++;
      if (counter >= 2) {  /* Process every 2nd event to reduce jitter */
        counter = 0;
        return direction;
      }
      return 0;
  }
}

/**
  * @brief  Get the number of events in the queue
  * @retval Number of events waiting
  */
uint8_t RotaryEncoder_GetQueueCount(void)
{
  return queueCount;
}

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
