#ifndef __MACROS
#define __MACROS

#include <inttypes.h>

#define __GPIO_WRITE(GPIOx, PIN, STATE) if (STATE) GPIOx->BSRR |= (1<<PIN); \
		else GPIOx->BSRR |= (0x10000<<PIN)
#define __GPIO_READ(GPIOx, PIN) (uint8_t)((GPIOx->IDR >> PIN) & 1)
#define _inline_ __attribute__((always_inline)) inline

_inline_ uint8_t __popcnt8(uint8_t in) {
	in -= (in >> 1) & 0x55;
	in = (in & 0x33) + ((in >> 2) & 0x33);
	return (in + (in >> 4)) & 0xf;
}

_inline_ uint16_t __popcnt16(uint16_t in) {
	in -= (in >> 1) & 0x5555;
	in = (in & 0x3333) + ((in >> 2) & 0x3333);
  in = (in + (in >> 4)) & 0xf0f;
	return (in + (in >> 8)) & 0x1f;
}

_inline_ uint32_t __popcnt32(uint32_t in) {
	in -= (in >> 1) & 0x5555;
	in = (in & 0x3333) + ((in >> 2) & 0x3333);
  in = (in + (in >> 4)) & 0xf0f;
  in += (in >> 8);
	return (in + (in >> 16)) & 0x3f;
}

#endif
