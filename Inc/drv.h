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
#include "phy.h"
#include "string.h"

#define DRV_RX_IDLE_COUNT 4
#define DRV_RX_SYNC_COUNT 8
#define DRV_RX_WAIT_COUNT 200
#define DRV_RX_DATA_COUNT 8
#define DRV_RX_DATA_TDP 0x9AD76528

#define DRV_TX_PRELOAD_COUNT 8

// Macro for absolute delta
#define __delta(a, b) ((a > b) ? (a - b):(b - a))

// Receive Driver Status TypeDef
typedef enum {
  DRV_RX_STATUS_RESET,
  DRV_RX_STATUS_IDLE,
  DRV_RX_STATUS_SYNC,
  DRV_RX_STATUS_WAIT,
  DRV_RX_STATUS_ACTIVE,
  DRV_RX_STATUS_BUSY
} DRV_RX_StatusTypeDef;

typedef enum {
  DRV_TX_STATUS_RESET,
  DRV_TX_STATUS_VISIBILITY,
  DRV_TX_STATUS_SYNC,
  DRV_TX_STATUS_ACTIVE,
  DRV_TX_STATUS_STOP,
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
  struct {
    uint8_t RLL: 1;
  } SR;
} DRV_RX_HandleTypeDef;

typedef struct {
  TIM_HandleTypeDef *htim;
  uint8_t PreloadData;
  uint8_t PreloadDataLen;
  uint8_t *Data;
  uint8_t *Send;
  uint16_t DataLen;
  uint16_t SendLen;
  DRV_TX_StatusTypeDef Status;
  struct {
    uint8_t RLL: 1;
    uint8_t Visibility: 1;
  } SR;
} DRV_TX_HandleTypeDef;

typedef struct {
  DRV_RX_HandleTypeDef RX;
  DRV_TX_HandleTypeDef TX;
} DRV_HandleTypeDef;

/* Static handler */
//extern DRV_HandleTypeDef DRV;

/* Function prototypes */
void DRV_Init(void);

void DRV_RX_Start(void);
void DRV_RX_Stop(void);
void DRV_RX_WriteReset(void);
void DRV_RX_ActiveHandler(void);
void DRV_RX_WaitHandler(void);
void DRV_RX_SyncHandler(void);
void DRV_RX_IdleHandler(void);
void DRV_RX_SetStatus(DRV_RX_StatusTypeDef Status);

DRV_TX_StatusTypeDef DRV_TX_GetStatus(void);
void DRV_TX_SetData(void);
void DRV_TX_Preload(void);
void DRV_TX_SetStatus(DRV_TX_StatusTypeDef Status);

/* Endpoints for API Calls */
uint8_t DRV_API_SendStart(uint8_t *Data, uint16_t Length);
void DRV_API_ReceiveComplete(void);
// Specific APIs for timer interrupts
void DRV_API_InputCaptureCallback(TIM_HandleTypeDef *htim);
void DRV_API_OutputCompareCallback(TIM_HandleTypeDef *htim);
void DRV_API_UpdateEventCallback(TIM_HandleTypeDef *htim);

#endif //__DRV
