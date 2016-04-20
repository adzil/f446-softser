#include "phy.h"

#define PHY_RX_SYNC_TIMEOUT 256
#define PHY_DATA_TDP 0x9AD76528
#define PHY_DATA_COUNT 8
#define PHY_SAMPLE_COUNT 8

typedef enum {
	PHY_RX_IDLE = 0x0,
	PHY_RX_READY = 0x1,
	PHY_RX_SYNC = 0x2,
	PHY_RX_ACTIVE = 0x4
} PHY_RXStatus_t;

typedef struct {
	PHY_RXStatus_t Status;
	uint32_t DataBit;
	uint32_t LastIC;
  uint32_t BitPeriod;
	uint8_t SampleBit;
	uint8_t TriggerCount;
	uint8_t SampleCount;
	uint8_t DataCount;
  uint16_t SyncTimeout;
} PHY_RXHandle_t;

PHY_RXHandle_t hprx;

extern osThreadId tid_sendSerial;

uint8_t printval;

#define __decide(a) (a > 4) ? 1:0

void __PHY_RXSET_IDLE(void) {
	// Disable all peripherals
	HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_2);
	HAL_TIM_OC_Stop_IT(&htim2, TIM_CHANNEL_1);
	HAL_TIM_Base_Stop(&htim2);
  
  hprx.Status = PHY_RX_IDLE;
}

void __PHY_RXSET_READY(void) {
  // Stop OC interrupt
	HAL_TIM_OC_Stop_IT(&htim2, TIM_CHANNEL_1);
	// Set TIM2 register values
	htim2.Instance->ARR = 0xffffffff;
	htim2.Instance->CNT = 0;
	// Initiate TIM2 and TIM2 Input Capture interrupt
	HAL_TIM_Base_Start(&htim2);
	HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
  
  hprx.TriggerCount = 0;
  hprx.Status = PHY_RX_READY;
}

void __PHY_RXSET_SYNC(uint32_t syncVal) {
	// Set TIM2 register values
	htim2.Instance->ARR = syncVal >> 3;
	htim2.Instance->CCR1 = syncVal >> 4;
	htim2.Instance->CNT = 0;
	// Initiate TIM2 and TIM2 Input Capture interrupt
	HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_1);
  
  hprx.SyncTimeout = PHY_RX_SYNC_TIMEOUT;
  hprx.Status = PHY_RX_SYNC;
}

void __PHY_RXSET_ACTIVE(void) {
  hprx.Status = PHY_RX_ACTIVE;
}

void PHY_RXInit(void) {
	hprx.Status = PHY_RX_IDLE;
	hprx.LastIC = 0;
	hprx.TriggerCount = 0;
	hprx.BitPeriod = 0;
	hprx.DataCount = PHY_DATA_COUNT;
	hprx.SampleCount = PHY_SAMPLE_COUNT;
}

void PHY_RXStart(void) {
	// Checks if the PHY is on idle mode
	if (hprx.Status != PHY_RX_IDLE) return;
	__PHY_RXSET_READY();
}

void PHY_RXStop(void) {
	// Checks if the PHY is on idle mode
	if (hprx.Status == PHY_RX_IDLE) return;
	__PHY_RXSET_IDLE();
}

void PHY_Init(void) {
  PHY_RXInit();
  PHY_RXStart();
}

__inline void __PHY_RXResetSample(void) {
  hprx.SampleCount = PHY_SAMPLE_COUNT;
}

__inline void __PHY_RXResetData(void) {
  hprx.DataCount = PHY_DATA_COUNT;
}

void __PHY_RXWriteData(void) {
  uint8_t DataOut;
  
	// Reset sample counter
	__PHY_RXResetSample();
	// Write to data bit
	hprx.DataBit = (hprx.DataBit << 1) | (__decide(__popcnt8(hprx.SampleBit)));
  DataOut = hprx.DataBit & 0xff;
  // Checks if the data is full
  if (!--hprx.DataCount) {
    __PHY_RXResetData();
    
    if (hprx.Status == PHY_RX_ACTIVE) {
      if (DataOut != 0xff) {
        printval = DataOut;
        osSignalSet(tid_sendSerial, 1);
      } else {
        __PHY_RXSET_READY();
      }
    }
  }
  
  // Check for TDP
  if (hprx.Status == PHY_RX_SYNC) {
    if (__popcnt32(hprx.DataBit ^ PHY_DATA_TDP) < 8) {
      __PHY_RXResetData();
      __PHY_RXSET_ACTIVE();
    }
    if (!--hprx.SyncTimeout) {
      __PHY_RXSET_READY();
    }
  }
}

void __PHY_RXWriteSample(void) {
	// Get new sample bit
	hprx.SampleBit = (hprx.SampleBit << 1) | (__GPIO_READ(GPIOA, 1));
	// Checks if the sample is full
	if (!--hprx.SampleCount) {
    __PHY_RXWriteData();
  }
}

// Get Fast Locking Pattern (FLP) by using edge triggered detectors
void __PHY_RXGetFLP(void) {
  uint32_t SyncTime;
  
  // Get synchronization time (it should be PHY_RX_SPEED * 2)
  SyncTime = htim2.Instance->CCR2 - hprx.LastIC;
  hprx.LastIC = htim2.Instance->CCR2;
  // Get the delta SyncTime with the Speed reference (PHY_RX_SPEED * 2)
  if ((__min_abs(SyncTime, hprx.BitPeriod)) < 4) {
    // The difference is tolerable, add the trigger counter
    if (++hprx.TriggerCount >= 15) {
      // The trigger counter OK, set status to SYNCED
      __PHY_RXResetSample();
      __PHY_RXResetData();
      __PHY_RXSET_SYNC(hprx.BitPeriod >> 1);
    }
  } else {
    // Delta mismatch, reset the trigger counter
    hprx.TriggerCount = 0;
    hprx.BitPeriod = SyncTime;
  }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
  if (htim == &htim2) {
		if (hprx.Status == PHY_RX_READY) {
      // Get Fast Locking Pattern (FLP)
			__PHY_RXGetFLP();
		} else {
      // Resync the timer
      if (hprx.SampleCount < 3) {
        // Resync with the clock edge
        htim2.Instance->CNT = 0;
        __PHY_RXWriteData();
      } else if (hprx.SampleCount > 6) {
        htim2.Instance->CNT = 0;
        __PHY_RXResetSample();
      }
    }
  }
}

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim == &htim2) {
    // Write new sample on Output compare
		__PHY_RXWriteSample();
  }
}
