/** Self-header Includes **/
#include "opt.h"

#define OPT_RX_TRIGGER_COUNT 15

// Configuration macros
#define __HTIM htim2
#define __TIM __HTIM.Instance

OPT_RXHandleType hoptrx;

/* Function prototypes */
void __OPT_RXTriggerReset(OPT_RXHandleType *Handle);
void __OPT_RXTriggerWrite(OPT_RXHandleType *Handle);
void __OPT_RXSetStatus(OPT_RXHandleType *Handle, OPT_RXStatusType Status);

/* Private Methods */
inline void __OPT_RXTriggerReset(OPT_RXHandleType *Handle) {
  Handle->TriggerCount = OPT_RX_TRIGGER_COUNT;
}

void __OPT_RXTriggerWrite(OPT_RXHandleType *Handle) {
  uint32_t NewPeriod;
  
  NewPeriod = Handle->htim->Instance->CCR2 - Handle->LastIC;
  Handle->LastIC = Handle->htim->Instance->CCR2;
  
  if (__min_abs(NewPeriod, Handle->BitPeriod) < 4) {
    if (!--Handle->TriggerCount) {
      __OPT_RXSetStatus(Handle, OPT_RXStatusSync);
    }
  } else {
    __OPT_RXTriggerReset(Handle);
    Handle->BitPeriod = NewPeriod;
  }
}

void __OPT_RXSetStatus(OPT_RXHandleType *Handle, OPT_RXStatusType Status) {
  if (Status == OPT_RXStatusReset) { 
    // Disable all timers and interrupts
    HAL_TIM_OC_Stop_IT(Handle->htim, TIM_CHANNEL_1);
    HAL_TIM_IC_Stop_IT(Handle->htim, TIM_CHANNEL_2);
    HAL_TIM_OC_Stop_IT(Handle->htim, TIM_CHANNEL_3);
    HAL_TIM_OC_Stop_IT(Handle->htim, TIM_CHANNEL_4);
    HAL_TIM_Base_Stop(Handle->htim);
  } else if (Status == OPT_RXStatusReady) {
    // Stop output compare interrupt
    HAL_TIM_OC_Stop_IT(Handle->htim, TIM_CHANNEL_1);
    HAL_TIM_OC_Stop_IT(Handle->htim, TIM_CHANNEL_3);
    HAL_TIM_OC_Stop_IT(Handle->htim, TIM_CHANNEL_4);
    // Realign timer ARR and CNT register
    Handle->htim->Instance->ARR = 0xffffffff;
    Handle->htim->Instance->CNT = 0;
    __OPT_RXTriggerReset(Handle);
    // Start timer and input capture
    HAL_TIM_Base_Start(Handle->htim);
    HAL_TIM_IC_Start_IT(Handle->htim, TIM_CHANNEL_2);
  } else if (Status == OPT_RXStatusSync) {
    // Realign timer ARR, CNT, and CCRx values
    Handle->htim->Instance->ARR = Handle->BitPeriod >> 1;
    Handle->htim->Instance->CCR1 = Handle->BitPeriod >> 3;
    Handle->htim->Instance->CCR3 = Handle->BitPeriod >> 2;
    Handle->htim->Instance->CCR4 = (Handle->BitPeriod >> 2) + (Handle->BitPeriod >> 3);
    Handle->htim->Instance->CNT = 0;
    // Start output compare interrupts
    HAL_TIM_OC_Start_IT(Handle->htim, TIM_CHANNEL_1);
    HAL_TIM_OC_Start_IT(Handle->htim, TIM_CHANNEL_3);
    HAL_TIM_OC_Start_IT(Handle->htim, TIM_CHANNEL_4);
  } else {
    return; // Undefined status value
  }
  // Set the apropriate status 
  Handle->Status = Status;
}

/* Public Methods */

/* Interrupt handlers */
void OPT_RXTimICInterruptCallback(OPT_RXHandleType *Handle) {
  if (Handle->Status == OPT_RXStatusReady) {
    __OPT_RXTriggerWrite(Handle);
  } else
  if (Handle->Status == OPT_RXStatusSync || Handle->Status == OPT_RXStatusActive) {
    __OPT_RXSampleSync(Handle);
  }
}

void OPT_RXTimOCInterruptCallback(OPT_RXHandleType *Handle) {
  if (Handle->Status == OPT_RXStatusSync || Handle->Status == OPT_RXStatusActive) {
    __OPT_RXSampleWrite(Handle);
  }
}