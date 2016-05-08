// Pseudo-Random number generator
#include <random.h>
#include "lock.h"

LOCK_Handle RND_Lock;

void RND_Init(void) {
  int i;
  uint32_t Seed;

  Seed = 0;
  HAL_ADC_Start(&hadc1);
  for (i = 0; i < 8; i++) {
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    Seed = (Seed << 4) | (HAL_ADC_GetValue(&hadc1) & 0xf);
  }
  HAL_ADC_Stop(&hadc1);
  // Set random seed value
  srand(Seed);
}

uint32_t RND_Get(void) {
  uint32_t RndVal;

  LOCK_Start(&RND_Lock);
  RndVal = (uint32_t) rand();
  LOCK_End(&RND_Lock);

  return RndVal;
}