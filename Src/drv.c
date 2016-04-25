/**
 *  Optical Driver for Visible Light Communication Project
 *  drv.c
 *  Copyright (c) 2016 Fadhli Dzil Ikram
 *
 */

/* Self header includes */
#include <drv.h>

/* Global variables */
DRV_HandleTypeDef DRV;
//uint8_t Buffer[1024];
//uint16_t BufferLen;

static uint8_t DRV_TX_StartBit[] = {0x55,0x55,0x55,0x55,0x55,0x55, 0x55, 0x55,
                                    0x55,0x55, 0x55, 0x55,0x9A,0xD7,0x65,0x28};

void DRV_RX_SetStatus(DRV_RX_StatusTypeDef Status);
void DRV_TX_SetStatus(DRV_TX_StatusTypeDef Status);

/* Function declaration */
void DRV_Init(void) {
  // Initiate receiver
  DRV.RX.htim = &htim2;
  DRV_RX_SetStatus(DRV_RX_STATUS_RESET);
  DRV.RX.SR.RLL = 1;
  // Initiate transmitter
  DRV.TX.htim = &htim3;
  DRV_TX_SetStatus(DRV_TX_STATUS_RESET);
  DRV.TX.SR.RLL = 1;
}

void DRV_RX_Start(void) {
  DRV_RX_SetStatus(DRV_RX_STATUS_IDLE);
}

void DRV_RX_Stop(void) {
  DRV_RX_SetStatus(DRV_RX_STATUS_RESET);
}

void DRV_RX_WriterReset(void);
inline void DRV_RX_WriterReset(void) {
  if (DRV.RX.SR.RLL) {
    DRV.RX.DataCount = DRV_RX_DATA_COUNT;
  } else {
    DRV.RX.DataCount = DRV_RX_DATA_COUNT;
  }
}

void DRV_RX_Writer(void) {
  uint8_t Data;

  if (DRV.RX.Status == DRV_RX_STATUS_WAIT) {
    // Fetch new bit
    DRV.RX.DataBit = (DRV.RX.DataBit << 1) | __GPIO_READ(GPIOA, 1);
    // Check for TDP symbols
    if (DRV.RX.DataBit == DRV_RX_DATA_TDP) {
      DRV_RX_SetStatus(DRV_RX_STATUS_ACTIVE);
    }
    // Check for TDP timeout
    if (!--DRV.RX.WaitCount) {
      DRV_RX_SetStatus(DRV_RX_STATUS_IDLE);
    }
  } else if (DRV.RX.Status == DRV_RX_STATUS_ACTIVE) {
    // Check for interleaving and fetch data
    //if (!DRV.RX.SR.RLL || (DRV.RX.DataCount & 1)) {
      DRV.RX.DataBit = (DRV.RX.DataBit << 1) | __GPIO_READ(GPIOA, 1);
    //}
    if (!--DRV.RX.DataCount) {
      DRV_RX_WriterReset();
      Data = DRV.RX.DataBit & 0xff;
      if (PHY_RX_Write(Data) || Data == 0xff) {
        // End receiving message
        DRV_RX_SetStatus(DRV_RX_STATUS_RESET);
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
      // Period averaging
      DRV.RX.Period += NewPeriod;
      DRV.RX.Period = (DRV.RX.Period >> 1);
      // Check for sync counter
      if (!--DRV.RX.SyncCount) {
        // Change state to TDP wait
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
    HAL_TIM_Base_Stop(DRV.RX.htim);
  } else if (Status == DRV_RX_STATUS_IDLE) {
    // Idle state, timer in low speed mode
    // Stop output compare interrupt
    HAL_TIM_OC_Stop_IT(DRV.RX.htim, TIM_CHANNEL_1);
    // Set the PSC, ARR, and CNT value
    TIM->ARR = 0xFFFFFFFF;
    TIM->PSC = 2000;
    // Reset the timer
    TIM->EGR |= TIM_EGR_UG;
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
    TIM->PSC = 0;
    // Reset the timer
    TIM->EGR |= TIM_EGR_UG;
    // Reset the sync state
    DRV.RX.IdleCount = DRV_RX_IDLE_COUNT;
    DRV.RX.SyncCount = DRV_RX_SYNC_COUNT;
  } else if (Status == DRV_RX_STATUS_WAIT &&
             DRV.RX.Status == DRV_RX_STATUS_SYNC) {
    // Reset the data counter
    DRV.RX.WaitCount = DRV_RX_WAIT_COUNT;
    // Set the output compare values
    TIM->ARR = DRV.RX.Period >> 1;
    TIM->CCR1 = DRV.RX.Period >> 3;
    // Reset the timer
    TIM->EGR |= TIM_EGR_UG;
    // Start output compare interrupt
    HAL_TIM_OC_Start_IT(DRV.RX.htim, TIM_CHANNEL_1);
    //HAL_TIM_IC_Stop_IT(DRV.RX.htim, TIM_CHANNEL_2);
  } else if (Status == DRV_RX_STATUS_ACTIVE &&
      DRV.RX.Status == DRV_RX_STATUS_WAIT) {
    // Only change status to active on sync
    // Reset the data counter
    DRV_RX_WriterReset();
  } else {
    // Illegal status set
    return;
  }
  // Set the appropriate status
  DRV.RX.Status = Status;
}

/* Transmission functions */
void DRV_TX_Send(uint8_t *Data, uint32_t DataLen) {
  DRV.TX.Data = Data;

  if (DRV.TX.SR.RLL) {
    DRV.TX.DataLen = DataLen << 4;
  } else {
    DRV.TX.DataLen = DataLen << 3;
  }

  DRV.TX.Start = DRV_TX_StartBit;
  DRV.TX.StartLen = sizeof(DRV_TX_StartBit) << 3;
  DRV_TX_SetStatus(DRV_TX_STATUS_ACTIVE);
}

void DRV_TX_SetStatus(DRV_TX_StatusTypeDef Status) {
  //TIM_TypeDef *TIM = DRV.TX.htim->Instance;

  if (Status == DRV_TX_STATUS_RESET) {
    HAL_TIM_Base_Stop_IT(DRV.TX.htim);
  } else if (Status == DRV_TX_STATUS_ACTIVE) {
    //TIM4->CCR1 = 1105;
    // Reset the timer
    TIM4->EGR |= TIM_EGR_UG;
    HAL_TIM_Base_Start_IT(DRV.TX.htim);
  } else {
    // Invalid status set
    return;
  }

  DRV.TX.Status = Status;
}

/* Interrupt callback */
void DRV_RX_TimerICCallback(void) {
  TIM_TypeDef *TIM = DRV.RX.htim->Instance;
  //uint32_t DiffTime;
  
  if (DRV.RX.Status == DRV_RX_STATUS_IDLE ||
      DRV.RX.Status == DRV_RX_STATUS_SYNC) {
    DRV_RX_Synchronizer();
  } else if (DRV.RX.Status == DRV_RX_STATUS_WAIT ||
      DRV.RX.Status == DRV_RX_STATUS_ACTIVE) {
    //DiffTime = TIM->CCR2 >> 4;
    //if (DiffTime == 0 || DiffTime == (TIM->ARR >> 4)) {
      TIM->EGR |= TIM_EGR_UG;
    //}
	}
}

void DRV_RX_TimerOCCallback(void) {
  if (DRV.RX.Status == DRV_RX_STATUS_WAIT ||
      DRV.RX.Status == DRV_RX_STATUS_ACTIVE) {
    DRV_RX_Writer();
  }
}

void DRV_TX_TimerOverflowCallback(void) {
	uint8_t BitPos;
  uint8_t TxBit;

	// Checks for data length in bits
  if (DRV.TX.StartLen) {
    DRV.TX.StartLen--;
    BitPos = DRV.TX.StartLen & 7;
    TxBit = (*DRV.TX.Start >> BitPos) & 1;
    if (!BitPos) {
      DRV.TX.Start++;
    }
  } else if (DRV.TX.DataLen) {
    DRV.TX.DataLen--;
    if (DRV.TX.SR.RLL) {
      BitPos = DRV.TX.DataLen & 15;
      TxBit = (*DRV.TX.Data >> (BitPos >> 1)) & 1;
      if (!(DRV.TX.DataLen & 1)) {
        TxBit ^= 1;
      }
    } else {
      BitPos = DRV.TX.DataLen & 7;
      TxBit = (*DRV.TX.Data >> BitPos) & 1;
    }
    if (!BitPos) {
      DRV.TX.Data++;
    }
	} else {
		DRV_TX_SetStatus(DRV_TX_STATUS_RESET);
    TxBit = 1;
	}

  if (TxBit) {
    TIM4->CCR1 = 2210;
    __GPIO_WRITE(GPIOA, 9, GPIO_PIN_SET);
  } else {
    TIM4->CCR1 = 1105;
    __GPIO_WRITE(GPIOA, 9, GPIO_PIN_RESET);
  }

}
