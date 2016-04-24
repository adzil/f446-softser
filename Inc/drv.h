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
#include "cmsis_os.h"

#define DRV_RX_IDLE_COUNT 8
#define DRV_RX_SYNC_COUNT 8
#define DRV_RX_WAIT_COUNT 128
#define DRV_RX_DATA_COUNT 8
#define DRV_RX_DATA_TDP 0x9AD76528

// Macro for absolute delta
#define __delta(a, b) ((a > b) ? (a - b):(b - a))

// Receive Driver Status TypeDef
typedef enum {
  DRV_RX_STATUS_RESET,
  DRV_RX_STATUS_IDLE,
  DRV_RX_STATUS_SYNC,
  DRV_RX_STATUS_WAIT,
  DRV_RX_STATUS_ACTIVE
} DRV_RX_StatusTypeDef;

typedef enum {
  DRV_TX_STATUS_RESET,
  DRV_TX_STATUS_ACTIVE
} DRV_TX_StatusTypeDef;

typedef struct {
  DRV_RX_StatusTypeDef Status;
  uint32_t Period;
  uint32_t ICValue;
  uint32_t DataBit;
  TIM_HandleTypeDef *htim;
  uint8_t IdleCount;
  uint8_t SyncCount;
  uint8_t WaitCount;

  uint8_t DataCount;
  uint8_t SampleBit;
	uint8_t SampleLock;
} DRV_RX_HandleTypeDef;

typedef struct {
  TIM_HandleTypeDef *htim;
  DRV_TX_StatusTypeDef Status;
  uint32_t DataLen;
  uint8_t *Data;
  uint8_t SendBit;
} DRV_TX_HandleTypeDef;

typedef struct {
  DRV_RX_HandleTypeDef RX;
  DRV_TX_HandleTypeDef TX;
} DRV_HandleTypeDef;

extern DRV_HandleTypeDef DRV;

/* Public function prototypes */
void DRV_Init(void);
void DRV_RX_Start(void);
void DRV_RX_Stop(void);

void DRV_TX_Send(uint8_t *Data, uint32_t DataLen);

/* Interrupt handler function */
void DRV_RX_TimerICCallback(void);
void DRV_RX_TimerOCCallback(void);

void DRV_TX_TimerOverflowCallback(void);

/* Inline functions */
/* inline void DRV_RX_SampleCallback(void) {
  Handle->RX->SampleValue = __GPIO_READ(GPIOA, 1);
}*/

#endif //__DRV
