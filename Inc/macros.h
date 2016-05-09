#ifndef __MACROS
#define __MACROS

#include <inttypes.h>
//#include <string.h>
//#include "stm32f4xx.h"

//#define APP_DEBUG

#define __GPIO_WRITE(GPIOx, PIN, STATE) if (STATE) GPIOx->BSRR |= (1<<PIN); \
		else GPIOx->BSRR |= (0x10000<<PIN)
#define __GPIO_READ(GPIOx, PIN) (uint8_t)((GPIOx->IDR >> PIN) & 1)
#define __CEIL_DIV(a, b) ((a + b - 1) / b)
#define _inline_ __attribute__((always_inline)) static inline

#define MemberSize(STRUCT, MEMBER) sizeof(((STRUCT *)0)->MEMBER)

#define NEXT_SRC 1
#define NEXT_DST 2

// Copy and reverse function
#define __CopyByte(DST, SRC) *((uint8_t *) DST) = *((uint8_t *) SRC)
#define __CopyWord(DST, SRC) *((uint16_t *) DST) = __REV16(*((uint16_t *) SRC))
#define __CopyDword(DST, SRC) *((uint32_t *) DST) = __REV(*((uint32_t *) SRC))

#define ByteToBuffer(DST, SRC) do {__CopyByte(DST, SRC); DST += 1;} while(0)
#define ByteFromBuffer(DST, SRC) do {__CopyByte(DST, SRC); SRC += 1;} while(0)

#define WordToBuffer(DST, SRC) do {__CopyWord(DST, SRC); DST += 2;} while(0)
#define WordFromBuffer(DST, SRC) do {__CopyWord(DST, SRC); SRC += 2;} while(0)

#define DwordToBuffer(DST, SRC) do {__CopyDword(DST, SRC); DST += 4;} while(0)
#define DwordFromBuffer(DST, SRC) do {__CopyDword(DST, SRC); SRC += 4;} while(0)

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
