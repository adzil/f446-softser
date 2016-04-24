/**
 *  Optical Driver for Visible Light Communication Project
 *  drv.c
 *  Copyright (c) 2016 Fadhli Dzil Ikram
 *
 */

/* Self header includes */
#include <drv.h>
#include <stm32f446xx.h>

/* Constants */
const uint8_t popcnt3 = 0xE8;

/* Global variables */
DRV_HandleTypeDef DRV;
uint8_t printval;
extern osThreadId tid_sendSerial;

/* Private function prototypes */
//void DRV_RX_WaitTrigger(void);
//void DRV_RX_SampleTrigger(void);
//void DRV_RX_DataWrite(void);

void DRV_RX_SetStatus(DRV_RX_StatusTypeDef Status);
void DRV_TX_SetStatus(DRV_TX_StatusTypeDef Status);

/* Function declaration */
void DRV_Init(void) {
  // Initiate receiver
  DRV.RX.htim = &htim2;
  DRV_RX_SetStatus(DRV_RX_STATUS_RESET);
  // Initiate transmitter
  DRV.TX.htim = &htim3;
  DRV_TX_SetStatus(DRV_TX_STATUS_RESET);
}

void DRV_RX_Start(void) {
  DRV_RX_SetStatus(DRV_RX_STATUS_IDLE);
}

void DRV_RX_Stop(void) {
  DRV_RX_SetStatus(DRV_RX_STATUS_RESET);
}

void DRV_RX_Writer(void) {
  uint8_t NewData;

  NewData = (popcnt3 >> (DRV.RX.SampleBit & 7)) & 1;
  DRV.RX.DataBit = (DRV.RX.DataBit << 1) | NewData;

  if (DRV.RX.Status == DRV_RX_STATUS_WAIT) {
    // Check for TDP symbols
    if (DRV.RX.DataBit == DRV_RX_DATA_TDP) {
      DRV_RX_SetStatus(DRV_RX_STATUS_ACTIVE);
    }
    // Check for TDP timeout
    if (!--DRV.RX.WaitCount) {
      DRV_RX_SetStatus(DRV_RX_STATUS_IDLE);
    }
  } else if (DRV.RX.Status == DRV_RX_STATUS_ACTIVE) {
    if (!--DRV.RX.DataCount) {
      DRV.RX.DataCount = DRV_RX_DATA_COUNT;
      printval = DRV.RX.DataBit & 0xff;
      if (printval != 0xff) {
        osSignalSet(tid_sendSerial, 1);
      } else {
        DRV_RX_SetStatus(DRV_RX_STATUS_IDLE);
      }
    }
  }
}

void DRV_RX_Synchronizer(void) {
  TIM_TypeDef *TIM = DRV.RX.htim->Instance;
  uint32_t NewPeriod;

  NewPeriod = TIM->CCR2 - DRV.RX.ICValue;
  DRV.RX.ICValue = TIM->CCR2;

  if (DRV.RX.Status == DRV_RX_STATUS_SYNC) {
    if (__delta(NewPeriod, DRV.RX.Period) < 16) {
      if (!--DRV.RX.SyncCount) {
        DRV_RX_SetStatus(DRV_RX_STATUS_WAIT);
      }
    } else {
      DRV.RX.Period = NewPeriod;
      DRV.RX.SyncCount = DRV_RX_SYNC_COUNT;
      if (!--DRV.RX.IdleCount) {
        DRV_RX_SetStatus(DRV_RX_STATUS_IDLE);
      }
    }
  } else if (DRV.RX.Status == DRV_RX_STATUS_IDLE) {
    if (!__delta(NewPeriod, DRV.RX.Period)) {
      if (!--DRV.RX.IdleCount) {
				DRV_RX_SetStatus(DRV_RX_STATUS_SYNC);
      }
    } else {
      DRV.RX.Period = NewPeriod;
      DRV.RX.IdleCount = DRV_RX_IDLE_COUNT;
    }
  }
}

void DRV_RX_SetStatus(DRV_RX_StatusTypeDef Status) {
  TIM_TypeDef *TIM = DRV.RX.htim->Instance;

  if (Status == DRV_RX_STATUS_RESET) {
    // Reset state, disable all RX Timers
    HAL_TIM_OC_Stop_IT(DRV.RX.htim, TIM_CHANNEL_1);
    HAL_TIM_IC_Stop_IT(DRV.RX.htim, TIM_CHANNEL_2);
    HAL_TIM_OC_Stop_IT(DRV.RX.htim, TIM_CHANNEL_3);
    HAL_TIM_OC_Stop_IT(DRV.RX.htim, TIM_CHANNEL_4);
    HAL_TIM_Base_Stop(DRV.RX.htim);
  } else if (Status == DRV_RX_STATUS_IDLE) {
    // Idle state, timer in low speed mode
    // Stop output compare interrupt
    HAL_TIM_OC_Stop_IT(DRV.RX.htim, TIM_CHANNEL_1);
    HAL_TIM_OC_Stop_IT(DRV.RX.htim, TIM_CHANNEL_3);
    HAL_TIM_OC_Stop_IT(DRV.RX.htim, TIM_CHANNEL_4);
    // Set the PSC, ARR, and CNT value
    TIM->ARR = 0xFF;
    TIM->CNT = 0;
    TIM->CNT = 0xFF;
    TIM->PSC = 2000;
    // Reset the idle state
    DRV.RX.IdleCount = DRV_RX_IDLE_COUNT;
    // Start base timer and input capture
    HAL_TIM_Base_Start(DRV.RX.htim);
    HAL_TIM_IC_Start_IT(DRV.RX.htim, TIM_CHANNEL_2);
  } else if (Status == DRV_RX_STATUS_SYNC &&
      DRV.RX.Status == DRV_RX_STATUS_IDLE) {
    // Only switch to sync status when idle
    // Reset the PSC, ARR, and CNT value
    TIM->ARR = 0xFFFFFFFF;
    TIM->CNT = 0;
    TIM->CNT = 0xFFFFFFFF;
    TIM->PSC = 0;
    // Reset the sync state
    DRV.RX.IdleCount = DRV_RX_IDLE_COUNT;
    DRV.RX.SyncCount = DRV_RX_SYNC_COUNT;
  } else if (Status == DRV_RX_STATUS_WAIT &&
             DRV.RX.Status == DRV_RX_STATUS_SYNC) {
    // Reset the data counter
    DRV.RX.SampleLock = 0;
    DRV.RX.WaitCount = DRV_RX_WAIT_COUNT;
    // Set the output compare values
    TIM->ARR = DRV.RX.Period >> 1;
    TIM->CCR1 = DRV.RX.Period >> 3;
    TIM->CCR3 = DRV.RX.Period >> 2;
    TIM->CCR4 = (DRV.RX.Period >> 2) + (DRV.RX.Period >> 3);
    TIM->CNT = 0;
    // Start output compare interrupt
    HAL_TIM_OC_Start_IT(DRV.RX.htim, TIM_CHANNEL_1);
    HAL_TIM_IC_Stop_IT(DRV.RX.htim, TIM_CHANNEL_2);
    HAL_TIM_OC_Start_IT(DRV.RX.htim, TIM_CHANNEL_3);
    HAL_TIM_OC_Start_IT(DRV.RX.htim, TIM_CHANNEL_4);
  } else if (Status == DRV_RX_STATUS_ACTIVE &&
      DRV.RX.Status == DRV_RX_STATUS_WAIT) {
    // Only change status to active on sync
    // Reset the data counter
    DRV.RX.DataCount = DRV_RX_DATA_COUNT;
  } else {
    // Illegal status set
    return;
  }

  // Set the appropriate status
  DRV.RX.Status = Status;
}

/* Transmission functions */
void DRV_TX_Load(void) {
  uint8_t BitPos;

  // Checks for data length in bits
  if (DRV.TX.DataLen--) {
    BitPos = DRV.TX.DataLen & 7;
    DRV.TX.SendBit = (*DRV.TX.Data >> BitPos) & 1;
    if (!BitPos) DRV.TX.Data++;
  } else {
    // End transmission
    DRV.TX.SendBit = 0xff;
  }
}

void DRV_TX_Send(uint8_t *Data, uint32_t DataLen) {
  DRV.TX.Data = Data;
  DRV.TX.DataLen = DataLen << 3;

  DRV_TX_SetStatus(DRV_TX_STATUS_ACTIVE);
}

void DRV_TX_SetStatus(DRV_TX_StatusTypeDef Status) {
  TIM_TypeDef *TIM = DRV.TX.htim->Instance;

  if (Status == DRV_TX_STATUS_RESET) {
    HAL_TIM_Base_Stop_IT(DRV.TX.htim);
  } else if (Status == DRV_TX_STATUS_ACTIVE) {
    DRV_TX_Load();
    TIM->CNT = 0;
    HAL_TIM_Base_Start_IT(DRV.TX.htim);
  } else {
    // Invalid status set
    return;
  }

  DRV.TX.Status = Status;
}

/* Interrupt callback */
void DRV_RX_TimerICCallback(void) {
  if (DRV.RX.Status == DRV_RX_STATUS_IDLE ||
      DRV.RX.Status == DRV_RX_STATUS_SYNC) {
    DRV_RX_Synchronizer();
  } else if (DRV.RX.Status == DRV_RX_STATUS_WAIT ||
      DRV.RX.Status == DRV_RX_STATUS_ACTIVE) {
    /*if (!DRV.RX.SampleLock) {
			DRV.RX.htim->Instance->CNT = 0;
		}*/
  }
}

void DRV_RX_TimerOCCallback(void) {
  if (DRV.RX.Status == DRV_RX_STATUS_WAIT ||
      DRV.RX.Status == DRV_RX_STATUS_ACTIVE) {
    DRV.RX.SampleBit = (DRV.RX.SampleBit << 1) | __GPIO_READ(GPIOA, 1);
		if (DRV.RX.htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
			DRV.RX.SampleLock = 1;
    } else if (DRV.RX.htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4) {
			DRV.RX.SampleLock = 0;
      // Write the sample to data
      DRV_RX_Writer();
    }
  }
}

void DRV_TX_TimerOverflowCallback(void) {
  if (DRV.TX.Status == DRV_TX_STATUS_ACTIVE) {
    if (DRV.TX.SendBit != 0xff) {
      __GPIO_WRITE(GPIOA, 9, DRV.TX.SendBit);
      DRV_TX_Load();
    } else {
      __GPIO_WRITE(GPIOA, 9, GPIO_PIN_SET);
      DRV_TX_SetStatus(DRV_TX_STATUS_RESET);
    }
  }
}
