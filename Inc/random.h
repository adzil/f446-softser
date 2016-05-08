#ifndef __RANDOM__
#define __RANDOM__

#include <stdlib.h>
#include <inttypes.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "adc.h"

void RND_Init(void);
uint32_t RND_Get(void);

#endif // __RANDOM__