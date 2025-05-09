 /**
  ******************************************************************************
  * @file           : rotary_encoder.h
  * @brief          : Header file for rotary encoder handling
  * @author         : Audio Crossover DSP Project
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 Audio Crossover Project.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ROTARY_ENCODER_H
#define __ROTARY_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"
#include "tim.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  Rotary Encoder Event Structure
  */
typedef struct {
    int8_t direction;        /* 1: Clockwise, -1: Counter-clockwise, 0: No movement */
    uint8_t buttonPressed;   /* 1: Button pressed, 0: Button not pressed */
    uint8_t buttonReleased;  /* 1: Button released, 0: Button not released */
    uint8_t buttonHeld;      /* 1: Button held for long press, 0: Not held */
} RotaryEvent_t;

/**
  * @brief  Rotary Encoder Mode Enumeration
  */
typedef enum {
    ROTARY_MODE_NORMAL,      /* Standard sensitivity */
    ROTARY_MODE_FINE,        /* Fine adjustment (higher resolution) */
    ROTARY_MODE_COARSE       /* Coarse adjustment (lower resolution) */
} RotaryMode_t;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initialize the rotary encoder
  * @retval None
  */
void RotaryEncoder_Init(void);

/**
  * @brief  Sample the rotary encoder pins for state changes
  * @note   This should be called from a timer ISR or similar
  * @retval None
  */
void RotaryEncoder_Sample(void);

/**
  * @brief  Get the latest rotary encoder event if available
  * @param  event Pointer to event structure to fill
  * @retval 1 if event available, 0 if no event
  */
uint8_t RotaryEncoder_GetEvent(RotaryEvent_t* event);

/**
  * @brief  Set the rotary encoder sensitivity mode
  * @param  mode The desired sensitivity mode
  * @retval None
  */
void RotaryEncoder_SetMode(RotaryMode_t mode);

/**
  * @brief  Get the current button state
  * @retval 1 if button is currently pressed, 0 if released
  */
uint8_t RotaryEncoder_GetButtonState(void);

/**
  * @brief  Reset rotary encoder accumulator
  * @note   Useful when changing menu screens to avoid unwanted actions
  * @retval None
  */
void RotaryEncoder_Reset(void);

/**
  * @brief  Enable or disable rotary encoder processing
  * @param  state 1 to enable, 0 to disable
  * @retval None
  */
void RotaryEncoder_SetEnabled(uint8_t state);

#ifdef __cplusplus
}
#endif

#endif /* __ROTARY_ENCODER_H */
