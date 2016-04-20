#ifndef __PHY
#define __PHY

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "tim.h"
#include "macros.h"
#include "cmsis_os.h"

#define __min_abs(a, b) ((a > b) ? (a - b) : (b - a))

/* Function prototypes */
void PHY_Sample(uint8_t Bit);

#endif
