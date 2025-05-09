 /**
  ******************************************************************************
  * @file           : factory_presets.c
  * @brief          : Implementation of factory audio presets
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
#include "factory_presets.h"
#include <string.h>  // For memcpy

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Array of preset names for display */
static const char* presetNames[NUM_FACTORY_PRESETS] = {
    "Default (Flat)",
    "Rock",
    "Jazz",
    "Dangdut",
    "Pop"
};

/* Default (Flat) preset - neutral response across the spectrum */
static const SystemSettings_t defaultPreset = {
    /* Crossover Settings */
    .crossover = {
        .enabled = 1,                        /* Crossover enabled */
        .filterType = FILTER_LINKWITZ_RILEY, /* Filter type */
        .order = 4,                          /* 24dB/octave slopes */
        /* Sub band settings */
        .subBand = {
            .enabled = 1,                    /* Sub band enabled */
            .highFreq = 80,                  /* 80Hz high cutoff */
            .gain = 0.0f,                    /* 0dB gain (flat) */
            .phase = 0                       /* Normal phase */
        },
        /* Low band settings */
        .lowBand = {
            .enabled = 1,                    /* Low band enabled */
            .lowFreq = 80,                   /* 80Hz low cutoff */
            .highFreq = 500,                 /* 500Hz high cutoff */
            .gain = 0.0f,                    /* 0dB gain (flat) */
            .phase = 0                       /* Normal phase */
        },
        /* Mid band settings */
        .midBand = {
            .enabled = 1,                    /* Mid band enabled */
            .lowFreq = 500,                  /* 500Hz low cutoff */
            .highFreq = 4000,                /* 4kHz high cutoff */
            .gain = 0.0f,                    /* 0dB gain (flat) */
            .phase = 0                       /* Normal phase */
        },
        /* High band settings */
        .highBand = {
            .enabled = 1,                    /* High band enabled */
            .lowFreq = 4000,                 /* 4kHz low cutoff */
            .gain = 0.0f,                    /* 0dB gain (flat) */
            .phase = 0                       /* Normal phase */
        }
    },
    
    /* Compressor Settings - default mild settings */
    .compressor = {
        .enabled = 0,                        /* Compressor disabled by default */
        .threshold = -24.0f,                 /* Threshold in dB */
        .ratio = 2.0f,                       /* 2:1 ratio */
        .attack = 20.0f,                     /* 20ms attack time */
        .release = 200.0f,                   /* 200ms release time */
        .makeupGain = 0.0f,                  /* No makeup gain */
        .kneeWidth = 6.0f                    /* Soft knee */
    },
    
    /* Limiter Settings - default protection */
    .limiter = {
        .enabled = 1,                        /* Limiter enabled */
        .threshold = 0.0f,                   /* 0dB threshold */
        .release = 50.0f,                    /* 50ms release time */
        .lookaheadMs = 5.0f                  /* 5ms lookahead */
    },
    
    /* Delay Settings - no delay by default */
    .delay = {
        .enabled = 0,                        /* Delay disabled */
        .subBandDelay = 0,                   /* No sub band delay */
        .lowBandDelay = 0,                   /* No low band delay */
        .midBandDelay = 0,                   /* No mid band delay */
        .highBandDelay = 0                   /* No high band delay */
    }
};

/* Rock preset - boosted lows and highs for rock music */
static const SystemSettings_t rockPreset = {
    /* Crossover Settings */
    .crossover = {
        .enabled = 1,
        .filterType = FILTER_LINKWITZ_RILEY,
        .order = 4,
        /* Sub band settings */
        .subBand = {
            .enabled = 1,
            .highFreq = 90,                  /* Slightly higher sub crossover */
            .gain = 3.0f,                    /* +3dB boost for the sub */
            .phase = 0
        },
        /* Low band settings */
        .lowBand = {
            .enabled = 1,
            .lowFreq = 90,
            .highFreq = 600,                 /* Wider low band */
            .gain = 2.0f,                    /* +2dB boost for low mids */
            .phase = 0
        },
        /* Mid band settings */
        .midBand = {
            .enabled = 1,
            .lowFreq = 600,
            .highFreq = 3500,
            .gain = -1.0f,                   /* Slight mid scoop (-1dB) */
            .phase = 0
        },
        /* High band settings */
        .highBand = {
            .enabled = 1,
            .lowFreq = 3500,
            .gain = 2.5f,                    /* +2.5dB boost for highs */
            .phase = 0
        }
    },
    
    /* Compressor Settings - stronger for rock */
    .compressor = {
        .enabled = 1,                        /* Compressor enabled */
        .threshold = -20.0f,
        .ratio = 3.0f,                       /* 3:1 ratio for more punch */
        .attack = 15.0f,                     /* Faster attack */
        .release = 150.0f,                   /* Faster release */
        .makeupGain = 1.5f,                  /* Makeup gain to compensate */
        .kneeWidth = 4.0f
    },
    
    /* Limiter Settings */
    .limiter = {
        .enabled = 1,
        .threshold = -0.5f,                  /* Slightly lower threshold */
        .release = 45.0f,
        .lookaheadMs = 5.0f
    },
    
    /* Delay Settings */
    .delay = {
        .enabled = 0,                        /* No delay for Rock preset */
        .subBandDelay = 0,
        .lowBandDelay = 0,
        .midBandDelay = 0,
        .highBandDelay = 0
    }
};

/* Jazz preset - warm mids, detailed highs */
static const SystemSettings_t jazzPreset = {
    /* Crossover Settings */
    .crossover = {
        .enabled = 1,
        .filterType = FILTER_LINKWITZ_RILEY,
        .order = 4,
        /* Sub band settings */
        .subBand = {
            .enabled = 1,
            .highFreq = 70,                  /* Lower sub crossover */
            .gain = 1.0f,                    /* Subtle sub boost */
            .phase = 0
        },
        /* Low band settings */
        .lowBand = {
            .enabled = 1,
            .lowFreq = 70,
            .highFreq = 450,
            .gain = 1.5f,                    /* +1.5dB for warmth */
            .phase = 0
        },
        /* Mid band settings */
        .midBand = {
            .enabled = 1,
            .lowFreq = 450,
            .highFreq = 5000,                /* Extended mids for jazz */
            .gain = 0.5f,                    /* Subtle mid boost */
            .phase = 0
        },
        /* High band settings */
        .highBand = {
            .enabled = 1,
            .lowFreq = 5000,                 /* Higher crossover point */
            .gain = 0.0f,                    /* Flat high response */
            .phase = 0
        }
    },
    
    /* Compressor Settings - gentle for jazz */
    .compressor = {
        .enabled = 1,
        .threshold = -18.0f,
        .ratio = 1.5f,                       /* Very gentle compression */
        .attack = 25.0f,                     /* Slower attack to preserve dynamics */
        .release = 250.0f,                   /* Longer release */
        .makeupGain = 0.5f,
        .kneeWidth = 8.0f                    /* Very soft knee */
    },
    
    /* Limiter Settings */
    .limiter = {
        .enabled = 1,
        .threshold = -1.0f,
        .release = 60.0f,                    /* Slower limiter release */
        .lookaheadMs = 5.0f
    },
    
    /* Delay Settings */
    .delay = {
        .enabled = 0,
        .subBandDelay = 0,
        .lowBandDelay = 0,
        .midBandDelay = 0,
        .highBandDelay = 0
    }
};

/* Dangdut preset - emphasized mids, punchy bass */
static const SystemSettings_t dangdutPreset = {
    /* Crossover Settings */
    .crossover = {
        .enabled = 1,
        .filterType = FILTER_LINKWITZ_RILEY,
        .order = 4,
        /* Sub band settings */
        .subBand = {
            .enabled = 1,
            .highFreq = 100,                 /* Higher sub crossover for punchier bass */
            .gain = 3.5f,                    /* +3.5dB strong sub boost */
            .phase = 0
        },
        /* Low band settings */
        .lowBand = {
            .enabled = 1,
            .lowFreq = 100,
            .highFreq = 400,
            .gain = 1.0f,                    /* Slight low boost */
            .phase = 0
        },
        /* Mid band settings */
        .midBand = {
            .enabled = 1,
            .lowFreq = 400,
            .highFreq = 2800,
            .gain = 2.5f,                    /* +2.5dB mid boost for vocal presence */
            .phase = 0
        },
        /* High band settings */
        .highBand = {
            .enabled = 1,
            .lowFreq = 2800,
            .gain = 2.0f,                    /* +2dB high boost for tambourine/percussion */
            .phase = 0
        }
    },
    
    /* Compressor Settings - stronger for punchy sound */
    .compressor = {
        .enabled = 1,
        .threshold = -22.0f,
        .ratio = 3.5f,                       /* Stronger compression */
        .attack = 10.0f,                     /* Fast attack */
        .release = 120.0f,                   /* Medium release */
        .makeupGain = 2.0f,                  /* More makeup gain */
        .kneeWidth = 3.0f                    /* Harder knee */
    },
    
    /* Limiter Settings */
    .limiter = {
        .enabled = 1,
        .threshold = -0.5f,                  /* Lower threshold */
        .release = 40.0f,                    /* Fast release */
        .lookaheadMs = 5.0f
    },
    
    /* Delay Settings */
    .delay = {
        .enabled = 0,
        .subBandDelay = 0,
        .lowBandDelay = 0,
        .midBandDelay = 0,
        .highBandDelay = 0
    }
};

/* Pop preset - balanced with slight low and high boost */
static const SystemSettings_t popPreset = {
    /* Crossover Settings */
    .crossover = {
        .enabled = 1,
        .filterType = FILTER_LINKWITZ_RILEY,
        .order = 4,
        /* Sub band settings */
        .subBand = {
            .enabled = 1,
            .highFreq = 85,
            .gain = 2.0f,                    /* +2dB sub boost */
            .phase = 0
        },
        /* Low band settings */
        .lowBand = {
            .enabled = 1,
            .lowFreq = 85,
            .highFreq = 450,
            .gain = 1.0f,                    /* Slight low boost */
            .phase = 0
        },
        /* Mid band settings */
        .midBand = {
            .enabled = 1,
            .lowFreq = 450,
            .highFreq = 3800,
            .gain = 0.0f,                    /* Flat mids */
            .phase = 0
        },
        /* High band settings */
        .highBand = {
            .enabled = 1,
            .lowFreq = 3800,
            .gain = 1.5f,                    /* +1.5dB high boost for clarity */
            .phase = 0
        }
    },
    
    /* Compressor Settings - modern pop compression */
    .compressor = {
        .enabled = 1,
        .threshold = -18.0f,
        .ratio = 2.5f,                       /* Medium compression */
        .attack = 15.0f,                     /* Medium attack */
        .release = 180.0f,                   /* Medium release */
        .makeupGain = 1.0f,
        .kneeWidth = 5.0f
    },
    
    /* Limiter Settings */
    .limiter = {
        .enabled = 1,
        .threshold = -0.5f,
        .release = 50.0f,
        .lookaheadMs = 5.0f
    },
    
    /* Delay Settings */
    .delay = {
        .enabled = 0,
        .subBandDelay = 0,
        .lowBandDelay = 0,
        .midBandDelay = 0,
        .highBandDelay = 0
    }
};

/* Array of preset pointers for easy access */
static const SystemSettings_t* factoryPresets[NUM_FACTORY_PRESETS] = {
    &defaultPreset,
    &rockPreset,
    &jazzPreset,
    &dangdutPreset,
    &popPreset
};

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief Initialize the factory presets subsystem
  * @retval None
  */
void FactoryPresets_Init(void)
{
    /* Nothing to initialize for now */
}

/**
  * @brief Get a factory preset by index
  * @param presetIndex The preset index to retrieve (0-4)
  * @param settings Pointer to SystemSettings_t structure to fill with preset data
  * @retval 0 if successful, non-zero on error
  */
uint8_t FactoryPresets_GetPreset(uint8_t presetIndex, SystemSettings_t* settings)
{
    /* Check parameters */
    if (settings == NULL) {
        return 1;  /* Invalid parameter */
    }
    
    /* Check preset index range */
    if (presetIndex >= NUM_FACTORY_PRESETS) {
        return 2;  /* Invalid preset index */
    }
    
    /* Copy preset data to the output structure */
    memcpy(settings, factoryPresets[presetIndex], sizeof(SystemSettings_t));
    
    return 0;  /* Success */
}

/**
  * @brief Get the name of a factory preset
  * @param presetIndex The preset index (0-4)
  * @return Pointer to preset name string
  */
const char* FactoryPresets_GetPresetName(uint8_t presetIndex)
{
    /* Check preset index range */
    if (presetIndex >= NUM_FACTORY_PRESETS) {
        return "Unknown Preset";  /* Invalid preset index */
    }
    
    return presetNames[presetIndex];
}

/************************ (C) COPYRIGHT Audio Crossover Project *****END OF FILE****/
