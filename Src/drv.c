/**
 *  Optical Driver for Visible Light Communication Project
 *  drv.c
 *  Copyright (c) 2016 Fadhli Dzil Ikram
 *
 */

/* Self header includes */
#include <drv.h>

/* Global variables */
// Driver handle variable
DRV_HandleTypeDef DRV;

// Set Start Bit code (96bit FLP and 32bit TDP)
const uint8_t DRV_TX_StartBit[] = {0x55,0x55,0x55,0x55,0x55,0x55, 0x55, 0x55,
                                   0x55,0x55, 0x55, 0x55,0x9A,0xD7,0x65,0x28};
const uint8_t DRV_TX_Visibility[] = {0xE2, 0xD4, 0xB1, 0x39};
const uint8_t DRV_TX_Stop[] = {0x1};

/* Private function prototypes */
void DRV_RX_WriteReset(void);
void DRV_RX_SetStatus(DRV_RX_StatusTypeDef Status);
void DRV_TX_SetStatus(DRV_TX_StatusTypeDef Status);

/* Function declaration */
// Driver initialization code and Status Register settings
void DRV_Init(void) {
  // Initiate receiver
  DRV.RX.htim = &htim2;
  DRV_RX_SetStatus(DRV_RX_STATUS_RESET);
  DRV.RX.SR.RLL = 0;
  // Initiate transmitter
  DRV.TX.htim = &htim3;
  DRV_TX_SetStatus(DRV_TX_STATUS_RESET);
  DRV.TX.SR.RLL = 0;
  DRV.TX.SR.Visibility = 0;
  //DRV_TX_SetStatus(DRV_TX_STATUS_VISIBILITY);
}

// Start RX module
void DRV_RX_Start(void) {
  DRV_RX_SetStatus(DRV_RX_STATUS_IDLE);
}

// Stop RX module
void DRV_RX_Stop(void) {
  DRV_RX_SetStatus(DRV_RX_STATUS_RESET);
}

// Reset write state (typically when transitioning from WAIT to ACTIVE)
// and bit reset after data counter expires
inline void DRV_RX_WriteReset(void) {
  if (DRV.RX.SR.RLL) {
    // Multiply data count by 2 if using the RLL code
    DRV.RX.DataCount = DRV_RX_DATA_COUNT << 1;
  } else {
    // Otherwise, just receive the data as is
    DRV.RX.DataCount = DRV_RX_DATA_COUNT;
  }
}

// Write new data to the RX FIFO buffer and TDP detection
void DRV_RX_Write(void) {
  uint8_t Data;

  if (DRV.RX.Status == DRV_RX_STATUS_WAIT) {
    // System waits for TDP data to begin the actual data transmission
    // Fetch new bit
    DRV.RX.DataBit = (DRV.RX.DataBit << 1) | __GPIO_READ(GPIOA, 1);
    // Check for TDP symbols
    if (DRV.RX.DataBit == DRV_RX_DATA_TDP) {
      // TDP symbol found, change to active state
      DRV_RX_SetStatus(DRV_RX_STATUS_ACTIVE);
    }
    // Check for TDP timeout
    if (!--DRV.RX.WaitCount) {
      // TDP symbol not found, set back receiver to idle state
      DRV_RX_SetStatus(DRV_RX_STATUS_IDLE);
    }
  } else if (DRV.RX.Status == DRV_RX_STATUS_ACTIVE) {
    // System waits for new data and sample it to RX FIFO buffer
    // Check for interleaving mode and fetch data
    if (!DRV.RX.SR.RLL || !(DRV.RX.DataCount & 1)) {
      DRV.RX.DataBit = (DRV.RX.DataBit << 1) | __GPIO_READ(GPIOA, 1);
    }
    // Check for data count
    if (!--DRV.RX.DataCount) {
      // Reset the data count
      DRV_RX_WriteReset();
      // Extract the actual 8 bit data
      Data = DRV.RX.DataBit & 0xff;
      if (Data == 0xff || PHY_RX_Write(Data)) {
        // End receiving message
        DRV_RX_SetStatus(DRV_RX_STATUS_IDLE);
      }
    }
  }
}

// Staged synchronization with FLP clocks
void DRV_RX_Sync(void) {
  TIM_TypeDef *TIM = DRV.RX.htim->Instance;
  uint32_t NewPeriod;

  // Get new period
  NewPeriod = TIM->CCR2 - DRV.RX.ICValue;
  DRV.RX.ICValue = TIM->CCR2;

  if (DRV.RX.Status == DRV_RX_STATUS_SYNC) {
    // Synchronized status, examine the signal thoroughly
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
      // Reset the synchronize period
      DRV.RX.Period = NewPeriod;
      DRV.RX.SyncCount = DRV_RX_SYNC_COUNT;
      if (!--DRV.RX.IdleCount) {
        // Can't synchronize with the signal, set back status
        DRV_RX_SetStatus(DRV_RX_STATUS_IDLE);
      }
    }
  } else if (DRV.RX.Status == DRV_RX_STATUS_IDLE) {
    // Check for suspecting FLP signal (had same input capture value)
    if (__delta(NewPeriod, DRV.RX.Period) < 2) {
      if (!--DRV.RX.IdleCount) {
        // FLP signal confirmed, set to synchronized status
				DRV_RX_SetStatus(DRV_RX_STATUS_SYNC);
      }
    } else {
      // Get new period
      DRV.RX.Period = NewPeriod;
      DRV.RX.IdleCount = DRV_RX_IDLE_COUNT;
    }
  }
}

// General purpose state machine set with some configurations
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
    TIM->PSC = 1000;
    // Clear input filter on Input Capture
    TIM->CCMR1 &= ~TIM_CCMR1_IC2F;
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
    TIM->PSC = 50;
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
    // Set filter to Input Capture
    TIM->CCMR1 |= TIM_CCMR1_IC2F;
    // Reset the timer
    TIM->EGR |= TIM_EGR_UG;
    // Start output compare interrupt
    HAL_TIM_OC_Start_IT(DRV.RX.htim, TIM_CHANNEL_1);
    //HAL_TIM_IC_Stop_IT(DRV.RX.htim, TIM_CHANNEL_2);
  } else if (Status == DRV_RX_STATUS_ACTIVE &&
      DRV.RX.Status == DRV_RX_STATUS_WAIT) {
    // Only change status to active on sync
    // Reset the data counter
    DRV_RX_WriteReset();
  } else {
    // Illegal status set
    return;
  }
  // Set the appropriate status
  DRV.RX.Status = Status;
}

/* Transmission functions */
// Transmission data reset handler
void DRV_TX_SetData(void) {
  if (DRV.TX.Status == DRV_TX_STATUS_VISIBILITY) {
    DRV.TX.Data = (uint8_t *) DRV_TX_Visibility;
    DRV.TX.DataLen = sizeof(DRV_TX_Visibility) << 3;
  } else if (DRV.TX.Status == DRV_TX_STATUS_SYNC) {
    DRV.TX.Data = (uint8_t *) DRV_TX_StartBit;
    DRV.TX.DataLen = sizeof(DRV_TX_StartBit) << 3;
  } else if (DRV.TX.Status == DRV_TX_STATUS_ACTIVE) {
    DRV.TX.Data = DRV.TX.Send;
    DRV.TX.DataLen = DRV.TX.SendLen;
  } else if (DRV.TX.Status == DRV_TX_STATUS_STOP) {
    DRV.TX.Data = DRV_TX_Stop;
    DRV.TX.DataLen = 1;
  }
}

// Preload bit to the handle register
void DRV_TX_Preload(void) {
  uint8_t BitPos;

  // Do not continue on reset
  if (DRV.TX.Status == DRV_TX_STATUS_RESET) return;

  // Set for the next state
  if (!DRV.TX.DataLen) {
    if (DRV.TX.Status == DRV_TX_STATUS_VISIBILITY) {
      DRV_TX_SetData();
    } else if (DRV.TX.Status == DRV_TX_STATUS_SYNC) {
      DRV_TX_SetStatus(DRV_TX_STATUS_ACTIVE);
    } else if (DRV.TX.Status == DRV_TX_STATUS_ACTIVE) {
      if (DRV.TX.SR.Visibility) {
        DRV_TX_SetStatus(DRV_TX_STATUS_VISIBILITY);
      } else {
        DRV_TX_SetStatus(DRV_TX_STATUS_STOP);
      }
    } else if (DRV.TX.Status == DRV_TX_STATUS_STOP) {
      DRV_TX_SetStatus(DRV_TX_STATUS_RESET);
    }
  }

  // Check if the transmission is in RLL mode or not
  if (DRV.TX.SR.RLL && DRV.TX.Status == DRV_TX_STATUS_ACTIVE) {
    // RLL mode, use 16 bit for position
    BitPos = --DRV.TX.DataLen & 15;
    DRV.TX.PreloadBit = (*DRV.TX.Data >> (BitPos >> 1)) & 1;
    // Flip bit on second RLL
    if (!(DRV.TX.DataLen & 1)) {
      DRV.TX.PreloadBit ^= 1;
    }
  } else {
    // Not in RLL mode, continue using 8 bit
    BitPos = --DRV.TX.DataLen & 7;
    DRV.TX.PreloadBit = (*DRV.TX.Data >> BitPos) & 1;
  }

  if (!BitPos) {
    // Advance data counter on zero
    DRV.TX.Data++;
  }
}

// General purpose send function
void DRV_TX_Send(uint8_t *Data, uint32_t DataLen) {
  DRV.TX.Data = Data;

  // Check for data length by RLL coding
  if (DRV.TX.SR.RLL) {
    DRV.TX.SendLen = DataLen << 4;
  } else {
    DRV.TX.SendLen = DataLen << 3;
  }

  // Set start bit template
  DRV.TX.Send = Data;
  // Activate the transmission module
  DRV_TX_SetStatus(DRV_TX_STATUS_SYNC);
}

// General purpose state machine set with some configurations
void DRV_TX_SetStatus(DRV_TX_StatusTypeDef Status) {
  //TIM_TypeDef *TIM = DRV.TX.htim->Instance;

  if (Status == DRV_TX_STATUS_RESET) {
    // Reset status, stop all timers
    HAL_TIM_Base_Stop_IT(DRV.TX.htim);
  } else if ((Status == DRV_TX_STATUS_VISIBILITY &&
      DRV.TX.SR.Visibility) || Status == DRV_TX_STATUS_SYNC) {
    // Start the timer base interrupt
    HAL_TIM_Base_Start_IT(DRV.TX.htim);
  } else if (Status == DRV_TX_STATUS_ACTIVE &&
             DRV.TX.Status == DRV_TX_STATUS_SYNC) {
    // Check if there is nothing to send
    if (!DRV.TX.Send || !DRV.TX.SendLen) return;
  } else if (Status == DRV_TX_STATUS_STOP &&
              DRV.TX.Status == DRV_TX_STATUS_ACTIVE) {
    // Test
  } else {
    // Invalid status set
    return;
  }

  // Set the new status
  DRV.TX.Status = Status;
  // Set the data
  DRV_TX_SetData();
}

/* Interrupt callback */
// Interrupt callback on Input Capture match
void DRV_RX_TimerICCallback(void) {
  TIM_TypeDef *TIM = DRV.RX.htim->Instance;
  
  if (DRV.RX.Status == DRV_RX_STATUS_IDLE ||
      DRV.RX.Status == DRV_RX_STATUS_SYNC) {
    DRV_RX_Sync();
  } else if (DRV.RX.Status == DRV_RX_STATUS_WAIT ||
      DRV.RX.Status == DRV_RX_STATUS_ACTIVE) {
    // Reset the timer to match the drifts
    TIM->EGR |= TIM_EGR_UG;
	}
}

// Interrupt callback on Output Compare match
void DRV_RX_TimerOCCallback(void) {
  if (DRV.RX.Status == DRV_RX_STATUS_WAIT ||
      DRV.RX.Status == DRV_RX_STATUS_ACTIVE) {
    DRV_RX_Write();
  }
}

// Interrupt callback on Overflow match
void DRV_TX_TimerOverflowCallback(void) {
  // Send the data
  if (DRV.TX.PreloadBit) {
    TIM4->CCR1 = 2210;
    __GPIO_WRITE(GPIOA, 9, GPIO_PIN_SET);
  } else {
    TIM4->CCR1 = 1105;
    __GPIO_WRITE(GPIOA, 9, GPIO_PIN_RESET);
  }

  DRV_TX_Preload();
}

