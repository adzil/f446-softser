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
#define DRV_RX_DATA_COUNT 32
#define DRV_RX_DATA_TDP 0x9AD76528

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
  uint8_t SampleBit;
  uint8_t SampleLock;
  uint32_t DataBit;
  uint8_t DataCount;
  uint8_t SampleValue;
  TIM_HandleTypeDef *htim;
  TIM_TypeDef *TIM;
} DRV_RX_HandleTypeDef;

typedef struct {
  uint8_t DUMMY;
} DRV_TX_HandleTypeDef;

typedef struct {
  DRV_RX_HandleTypeDef *RX;
  DRV_TX_HandleTypeDef *TX;
} DRV_HandleTypeDef;

extern DRV_HandleTypeDef hDrv;

/* Public function prototypes */
void DRV_Init(void);
void DRV_RX_Start(void);
void DRV_RX_Stop(void);

/* Interrupt handler function */
void DRV_RX_TimerICCallback(DRV_HandleTypeDef *Handle);
void DRV_RX_TimerOCCallback(DRV_HandleTypeDef *Handle);
void DRV_RX_SampleCallback(DRV_HandleTypeDef *Handle);

/* Inline functions */
inline void DRV_RX_SampleCallback(DRV_HandleTypeDef *Handle) {
  Handle->RX->SampleValue = __GPIO_READ(GPIOA, 1);
}

#endif //__DRV
