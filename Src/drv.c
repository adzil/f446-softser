/**
 *  Optical Driver for Visible Light Communication Project
 *  drv.c
 *  Copyright (c) 2016 Fadhli Dzil Ikram
 *
 */

/* Self header includes */
#include <stm32f446xx.h>
#include <drv.h>
#include "drv.h"

/* Global variables */
DRV_HandleTypeDef hDrv;

/* Function prototypes */
void DRV_RX_WaitReset(DRV_HandleTypeDef *Handle);
void DRV_RX_WaitTrigger(DRV_HandleTypeDef *Handle);

void DRV_RX_SetStatus(DRV_HandleTypeDef *Handle, DRV_RX_StatusTypeDef Status);

/* Function declaration */
void DRV_RX_SetStatus(DRV_HandleTypeDef *Handle, DRV_RX_StatusTypeDef Status) {
  if (Status == DRV_RX_STATUS_RESET) {
    // Reset state, disable all RX Timers
    HAL_TIM_OC_Stop_IT(&RX_HTIM, TIM_CHANNEL_1);
    HAL_TIM_IC_Stop_IT(&RX_HTIM, TIM_CHANNEL_2);
    HAL_TIM_OC_Stop_IT(&RX_HTIM, TIM_CHANNEL_3);
    HAL_TIM_OC_Stop_IT(&RX_HTIM, TIM_CHANNEL_4);
    HAL_TIM_Base_Stop(&RX_HTIM);
  } else if (Status == DRV_RX_STATUS_WAIT) {
    // Wait state, set timer in free mode
    // Stop output compare interrupt
    HAL_TIM_OC_Stop_IT(&RX_HTIM, TIM_CHANNEL_1);
    HAL_TIM_OC_Stop_IT(&RX_HTIM, TIM_CHANNEL_3);
    HAL_TIM_OC_Stop_IT(&RX_HTIM, TIM_CHANNEL_4);
    // Realign ARR and CNT timer register
    RX_TIM->ARR = 0xFFFFFFFF;
    RX_TIM->CNT = 0;
    // Reset the wait state
    DRV_RX_WaitReset(Handle);
    // Start base timer and input capture
    HAL_TIM_Base_Start(&RX_HTIM);
    HAL_TIM_IC_Start_IT(&RX_HTIM, TIM_CHANNEL_2);
  } else if (Status == DRV_RX_STATUS_SYNC &&
             (Handle->RX.Status == DRV_RX_STATUS_WAIT ||
              Handle->RX.Status == DRV_RX_STATUS_ACTIVE)) {
    // Only change status to sync on wait or active
    RX_TIM->ARR = Handle->RX.SyncPeriod >> 1;
    RX_TIM->CCR1 = Handle->RX.SyncPeriod >> 3;
    RX_TIM->CCR3 = Handle->RX.SyncPeriod >> 2;
    RX_TIM->CCR4 = (Handle->RX.SyncPeriod >> 2) +
                   (Handle->RX.SyncPeriod >> 3);
    RX_TIM->CNT = 0;
    // Start output compare interrupt
    HAL_TIM_OC_Start_IT(&RX_HTIM, TIM_CHANNEL_1);
    HAL_TIM_OC_Start_IT(&RX_HTIM, TIM_CHANNEL_3);
    HAL_TIM_OC_Start_IT(&RX_HTIM, TIM_CHANNEL_4);
  } else if (Status == DRV_RX_STATUS_ACTIVE &&
             Handle->RX.Status == DRV_RX_STATUS_SYNC) {
    // Only change status to active on sync
    (void) 0;
  } else {
    // Illegal status set
    return;
  }

  // Set the appropriate status
  Handle->RX.Status = Status;
}