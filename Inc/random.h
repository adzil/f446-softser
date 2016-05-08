#ifndef __RANDOM__
#define __RANDOM__

#include <stdlib.h>
#include <inttypes.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "adc.h"

uint8_t RND_Get(void);

#endif // __RANDOM__
