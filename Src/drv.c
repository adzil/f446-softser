/**
 *  Optical Driver for Visible Light Communication Project
 *  drv.c
 *  Copyright (c) 2016 Fadhli Dzil Ikram
 *
 */

/* Self header includes */
#include <drv.h>
#include "drv.h"

/* Constants */
const uint8_t popcnt3 = 0xE8;

/* Global variables */
DRV_HandleTypeDef hDrv;
DRV_RX_HandleTypeDef hRXDrv;
DRV_TX_HandleTypeDef hTXDrv;

/* Private function prototypes */
void DRV_RX_WaitReset(DRV_HandleTypeDef *Handle);
void DRV_RX_WaitTrigger(DRV_HandleTypeDef *Handle);
void DRV_RX_SampleReset(DRV_HandleTypeDef *Handle);
void DRV_RX_SampleTrigger(DRV_HandleTypeDef *Handle);
void DRV_RX_DataReset(DRV_HandleTypeDef *Handle);
void DRV_RX_DataWrite(DRV_HandleTypeDef *Handle);

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

void DRV_RX_Start(void) {
  DRV_RX_SetStatus(&hDrv, DRV_RX_STATUS_WAIT);
}

void DRV_RX_Stop(void) {
  DRV_RX_SetStatus(&hDrv, DRV_RX_STATUS_RESET);
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
    Handle->RX->Period = NewPeriod;
  }
}

inline void DRV_RX_DataReset(DRV_HandleTypeDef *Handle) {
  Handle->RX->DataCount = DRV_RX_DATA_COUNT;
}

void DRV_RX_DataWrite(DRV_HandleTypeDef *Handle) {
  uint8_t NewData;

  NewData = (popcnt3 >> (Handle->RX->SampleBit & 7)) & 1;
  Handle->RX->DataBit = (Handle->RX->DataBit << 1) | NewData;

  if (!--Handle->RX->DataCount) {
    DRV_RX_DataReset(Handle);
  }

  if (Handle->RX->Status == DRV_RX_STATUS_SYNC) {
    // Check for TDP symbols
    if (Handle->RX->DataBit == DRV_RX_DATA_TDP) {
      DRV_RX_SetStatus(Handle, DRV_RX_STATUS_ACTIVE);
    }
  }
}

inline void DRV_RX_SampleReset(DRV_HandleTypeDef *Handle) {
  if (!Handle->RX->SampleLock) {
    // Reset the timer counter
    Handle->RX->TIM->CNT = 0;
  }
}

inline void DRV_RX_SampleTrigger(DRV_HandleTypeDef *Handle) {
  Handle->RX->SampleBit = (Handle->RX->SampleBit << 1) |
      (Handle->RX->SampleValue);
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
    // Reset the data counter
    DRV_RX_DataReset(Handle);
  } else {
    // Illegal status set
    return;
  }

  // Set the appropriate status
  Handle->RX->Status = Status;
}

void DRV_RX_TimerICCallback(DRV_HandleTypeDef *Handle) {
  if (Handle->RX->Status == DRV_RX_STATUS_WAIT) {
    DRV_RX_WaitTrigger(Handle);
  } else if (Handle->RX->Status == DRV_RX_STATUS_SYNC ||
      Handle->RX->Status == DRV_RX_STATUS_ACTIVE) {
    DRV_RX_SampleReset(Handle);
  }
}

void DRV_RX_TimerOCCallback(DRV_HandleTypeDef *Handle) {
  if (Handle->RX->Status == DRV_RX_STATUS_SYNC ||
      Handle->RX->Status == DRV_RX_STATUS_ACTIVE) {
    DRV_RX_SampleTrigger(Handle);
    // Lock or unlock the sample
    if (Handle->RX->htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
      Handle->RX->SampleLock = 1;
    } else if (Handle->RX->htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4) {
      Handle->RX->SampleLock = 0;
      // Write the sample to data
      DRV_RX_DataWrite(Handle);
    }
  }
}
