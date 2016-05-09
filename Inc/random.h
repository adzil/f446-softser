#ifndef __RANDOM__
#define __RANDOM__

#include "macros.h"
#include <inttypes.h>
#ifndef APP_DEBUG
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "adc.h"
#else
#include <stdlib.h>
#endif

#ifdef APP_DEBUG
_inline_ uint8_t RND_Get(void) {
  return rand() & 0xff;
}
#else
uint8_t RND_Get(void);
#endif

#endif // __RANDOM__
