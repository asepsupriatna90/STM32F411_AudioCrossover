 /**
  ******************************************************************************
  * @file           : pcm1808.h
  * @brief          : Header file for PCM1808 ADC driver
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
#ifndef __PCM1808_H
#define __PCM1808_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief PCM1808 Configuration Structure
  */
typedef struct {
    I2S_HandleTypeDef *i2s;        /* I2S Interface handle */
    GPIO_TypeDef *fmtGpioPort;     /* FMT pin GPIO port */
    uint16_t fmtPin;               /* FMT pin number */
    GPIO_TypeDef *mdGpioPort;      /* MD pin GPIO port */
    uint16_t mdPin;                /* MD pin number */
    GPIO_TypeDef *sckoGpioPort;    /* SCKO pin GPIO port */
    uint16_t sckoPin;              /* SCKO pin number */
    uint32_t sampleRate;           /* Desired sample rate */
    uint8_t useHwGain;             /* Use hardware gain (0: no, 1: yes) */
    GPIO_TypeDef *hwGainGpioPort;  /* Hardware gain control GPIO port */
    uint16_t hwGainPin;            /* Hardware gain control pin */
} PCM1808_Config_t;

/**
  * @brief PCM1808 Handle Structure
  */
typedef struct {
    PCM1808_Config_t config;       /* PCM1808 configuration */
    uint8_t initialized;           /* Initialization status */
    uint8_t running;               /* Running status */
    void *rxBuffer;                /* Pointer to DMA receive buffer */
    uint32_t rxBufferSize;         /* Size of DMA receive buffer */
    uint8_t overrunError;          /* Flag to indicate overrun error */
} PCM1808_HandleTypeDef;

/* Exported constants --------------------------------------------------------*/
/* PCM1808 Format control (FMT) */
#define PCM1808_FMT_24BIT_MSB     0   /* 24-bit MSB justified */
#define PCM1808_FMT_24BIT_I2S     1   /* 24-bit I2S format */

/* PCM1808 Mode control (MD) */
#define PCM1808_MD_SLAVE          0   /* Slave mode */
#define PCM1808_MD_MASTER         1   /* Master mode */

/* PCM1808 Hardware Gain Settings */
#define PCM1808_GAIN_0DB          0   /* 0dB gain */
#define PCM1808_GAIN_PLUS_3_5DB   1   /* +3.5dB gain */

/* PCM1808 Status Flags */
#define PCM1808_STATUS_OK         0x00
#define PCM1808_STATUS_ERROR      0x01
#define PCM1808_STATUS_BUSY       0x02
#define PCM1808_STATUS_TIMEOUT    0x03

/* Exported macros -----------------------------------------------------------*/
/* PCM1808 Control Pin Macros */
#define PCM1808_SET_FMT(handle, state) \
  HAL_GPIO_WritePin((handle)->config.fmtGpioPort, (handle)->config.fmtPin, (state))

#define PCM1808_SET_MD(handle, state) \
  HAL_GPIO_WritePin((handle)->config.mdGpioPort, (handle)->config.mdPin, (state))

#define PCM1808_SET_HW_GAIN(handle, state) \
  if((handle)->config.useHwGain) \
    HAL_GPIO_WritePin((handle)->config.hwGainGpioPort, (handle)->config.hwGainPin, (state))

/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  Initialize PCM1808 ADC
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval Status (PCM1808_STATUS_OK if successful)
  */
uint8_t PCM1808_Init(PCM1808_HandleTypeDef *hpcm);

/**
  * @brief  Start PCM1808 ADC operation
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @param  pBuffer pointer to the buffer to receive data
  * @param  Size buffer size in number of samples
  * @retval Status (PCM1808_STATUS_OK if successful)
  */
uint8_t PCM1808_Start(PCM1808_HandleTypeDef *hpcm, void *pBuffer, uint32_t Size);

/**
  * @brief  Stop PCM1808 ADC operation
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval Status (PCM1808_STATUS_OK if successful)
  */
uint8_t PCM1808_Stop(PCM1808_HandleTypeDef *hpcm);

/**
  * @brief  Set gain level for PCM1808
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @param  gain gain level (PCM1808_GAIN_0DB or PCM1808_GAIN_PLUS_3_5DB)
  * @retval Status (PCM1808_STATUS_OK if successful)
  */
uint8_t PCM1808_SetGain(PCM1808_HandleTypeDef *hpcm, uint8_t gain);

/**
  * @brief  Check if PCM1808 has detected an overrun error
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval 1 if overrun detected, 0 otherwise
  */
uint8_t PCM1808_HasOverrun(PCM1808_HandleTypeDef *hpcm);

/**
  * @brief  Clear overrun error flag
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval None
  */
void PCM1808_ClearOverrun(PCM1808_HandleTypeDef *hpcm);

/**
  * @brief  Check if PCM1808 is currently running
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval 1 if running, 0 otherwise
  */
uint8_t PCM1808_IsRunning(PCM1808_HandleTypeDef *hpcm);

/**
  * @brief  Handle I2S transfer complete callback
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval None
  */
void PCM1808_RxCpltCallback(PCM1808_HandleTypeDef *hpcm);

/**
  * @brief  Handle I2S error callback
  * @param  hpcm pointer to a PCM1808_HandleTypeDef structure
  * @retval None
  */
void PCM1808_ErrorCallback(PCM1808_HandleTypeDef *hpcm);

#ifdef __cplusplus
}
#endif

#endif /* __PCM1808_H */

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
