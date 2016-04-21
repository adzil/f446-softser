/**
 *  Optical Driver for Visible Light Communication Project
 *  drv.c
 *  Copyright (c) 2016 Fadhli Dzil Ikram
 *
 */

/* Self header includes */
#include <stm32f446xx.h>
#include <drv.h>
#include <stm32f4xx_hal_tim.h>
#include "drv.h"

/* Global variables */
DRV_HandleTypeDef hDrv;
DRV_RX_HandleTypeDef hRXDrv;
DRV_TX_HandleTypeDef hTXDrv;

/* Private function prototypes */
void DRV_RX_WaitReset(DRV_HandleTypeDef *Handle);
void DRV_RX_WaitTrigger(DRV_HandleTypeDef *Handle);

void DRV_RX_SetStatus(DRV_HandleTypeDef *Handle, DRV_RX_StatusTypeDef Status);

/* Function declaration */
void DRV_Init(void) {
  // Initiate handler
  hDrv.RX = &hRXDrv;
  hDrv.TX = &hTXDrv;
  // Initiate receiver
  hDrv.RX->htim = &htim2;
  hDrv.RX->TIM = hDrv.RX->htim->Instance;
  hDrv.RX->Status = DRV_RX_STATUS_RESET;
}

inline void DRV_RX_WaitReset(DRV_HandleTypeDef *Handle) {
  Handle->RX->WaitCount = DRV_RX_WAIT_COUNT;
}

void DRV_RX_WaitTrigger(DRV_HandleTypeDef *Handle) {
  uint32_t NewPeriod;

  NewPeriod = Handle->RX->TIM->CCR2 - Handle->RX->ICValue;
  Handle->RX->ICValue = Handle->RX->TIM->CCR2;

  if (__delta(NewPeriod, Handle->RX->Period) < 4) {
    if (!--Handle->RX->WaitCount) {
      DRV_RX_SetStatus(Handle, DRV_RX_STATUS_SYNC);
    }
  } else {
    DRV_RX_WaitReset(Handle);
    Handle->RX->WaitCount = NewPeriod;
  }
}

void DRV_RX_SetStatus(DRV_HandleTypeDef *Handle, DRV_RX_StatusTypeDef Status) {
  if (Status == DRV_RX_STATUS_RESET) {
    // Reset state, disable all RX Timers
    HAL_TIM_OC_Stop_IT(Handle->RX->htim, TIM_CHANNEL_1);
    HAL_TIM_IC_Stop_IT(Handle->RX->htim, TIM_CHANNEL_2);
    HAL_TIM_OC_Stop_IT(Handle->RX->htim, TIM_CHANNEL_3);
    HAL_TIM_OC_Stop_IT(Handle->RX->htim, TIM_CHANNEL_4);
    HAL_TIM_Base_Stop(Handle->RX->htim);
  } else if (Status == DRV_RX_STATUS_WAIT) {
    // Wait state, set timer in free mode
    // Stop output compare interrupt
    HAL_TIM_OC_Stop_IT(Handle->RX->htim, TIM_CHANNEL_1);
    HAL_TIM_OC_Stop_IT(Handle->RX->htim, TIM_CHANNEL_3);
    HAL_TIM_OC_Stop_IT(Handle->RX->htim, TIM_CHANNEL_4);
    // Realign ARR and CNT timer register
    Handle->RX->TIM->ARR = 0xFFFFFFFF;
    Handle->RX->TIM->CNT = 0;
    // Reset the wait state
    DRV_RX_WaitReset(Handle);
    // Start base timer and input capture
    HAL_TIM_Base_Start(Handle->RX->htim);
    HAL_TIM_IC_Start_IT(Handle->RX->htim, TIM_CHANNEL_2);
  } else if (Status == DRV_RX_STATUS_SYNC &&
             (Handle->RX->Status == DRV_RX_STATUS_WAIT ||
              Handle->RX->Status == DRV_RX_STATUS_ACTIVE)) {
    // Only change status to sync on wait or active
    Handle->RX->TIM->ARR = Handle->RX->WaitCount >> 1;
    Handle->RX->TIM->CCR1 = Handle->RX->WaitCount >> 3;
    Handle->RX->TIM->CCR3 = Handle->RX->WaitCount >> 2;
    Handle->RX->TIM->CCR4 = (Handle->RX->WaitCount >> 2) +
                   (Handle->RX->WaitCount >> 3);
    Handle->RX->TIM->CNT = 0;
    // Start output compare interrupt
    HAL_TIM_OC_Start_IT(Handle->RX->htim, TIM_CHANNEL_1);
    HAL_TIM_OC_Start_IT(Handle->RX->htim, TIM_CHANNEL_3);
    HAL_TIM_OC_Start_IT(Handle->RX->htim, TIM_CHANNEL_4);
  } else if (Status == DRV_RX_STATUS_ACTIVE &&
             Handle->RX->Status == DRV_RX_STATUS_SYNC) {
    // Only change status to active on sync
    (void) 0;
  } else {
    // Illegal status set
    return;
  }

  // Set the appropriate status
  Handle->RX->Status = Status;
}