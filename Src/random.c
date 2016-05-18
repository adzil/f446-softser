// Pseudo-Random number generator
#include <random.h>
#include "stm32f4xx.h"
#include "lock.h"
#include "adc.h"

LOCK_Handle RND_Lock;

uint8_t RND_Get(void) {
  int i;
  uint32_t RndVal;

  RndVal = 0;
  LOCK_Start(&RND_Lock);
	
  HAL_ADC_Start(&hadc1);
  for (i = 0; i < 4; i++) {
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    RndVal = (RndVal << 2) | (HAL_ADC_GetValue(&hadc1) & 0x3);
  }
  HAL_ADC_Stop(&hadc1);
	
  LOCK_End(&RND_Lock);

  return (uint8_t)(RndVal & 0xff);
}
