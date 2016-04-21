/**
 *  Optical Driver for Visible Light Communication Project
 *  drv.h
 *  Copyright (c) 2016 Fadhli Dzil Ikram
 *
 */

#ifndef __DRV
#define __DRV

#include <inttypes.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "tim.h"
#include "macros.h"

#define DRV_RX_WAIT_COUNT 15

// Macro for absolute delta
#define __delta(a, b) ((a > b) ? (a - b):(b - a))

// Receive Driver Status TypeDef
typedef enum {
  DRV_RX_STATUS_RESET,
  DRV_RX_STATUS_WAIT,
  DRV_RX_STATUS_SYNC,
  DRV_RX_STATUS_ACTIVE
} DRV_RX_StatusTypeDef;

typedef struct {
  DRV_RX_StatusTypeDef Status;
  uint32_t Period;
  uint32_t ICValue;
  uint8_t WaitCount;
  TIM_HandleTypeDef *htim;
  TIM_TypeDef *TIM;
} DRV_RX_HandleTypeDef;

typedef struct {

} DRV_TX_HandleTypeDef;

typedef struct {
  DRV_RX_HandleTypeDef *RX;
  DRV_TX_HandleTypeDef *TX;
} DRV_HandleTypeDef;

extern DRV_HandleTypeDef hDrv;

/* Public function prototypes */
void DRV_Init(void);

#endif //__DRV
