 /**
  ******************************************************************************
  * @file           : lcd_driver.c
  * @brief          : Implementation of the LCD Driver for HD44780 compatible LCD
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
#include "lcd_driver.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* LCD Dimensions */
#define LCD_COLS        16
#define LCD_ROWS        2

/* Row start addresses for 16x2 LCD */
#define LCD_ROW_0_ADDR  0x00
#define LCD_ROW_1_ADDR  0x40

/* Timing constants */
#define LCD_DELAY_INIT  50      // ms
#define LCD_DELAY_CMD   2       // ms
#define LCD_DELAY_ENABLE 1      // ms
#define LCD_PULSE_DELAY 50      // us

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static LCD_ConfigTypeDef LCD_Config;
static uint8_t LCD_Backlight = LCD_PIN_BL;  // Backlight on by default

/* Private function prototypes -----------------------------------------------*/
static void LCD_InitHardware(void);
static void LCD_Write4Bits(uint8_t data);
static void LCD_SendNibble(uint8_t nibble, uint8_t isData);
static void LCD_Pulse_EN(void);
static void LCD_SetBacklight(uint8_t state);
static void LCD_I2C_Write(uint8_t data);

/* Private user code ---------------------------------------------------------*/

/**
  * @brief  Initialize the LCD
  * @param  None
  * @retval None
  */
void LCD_Init(void)
{
  /* Configure the LCD hardware interface */
  LCD_InitHardware();
  
  /* Power on delay */
  HAL_Delay(LCD_DELAY_INIT);
  
  /* Initialize with 4-bit interface
   * 
   * According to the HD44780 datasheet, when starting in 4-bit mode
   * we need a special initialization sequence:
   * 1. Send 0x03 three times (8-bit mode)
   * 2. Send 0x02 (switch to 4-bit mode)
   */
  
  /* Send 0x3 three times (8-bit mode) */
  LCD_SendNibble(0x3, 0);
  HAL_Delay(5);
  LCD_SendNibble(0x3, 0);
  HAL_Delay(5);
  LCD_SendNibble(0x3, 0);
  HAL_Delay(1);
  
  /* Switch to 4-bit mode */
  LCD_SendNibble(0x2, 0);
  HAL_Delay(1);
  
  /* Initialize the LCD with 4-bit mode, 2 lines, 5x8 dots */
  LCD_SendCommand(LCD_CMD_FUNCTION_SET | LCD_4BIT_MODE | LCD_2LINE | LCD_5x8DOTS);
  
  /* Turn the display on with cursor and blink disabled */
  LCD_SendCommand(LCD_CMD_DISPLAY_CTRL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF);
  
  /* Set the entry mode (cursor moves right, display does not shift) */
  LCD_SendCommand(LCD_CMD_ENTRY_MODE | LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_DEC);
  
  /* Clear the display */
  LCD_Clear();
  
  /* Turn on backlight if using I2C */
  if (LCD_Config.Mode == LCD_MODE_I2C_PCF8574) {
    LCD_BacklightOn();
  }
}

/**
  * @brief  Configure the LCD hardware interface
  * @param  None
  * @retval None
  */
static void LCD_InitHardware(void)
{
  /* Set up the LCD configuration for our specific project
   * This can be modified based on actual hardware connections */
  
  /* Using I2C mode with PCF8574 I/O expander */
  LCD_Config.Mode = LCD_MODE_I2C_PCF8574;
  LCD_Config.hi2c = &hi2c1;         // Use I2C1
  LCD_Config.Address = 0x27 << 1;   // Default address for PCF8574A: 0x27 (shifted for HAL)
  
  /* For direct mode, you would configure as follows:
  LCD_Config.Mode = LCD_MODE_4BIT_DIRECT;
  LCD_Config.RS_Port = LCD_RS_GPIO_Port;
  LCD_Config.RS_Pin = LCD_RS_Pin;
  LCD_Config.EN_Port = LCD_EN_GPIO_Port;
  LCD_Config.EN_Pin = LCD_EN_Pin;
  LCD_Config.D4_Port = LCD_D4_GPIO_Port;
  LCD_Config.D4_Pin = LCD_D4_Pin;
  LCD_Config.D5_Port = LCD_D5_GPIO_Port;
  LCD_Config.D5_Pin = LCD_D5_Pin;
  LCD_Config.D6_Port = LCD_D6_GPIO_Port;
  LCD_Config.D6_Pin = LCD_D6_Pin;
  LCD_Config.D7_Port = LCD_D7_GPIO_Port;
  LCD_Config.D7_Pin = LCD_D7_Pin;
  */
}

/**
  * @brief  Send command to LCD
  * @param  cmd Command to send
  * @retval None
  */
void LCD_SendCommand(uint8_t cmd)
{
  /* RS = 0 for command mode */
  LCD_Write4Bits((cmd & 0xF0) | 0x00);
  LCD_Write4Bits(((cmd << 4) & 0xF0) | 0x00);
  
  /* Some commands require longer delay */
  if (cmd == LCD_CMD_CLEAR || cmd == LCD_CMD_HOME) {
    HAL_Delay(LCD_DELAY_CMD);
  } else {
    HAL_Delay(1);
  }
}

/**
  * @brief  Send data to LCD
  * @param  data Data to send
  * @retval None
  */
void LCD_SendData(uint8_t data)
{
  /* RS = 1 for data mode */
  LCD_Write4Bits((data & 0xF0) | LCD_PIN_RS);
  LCD_Write4Bits(((data << 4) & 0xF0) | LCD_PIN_RS);
  
  /* Short delay after data */
  HAL_Delay(1);
}

/**
  * @brief  Write 4 bits to LCD in the appropriate mode
  * @param  data Data to write (higher 4 bits used)
  * @retval None
  */
static void LCD_Write4Bits(uint8_t data)
{
  if (LCD_Config.Mode == LCD_MODE_I2C_PCF8574) {
    /* I2C mode using PCF8574 I/O expander */
    LCD_I2C_Write(data | LCD_Backlight);
    LCD_Pulse_EN();
  } else {
    /* Direct GPIO mode */
    /* Set the data pins */
    HAL_GPIO_WritePin(LCD_Config.RS_Port, LCD_Config.RS_Pin, (data & LCD_PIN_RS) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    
    if (LCD_Config.RW_Port != NULL) {
      HAL_GPIO_WritePin(LCD_Config.RW_Port, LCD_Config.RW_Pin, (data & LCD_PIN_RW) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    
    HAL_GPIO_WritePin(LCD_Config.D4_Port, LCD_Config.D4_Pin, (data & LCD_PIN_D4) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_Config.D5_Port, LCD_Config.D5_Pin, (data & LCD_PIN_D5) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_Config.D6_Port, LCD_Config.D6_Pin, (data & LCD_PIN_D6) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_Config.D7_Port, LCD_Config.D7_Pin, (data & LCD_PIN_D7) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    
    /* Pulse the enable pin */
    LCD_Pulse_EN();
  }
}

/**
  * @brief  Send a nibble as either data or command
  * @param  nibble Lower 4 bits to send
  * @param  isData 1 for data, 0 for command
  * @retval None
  */
static void LCD_SendNibble(uint8_t nibble, uint8_t isData)
{
  uint8_t data = (nibble & 0x0F) << 4;
  if (isData) {
    data |= LCD_PIN_RS;
  }
  LCD_Write4Bits(data);
}

/**
  * @brief  Pulse the enable pin
  * @param  None
  * @retval None
  */
static void LCD_Pulse_EN(void)
{
  if (LCD_Config.Mode == LCD_MODE_I2C_PCF8574) {
    /* For I2C mode */
    uint8_t data = LCD_Backlight;
    if (LCD_Config.Mode == LCD_MODE_I2C_PCF8574) {
      /* Send with EN low */
      LCD_I2C_Write(data);
      HAL_Delay(1);
      
      /* Send with EN high */
      LCD_I2C_Write(data | LCD_PIN_EN);
      HAL_Delay(1);
      
      /* Send with EN low */
      LCD_I2C_Write(data);
      HAL_Delay(1);
    }
  } else {
    /* For direct GPIO mode */
    HAL_GPIO_WritePin(LCD_Config.EN_Port, LCD_Config.EN_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(LCD_Config.EN_Port, LCD_Config.EN_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(LCD_Config.EN_Port, LCD_Config.EN_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
  }
}

/**
  * @brief  Write data to the I2C LCD module (PCF8574)
  * @param  data Data to write
  * @retval None
  */
static void LCD_I2C_Write(uint8_t data)
{
  HAL_I2C_Master_Transmit(LCD_Config.hi2c, LCD_Config.Address, &data, 1, HAL_MAX_DELAY);
}

/**
  * @brief  Set the backlight state (for I2C mode only)
  * @param  state 1 for on, 0 for off
  * @retval None
  */
static void LCD_SetBacklight(uint8_t state)
{
  if (state) {
    LCD_Backlight = LCD_PIN_BL;
  } else {
    LCD_Backlight = 0;
  }
  
  if (LCD_Config.Mode == LCD_MODE_I2C_PCF8574) {
    LCD_I2C_Write(LCD_Backlight);
  }
}

/**
  * @brief  Clear the LCD display
  * @param  None
  * @retval None
  */
void LCD_Clear(void)
{
  LCD_SendCommand(LCD_CMD_CLEAR);
}

/**
  * @brief  Return cursor to home position
  * @param  None
  * @retval None
  */
void LCD_Home(void)
{
  LCD_SendCommand(LCD_CMD_HOME);
}

/**
  * @brief  Turn on the LCD display
  * @param  None
  * @retval None
  */
void LCD_DisplayOn(void)
{
  LCD_SendCommand(LCD_CMD_DISPLAY_CTRL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF);
}

/**
  * @brief  Turn off the LCD display
  * @param  None
  * @retval None
  */
void LCD_DisplayOff(void)
{
  LCD_SendCommand(LCD_CMD_DISPLAY_CTRL | LCD_DISPLAY_OFF | LCD_CURSOR_OFF | LCD_BLINK_OFF);
}

/**
  * @brief  Turn on the cursor
  * @param  None
  * @retval None
  */
void LCD_CursorOn(void)
{
  LCD_SendCommand(LCD_CMD_DISPLAY_CTRL | LCD_DISPLAY_ON | LCD_CURSOR_ON | LCD_BLINK_OFF);
}

/**
  * @brief  Turn off the cursor
  * @param  None
  * @retval None
  */
void LCD_CursorOff(void)
{
  LCD_SendCommand(LCD_CMD_DISPLAY_CTRL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF);
}

/**
  * @brief  Turn on the cursor blinking
  * @param  None
  * @retval None
  */
void LCD_BlinkOn(void)
{
  LCD_SendCommand(LCD_CMD_DISPLAY_CTRL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_ON);
}

/**
  * @brief  Turn off the cursor blinking
  * @param  None
  * @retval None
  */
void LCD_BlinkOff(void)
{
  LCD_SendCommand(LCD_CMD_DISPLAY_CTRL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF);
}

/**
  * @brief  Turn on the LCD backlight (for I2C mode only)
  * @param  None
  * @retval None
  */
void LCD_BacklightOn(void)
{
  LCD_SetBacklight(1);
}

/**
  * @brief  Turn off the LCD backlight (for I2C mode only)
  * @param  None
  * @retval None
  */
void LCD_BacklightOff(void)
{
  LCD_SetBacklight(0);
}

/**
  * @brief  Set the cursor position
  * @param  col Column position (0-15)
  * @param  row Row position (0-1)
  * @retval None
  */
void LCD_SetCursor(uint8_t col, uint8_t row)
{
  uint8_t row_offsets[] = {LCD_ROW_0_ADDR, LCD_ROW_1_ADDR};
  
  /* Keep within bounds */
  if (row >= LCD_ROWS) {
    row = LCD_ROWS - 1;
  }
  if (col >= LCD_COLS) {
    col = LCD_COLS - 1;
  }
  
  /* Set the position */
  LCD_SendCommand(LCD_CMD_SET_DDRAM_ADDR | (col + row_offsets[row]));
}

/**
  * @brief  Print a string on the LCD
  * @param  str String to print
  * @retval None
  */
void LCD_Print(const char* str)
{
  while (*str) {
    LCD_SendData(*str++);
  }
}

/**
  * @brief  Print a number on the LCD
  * @param  num Number to print
  * @retval None
  */
void LCD_PrintNumber(int32_t num)
{
  char buffer[12]; // Enough for a 32-bit integer
  sprintf(buffer, "%ld", num);
  LCD_Print(buffer);
}

/**
  * @brief  Print a floating point number on the LCD
  * @param  num Number to print
  * @param  precision Number of decimal places
  * @retval None
  */
void LCD_PrintFloat(float num, uint8_t precision)
{
  char buffer[16]; // Buffer for the formatted number
  
  /* Create format string based on precision */
  char format[8];
  sprintf(format, "%%.%df", precision);
  
  /* Format the number */
  sprintf(buffer, format, num);
  
  /* Print to LCD */
  LCD_Print(buffer);
}

/**
  * @brief  Create a custom character in CGRAM
  * @param  location Character location (0-7)
  * @param  charmap Array of 8 bytes containing the character pattern
  * @retval None
  */
void LCD_CreateCustomChar(uint8_t location, const uint8_t charmap[])
{
  /* Keep location within bounds */
  location &= 0x7;
  
  /* Set CGRAM address */
  LCD_SendCommand(LCD_CMD_SET_CGRAM_ADDR | (location << 3));
  
  /* Write character data */
  for (uint8_t i = 0; i < 8; i++) {
    LCD_SendData(charmap[i]);
  }
  
  /* Return to DDRAM address mode */
  LCD_SetCursor(0, 0);
}

/**
  * @brief  Print a custom character on the LCD
  * @param  location Character location (0-7)
  * @retval None
  */
void LCD_PrintCustomChar(uint8_t location)
{
  /* Keep location within bounds */
  location &= 0x7;
  
  /* Print the character */
  LCD_SendData(location);
}

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
