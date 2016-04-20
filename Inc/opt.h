/** Header Guard **/
#ifndef __OPT
#define __OPT

#include <inttypes.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "tim.h"
#include "macros.h"

#define __min_abs(a, b) ((a > b) ? (a - b) : (b - a))

#define OPT_RXSampleStart(Handle) ((Handle)->SampleValue = __GPIO_READ(GPIOA, 1))

typedef enum {
  OPT_RXStatusReset = 0,
  OPT_RXStatusReady = 1,
  OPT_RXStatusSync = 2,
  OPT_RXStatusActive = 4
} OPT_RXStatusType;

typedef struct {
  OPT_RXStatusType Status;
  uint32_t DataBit;
  uint32_t LastIC;
  uint32_t BitPeriod;
  TIM_HandleTypeDef *htim;
  uint8_t TriggerCount;
  uint8_t SampleValue;
} OPT_RXHandleType;

extern OPT_RXHandleType hoptrx;

void OPT_RXTimICInterruptCallback(OPT_RXHandleType *Handle);
void OPT_RXTimOCInterruptCallback(OPT_RXHandleType *Handle);

#endif // __OPT