 /**
  ******************************************************************************
  * @file           : lcd_driver.h
  * @brief          : Header for lcd_driver.c file.
  *                   This file contains the common defines and functions prototypes for
  *                   the LCD driver (HD44780 compatible 16x2 LCD)
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
#ifndef __LCD_DRIVER_H
#define __LCD_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"

/* Exported types ------------------------------------------------------------*/
/* LCD communication mode */
typedef enum {
  LCD_MODE_4BIT_DIRECT = 0,  // Direct connection with 4 data pins
  LCD_MODE_I2C_PCF8574 = 1,  // I2C connection with PCF8574 I/O expander
} LCD_ModeTypeDef;

/* LCD configuration */
typedef struct {
  LCD_ModeTypeDef Mode;      // LCD connection mode
  I2C_HandleTypeDef* hi2c;   // I2C handle for I2C mode
  uint8_t Address;           // I2C device address (usually 0x27 or 0x3F)
  GPIO_TypeDef* RS_Port;     // Register Select GPIO port (direct mode)
  uint16_t RS_Pin;           // Register Select GPIO pin (direct mode)
  GPIO_TypeDef* RW_Port;     // Read/Write GPIO port (direct mode, can be NULL)
  uint16_t RW_Pin;           // Read/Write GPIO pin (direct mode, can be NULL)
  GPIO_TypeDef* EN_Port;     // Enable GPIO port (direct mode)
  uint16_t EN_Pin;           // Enable GPIO pin (direct mode)
  GPIO_TypeDef* D4_Port;     // Data 4 GPIO port (direct mode)
  uint16_t D4_Pin;           // Data 4 GPIO pin (direct mode)
  GPIO_TypeDef* D5_Port;     // Data 5 GPIO port (direct mode)
  uint16_t D5_Pin;           // Data 5 GPIO pin (direct mode)
  GPIO_TypeDef* D6_Port;     // Data 6 GPIO port (direct mode)
  uint16_t D6_Pin;           // Data 6 GPIO pin (direct mode)
  GPIO_TypeDef* D7_Port;     // Data 7 GPIO port (direct mode)
  uint16_t D7_Pin;           // Data 7 GPIO pin (direct mode)
} LCD_ConfigTypeDef;

/* Exported constants --------------------------------------------------------*/
/* PCF8574 pin mappings for I2C LCD modules */
#define LCD_PIN_RS    (1 << 0)
#define LCD_PIN_RW    (1 << 1)
#define LCD_PIN_EN    (1 << 2)
#define LCD_PIN_BL    (1 << 3)   // Backlight
#define LCD_PIN_D4    (1 << 4)
#define LCD_PIN_D5    (1 << 5)
#define LCD_PIN_D6    (1 << 6)
#define LCD_PIN_D7    (1 << 7)

/* LCD commands */
#define LCD_CMD_CLEAR           0x01
#define LCD_CMD_HOME            0x02
#define LCD_CMD_ENTRY_MODE      0x04
#define LCD_CMD_DISPLAY_CTRL    0x08
#define LCD_CMD_SHIFT           0x10
#define LCD_CMD_FUNCTION_SET    0x20
#define LCD_CMD_SET_CGRAM_ADDR  0x40
#define LCD_CMD_SET_DDRAM_ADDR  0x80

/* Entry mode command options */
#define LCD_ENTRY_RIGHT         0x00
#define LCD_ENTRY_LEFT          0x02
#define LCD_ENTRY_SHIFT_INC     0x01
#define LCD_ENTRY_SHIFT_DEC     0x00

/* Display control command options */
#define LCD_DISPLAY_ON          0x04
#define LCD_DISPLAY_OFF         0x00
#define LCD_CURSOR_ON           0x02
#define LCD_CURSOR_OFF          0x00
#define LCD_BLINK_ON            0x01
#define LCD_BLINK_OFF           0x00

/* Shift command options */
#define LCD_SHIFT_DISPLAY       0x08
#define LCD_SHIFT_CURSOR        0x00
#define LCD_SHIFT_RIGHT         0x04
#define LCD_SHIFT_LEFT          0x00

/* Function set command options */
#define LCD_8BIT_MODE           0x10
#define LCD_4BIT_MODE           0x00
#define LCD_2LINE               0x08
#define LCD_1LINE               0x00
#define LCD_5x10DOTS            0x04
#define LCD_5x8DOTS             0x00

/* Exported macro ------------------------------------------------------------*/
/* Custom character definition */
#define LCD_MAX_CUSTOM_CHARS    8

/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  Initialize the LCD
  * @param  None
  * @retval None
  */
void LCD_Init(void);

/**
  * @brief  Send command to LCD
  * @param  cmd Command to send
  * @retval None
  */
void LCD_SendCommand(uint8_t cmd);

/**
  * @brief  Send data to LCD
  * @param  data Data to send
  * @retval None
  */
void LCD_SendData(uint8_t data);

/**
  * @brief  Clear the LCD display
  * @param  None
  * @retval None
  */
void LCD_Clear(void);

/**
  * @brief  Return cursor to home position
  * @param  None
  * @retval None
  */
void LCD_Home(void);

/**
  * @brief  Turn on the LCD display
  * @param  None
  * @retval None
  */
void LCD_DisplayOn(void);

/**
  * @brief  Turn off the LCD display
  * @param  None
  * @retval None
  */
void LCD_DisplayOff(void);

/**
  * @brief  Turn on the cursor
  * @param  None
  * @retval None
  */
void LCD_CursorOn(void);

/**
  * @brief  Turn off the cursor
  * @param  None
  * @retval None
  */
void LCD_CursorOff(void);

/**
  * @brief  Turn on the cursor blinking
  * @param  None
  * @retval None
  */
void LCD_BlinkOn(void);

/**
  * @brief  Turn off the cursor blinking
  * @param  None
  * @retval None
  */
void LCD_BlinkOff(void);

/**
  * @brief  Turn on the LCD backlight (for I2C mode only)
  * @param  None
  * @retval None
  */
void LCD_BacklightOn(void);

/**
  * @brief  Turn off the LCD backlight (for I2C mode only)
  * @param  None
  * @retval None
  */
void LCD_BacklightOff(void);

/**
  * @brief  Set the cursor position
  * @param  col Column position (0-15)
  * @param  row Row position (0-1)
  * @retval None
  */
void LCD_SetCursor(uint8_t col, uint8_t row);

/**
  * @brief  Print a string on the LCD
  * @param  str String to print
  * @retval None
  */
void LCD_Print(const char* str);

/**
  * @brief  Print a number on the LCD
  * @param  num Number to print
  * @retval None
  */
void LCD_PrintNumber(int32_t num);

/**
  * @brief  Print a floating point number on the LCD
  * @param  num Number to print
  * @param  precision Number of decimal places
  * @retval None
  */
void LCD_PrintFloat(float num, uint8_t precision);

/**
  * @brief  Create a custom character in CGRAM
  * @param  location Character location (0-7)
  * @param  charmap Array of 8 bytes containing the character pattern
  * @retval None
  */
void LCD_CreateCustomChar(uint8_t location, const uint8_t charmap[]);

/**
  * @brief  Print a custom character on the LCD
  * @param  location Character location (0-7)
  * @retval None
  */
void LCD_PrintCustomChar(uint8_t location);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_DRIVER_H */
