 /**
  ******************************************************************************
  * @file           : i2s_config.h
  * @brief          : Header for i2s_config.c file.
  *                   This file contains the common defines and functions for
  *                   I2S configuration and management for audio communication.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __I2S_CONFIG_H
#define __I2S_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2s.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  I2S Status enumeration
  */
typedef enum {
  I2S_STATUS_OK       = 0x00U,
  I2S_STATUS_ERROR    = 0x01U,
  I2S_STATUS_BUSY     = 0x02U,
  I2S_STATUS_TIMEOUT  = 0x03U
} I2S_StatusTypeDef;

/**
  * @brief  Audio Sample Rate enumeration
  */
typedef enum {
  AUDIO_FREQUENCY_8K    = 8000U,   /*!< 8kHz sampling rate    */
  AUDIO_FREQUENCY_11K   = 11025U,  /*!< 11.025kHz sampling rate */
  AUDIO_FREQUENCY_16K   = 16000U,  /*!< 16kHz sampling rate   */
  AUDIO_FREQUENCY_22K   = 22050U,  /*!< 22.05kHz sampling rate */
  AUDIO_FREQUENCY_32K   = 32000U,  /*!< 32kHz sampling rate   */
  AUDIO_FREQUENCY_44K   = 44100U,  /*!< 44.1kHz sampling rate */
  AUDIO_FREQUENCY_48K   = 48000U,  /*!< 48kHz sampling rate   */
  AUDIO_FREQUENCY_96K   = 96000U,  /*!< 96kHz sampling rate   */
  AUDIO_FREQUENCY_192K  = 192000U  /*!< 192kHz sampling rate  */
} AudioFreq_t;

/**
  * @brief  Audio Resolution enumeration
  */
typedef enum {
  AUDIO_RESOLUTION_16B = 0,  /*!< 16-bit audio resolution */
  AUDIO_RESOLUTION_24B = 1,  /*!< 24-bit audio resolution */
  AUDIO_RESOLUTION_32B = 2   /*!< 32-bit audio resolution */
} AudioRes_t;

/**
  * @brief I2S Audio Configuration Structure
  */
typedef struct {
  AudioFreq_t  SampleRate;     /*!< Audio sampling frequency */
  AudioRes_t   Resolution;     /*!< Audio bit resolution    */
  uint8_t      ChannelCount;   /*!< Number of audio channels */
  uint16_t     BufferSize;     /*!< Audio buffer size in samples */
  uint8_t      UseIrq;         /*!< Use IRQ for I2S communication */
} I2S_AudioConfigTypeDef;

/* Exported constants --------------------------------------------------------*/
#define I2S_STANDARD_PHILIPS         0x00000000U
#define I2S_STANDARD_MSB             0x00000010U
#define I2S_STANDARD_LSB             0x00000020U
#define I2S_STANDARD_PCM_SHORT       0x00000030U
#define I2S_STANDARD_PCM_LONG        0x000000B0U

#define I2S_MCLK_ENABLE              1
#define I2S_MCLK_DISABLE             0

/* Default audio configuration */
#define DEFAULT_AUDIO_SAMPLE_RATE    AUDIO_FREQUENCY_48K
#define DEFAULT_AUDIO_RESOLUTION     AUDIO_RESOLUTION_16B
#define DEFAULT_AUDIO_CHANNELS       2  /* Stereo */

/* PCM1808 ADC pins - adjust according to your hardware */
#define PCM1808_SCK_PIN              GPIO_PIN_10
#define PCM1808_SCK_PORT             GPIOB
#define PCM1808_SD_PIN               GPIO_PIN_3
#define PCM1808_SD_PORT              GPIOC
#define PCM1808_LRCK_PIN             GPIO_PIN_12
#define PCM1808_LRCK_PORT            GPIOB
#define PCM1808_FMT_PIN              GPIO_PIN_7  /* Format selection pin */
#define PCM1808_FMT_PORT             GPIOC
#define PCM1808_MD_PIN               GPIO_PIN_8  /* Mode selection pin */
#define PCM1808_MD_PORT              GPIOC

/* PCM5102A DAC pins - adjust according to your hardware */
#define PCM5102A_SCK_PIN             GPIO_PIN_10
#define PCM5102A_SCK_PORT            GPIOC
#define PCM5102A_SD_PIN              GPIO_PIN_12
#define PCM5102A_SD_PORT             GPIOC
#define PCM5102A_LRCK_PIN            GPIO_PIN_7
#define PCM5102A_LRCK_PORT           GPIOC
#define PCM5102A_MUTE_PIN            GPIO_PIN_9  /* Optional mute pin */
#define PCM5102A_MUTE_PORT           GPIOA

/* Exported macro ------------------------------------------------------------*/
#define IS_AUDIO_FREQUENCY(FREQ) (((FREQ) == AUDIO_FREQUENCY_8K)  || \
                                  ((FREQ) == AUDIO_FREQUENCY_11K) || \
                                  ((FREQ) == AUDIO_FREQUENCY_16K) || \
                                  ((FREQ) == AUDIO_FREQUENCY_22K) || \
                                  ((FREQ) == AUDIO_FREQUENCY_32K) || \
                                  ((FREQ) == AUDIO_FREQUENCY_44K) || \
                                  ((FREQ) == AUDIO_FREQUENCY_48K) || \
                                  ((FREQ) == AUDIO_FREQUENCY_96K) || \
                                  ((FREQ) == AUDIO_FREQUENCY_192K))

/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  Initialize I2S interface for audio communication
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure that contains
  *         the configuration information for I2S module
  * @param  Config Pointer to I2S_AudioConfigTypeDef structure that contains
  *         the audio configuration parameters
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_Init(I2S_HandleTypeDef* I2SHandle, I2S_AudioConfigTypeDef* Config);

/**
  * @brief  Initialize PCM1808 ADC
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_InitPCM1808(void);

/**
  * @brief  Initialize PCM5102A DAC
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_InitPCM5102A(void);

/**
  * @brief  Start I2S audio reception with DMA
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure
  * @param  pData Pointer to reception data buffer
  * @param  Size Size of data buffer
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_StartAudioReceive(I2S_HandleTypeDef* I2SHandle, uint16_t* pData, uint16_t Size);

/**
  * @brief  Start I2S audio transmission with DMA
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure
  * @param  pData Pointer to transmission data buffer
  * @param  Size Size of data buffer
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_StartAudioTransmit(I2S_HandleTypeDef* I2SHandle, uint16_t* pData, uint16_t Size);

/**
  * @brief  Stop I2S audio reception
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_StopAudioReceive(I2S_HandleTypeDef* I2SHandle);

/**
  * @brief  Stop I2S audio transmission
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_StopAudioTransmit(I2S_HandleTypeDef* I2SHandle);

/**
  * @brief  Mute or unmute the PCM5102A output
  * @param  Mute 0: unmute, 1: mute
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_SetMute(uint8_t Mute);

/**
  * @brief  Change the audio frequency
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure
  * @param  AudioFreq Audio frequency to be configured
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_SetAudioFreq(I2S_HandleTypeDef* I2SHandle, AudioFreq_t AudioFreq);

/**
  * @brief  Get current audio frequency
  * @retval Current audio frequency
  */
AudioFreq_t I2S_Config_GetAudioFreq(void);

/**
  * @brief  Setup and configure I2S clock
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure
  * @param  AudioFreq Audio frequency to be configured
  * @retval I2S status
  */
I2S_StatusTypeDef I2S_Config_SetupClock(I2S_HandleTypeDef* I2SHandle, AudioFreq_t AudioFreq);

/**
  * @brief  Check if I2S link is busy
  * @param  I2SHandle Pointer to I2S_HandleTypeDef structure
  * @retval 1 if busy, 0 if not busy
  */
uint8_t I2S_Config_IsBusy(I2S_HandleTypeDef* I2SHandle);

#ifdef __cplusplus
}
#endif

#endif /* __I2S_CONFIG_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
