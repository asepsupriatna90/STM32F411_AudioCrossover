/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body for Audio Crossover DSP Control Panel
  * @author         : Based on STM32F411 Audio Crossover Project
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
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Audio Processing Includes */
#include "audio_driver.h"
#include "audio_processing.h"
#include "crossover.h"
#include "compressor.h"
#include "limiter.h"
#include "delay.h"
#include "audio_preset.h"

/* Interface Includes */
#include "lcd_driver.h"
#include "rotary_encoder.h"
#include "button_handler.h"
#include "menu_system.h"
#include "ui_manager.h"

/* Storage Includes */
#include "flash_storage.h"
#include "preset_manager.h"
#include "factory_presets.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SYSTEM_VERSION "v1.0.0"
#define AUDIO_BUFFER_SIZE 256  // Must be a multiple of 2 and 4 for stereo processing

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static volatile uint8_t systemState = 0;
static volatile uint32_t systemTick = 0;
static AudioBuffer_t inputBuffer;
static AudioBuffer_t outputBuffer;
static SystemSettings_t systemSettings;
static uint8_t activePreset = 0;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void InitSystem(void);
static void InitAudio(void);
static void ProcessAudio(void);
static void HandleUserInterface(void);
static void SaveCurrentSettings(void);
static void LoadSettings(uint8_t presetIndex);
static void Error_Handler(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  InitSystem();
  
  /* Initialize audio subsystem */
  InitAudio();
  
  /* Show welcome screen */
  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_Print("Audio Crossover");
  LCD_SetCursor(0, 1);
  LCD_Print(SYSTEM_VERSION);
  HAL_Delay(2000);
  
  /* Load default preset */
  LoadSettings(PRESET_DEFAULT);
  
  /* Display main menu */
  Menu_ShowMain();

  /* Infinite loop */
  while (1)
  {
    /* Process audio if new samples are available */
    ProcessAudio();
    
    /* Handle user interface (buttons, encoder, menu) */
    HandleUserInterface();
    
    /* Check for system state changes */
    switch (systemState) {
      case SYSTEM_STATE_SAVE_SETTINGS:
        SaveCurrentSettings();
        systemState = SYSTEM_STATE_NORMAL;
        break;
      
      case SYSTEM_STATE_LOAD_PRESET:
        LoadSettings(activePreset);
        systemState = SYSTEM_STATE_NORMAL;
        break;
        
      default:
        /* Normal operation, nothing special to do */
        break;
    }
    
    /* Feed the watchdog if enabled */
    #ifdef USE_IWDG
    HAL_IWDG_Refresh(&hiwdg);
    #endif
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
  
  /** Configure I2S clock for audio
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Initialize all system components
  * @retval None
  */
static void InitSystem(void)
{
  /* Initialize low-level peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_I2S2_Init();
  MX_I2S3_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  
  /* Initialize display */
  LCD_Init();
  
  /* Initialize user input devices */
  RotaryEncoder_Init();
  ButtonHandler_Init();
  
  /* Initialize menu system */
  Menu_Init();
  UI_Init();
  
  /* Initialize storage */
  Flash_Init();
  PresetManager_Init();
  
  /* Start timers */
  HAL_TIM_Base_Start_IT(&htim2); // System tick for UI refresh
  HAL_TIM_Base_Start_IT(&htim3); // Used for encoder sampling
  
  /* Notify system initialization complete */
  #ifdef DEBUG
  printf("System initialization complete\r\n");
  #endif
}

/**
  * @brief Initialize audio subsystem and prepare buffers
  * @retval None
  */
static void InitAudio(void)
{
  /* Initialize audio buffers */
  for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
    inputBuffer.data[i] = 0;
    outputBuffer.data[i] = 0;
  }
  
  /* Initialize audio driver and codec */
  AudioDriver_Init();
  
  /* Initialize DSP modules */
  AudioProcessing_Init();
  Crossover_Init();
  Compressor_Init();
  Limiter_Init();
  Delay_Init();
  
  /* Initialize audio preset system */
  AudioPreset_Init();
  
  /* Start audio streaming */
  AudioDriver_Start();
  
  /* Notify audio initialization complete */
  #ifdef DEBUG
  printf("Audio initialization complete\r\n");
  #endif
}

/**
  * @brief Process audio data if new samples are available
  * @retval None
  */
static void ProcessAudio(void)
{
  if (AudioDriver_IsBufferReady()) {
    /* Get input samples from audio driver */
    AudioDriver_GetSamples(&inputBuffer);
    
    /* Apply audio processing chain */
    AudioProcessing_Process(&inputBuffer, &outputBuffer, &systemSettings);
    
    /* Send processed samples to output */
    AudioDriver_SendSamples(&outputBuffer);
  }
}

/**
  * @brief Handle user interface updates and input
  * @retval None
  */
static void HandleUserInterface(void)
{
  ButtonEvent_t buttonEvent;
  RotaryEvent_t rotaryEvent;
  
  /* Check for button events */
  if (ButtonHandler_GetEvent(&buttonEvent)) {
    UI_HandleButtonEvent(&buttonEvent);
  }
  
  /* Check for rotary encoder events */
  if (RotaryEncoder_GetEvent(&rotaryEvent)) {
    UI_HandleRotaryEvent(&rotaryEvent);
  }
  
  /* Update UI as needed */
  UI_Update();
}

/**
  * @brief Save current system settings to flash memory
  * @retval None
  */
static void SaveCurrentSettings(void)
{
  /* Get current settings from audio modules */
  Crossover_GetSettings(&systemSettings.crossover);
  Compressor_GetSettings(&systemSettings.compressor);
  Limiter_GetSettings(&systemSettings.limiter);
  Delay_GetSettings(&systemSettings.delay);
  
  /* Save settings to selected preset slot */
  PresetManager_SavePreset(activePreset, &systemSettings);
  
  /* Display confirmation message */
  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_Print("Settings saved");
  LCD_SetCursor(0, 1);
  LCD_Print("to preset ");
  LCD_PrintNumber(activePreset);
  HAL_Delay(1000);
  
  /* Return to previous menu */
  Menu_ReturnToPrevious();
}

/**
  * @brief Load settings from a preset
  * @param presetIndex The preset index to load
  * @retval None
  */
static void LoadSettings(uint8_t presetIndex)
{
  /* Check if preset is a factory preset or user preset */
  if (presetIndex < NUM_FACTORY_PRESETS) {
    /* Load factory preset */
    FactoryPresets_GetPreset(presetIndex, &systemSettings);
  } else {
    /* Load user preset from storage */
    PresetManager_LoadPreset(presetIndex, &systemSettings);
  }
  
  /* Apply loaded settings to all audio modules */
  Crossover_SetSettings(&systemSettings.crossover);
  Compressor_SetSettings(&systemSettings.compressor);
  Limiter_SetSettings(&systemSettings.limiter);
  Delay_SetSettings(&systemSettings.delay);
  
  /* Store active preset index */
  activePreset = presetIndex;
  
  /* Display confirmation message */
  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_Print("Preset loaded:");
  LCD_SetCursor(0, 1);
  
  /* Show preset name based on index */
  switch (presetIndex) {
    case PRESET_DEFAULT:
      LCD_Print("Default (Flat)");
      break;
    case PRESET_ROCK:
      LCD_Print("Rock");
      break;
    case PRESET_JAZZ:
      LCD_Print("Jazz");
      break;
    case PRESET_DANGDUT:
      LCD_Print("Dangdut");
      break;
    case PRESET_POP:
      LCD_Print("Pop");
      break;
    default:
      LCD_Print("User preset ");
      LCD_PrintNumber(presetIndex);
      break;
  }
  
  HAL_Delay(1000);
  
  /* Return to previous menu if not during init */
  if (systemState != SYSTEM_STATE_INITIALIZING) {
    Menu_ReturnToPrevious();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* Display error message */
  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_Print("System Error!");
  LCD_SetCursor(0, 1);
  LCD_Print("Please restart");
  
  /* Turn on error LED if available */
  HAL_GPIO_WritePin(ERROR_LED_GPIO_Port, ERROR_LED_Pin, GPIO_PIN_SET);
  
  /* Stop all ongoing processes */
  AudioDriver_Stop();
  
  /* Halt system */
  while (1)
  {
    /* Toggle LED to indicate error state */
    HAL_GPIO_TogglePin(ERROR_LED_GPIO_Port, ERROR_LED_Pin);
    HAL_Delay(500);
  }
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @param  htim TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* Timer for system tick */
  if (htim->Instance == TIM2) {
    systemTick++;
    
    /* Update UI refresh at appropriate rate */
    if (systemTick % UI_REFRESH_INTERVAL == 0) {
      UI_NeedsRefresh();
    }
  }
  /* Timer for encoder sampling */
  else if (htim->Instance == TIM3) {
    RotaryEncoder_Sample();
    ButtonHandler_Sample();
  }
}

/**
  * @brief  I2S RX Transfer completed callback
  * @param  hi2s I2S handle
  * @retval None
  */
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if (hi2s->Instance == I2S2) {
    AudioDriver_NotifyInputReady();
  }
}

/**
  * @brief  I2S TX Transfer completed callback
  * @param  hi2s I2S handle
  * @retval None
  */
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if (hi2s->Instance == I2S3) {
    AudioDriver_NotifyOutputComplete();
  }
}

/* Define the system state constants used in the above code */
#define SYSTEM_STATE_NORMAL          0
#define SYSTEM_STATE_INITIALIZING    1
#define SYSTEM_STATE_SAVE_SETTINGS   2
#define SYSTEM_STATE_LOAD_PRESET     3

/* Define preset indices */
#define PRESET_DEFAULT    0
#define PRESET_ROCK       1
#define PRESET_JAZZ       2
#define PRESET_DANGDUT    3
#define PRESET_POP        4
#define NUM_FACTORY_PRESETS 5

/* Define UI refresh rate */
#define UI_REFRESH_INTERVAL 10  // refresh UI every 10 ticks

/* Define an error LED pin - should match with GPIO init */
#ifndef ERROR_LED_Pin
#define ERROR_LED_Pin GPIO_PIN_13
#define ERROR_LED_GPIO_Port GPIOC
#endif

/**
  * @brief  Audio buffer structure
  */
typedef struct {
    int16_t data[AUDIO_BUFFER_SIZE];
} AudioBuffer_t;

/**
  * @brief  System settings structure that contains all DSP settings
  */
typedef struct {
    CrossoverSettings_t crossover;
    CompressorSettings_t compressor;
    LimiterSettings_t limiter;
    DelaySettings_t delay;
    // Add any additional module settings here
} SystemSettings_t;

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/