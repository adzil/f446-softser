#ifndef __MACROS
#define __MACROS

#include <inttypes.h>

#define __GPIO_WRITE(GPIOx, PIN, STATE) if (STATE) GPIOx->BSRR |= (1<<PIN); \
		else GPIOx->BSRR |= (0x10000<<PIN)
#define __GPIO_READ(GPIOx, PIN) (uint8_t)((GPIOx->IDR >> PIN) & 1)
#define __CEIL_DIV(a, b) ((a + b - 1) / b)
#define _inline_ __attribute__((always_inline)) static inline

#define struct_size(STRUCT, MEMBER) sizeof(((STRUCT *)0)->MEMBER)

// Reverse endian-ness
#define __GENTYPE(DATA, SIZE) (\
(SIZE == 4) ? *((uint32_t *) DATA) : (\
(SIZE == 2) ? *((uint16_t *) DATA) : \
*((uint8_t *) DATA)  									))

#define __GENREV(DATA, SIZE) (\
(SIZE == 4) ? __REV(__GENTYPE(DATA, SIZE)) : (\
(SIZE == 2) ? __REV16(__GENTYPE(DATA, SIZE)) :\
__GENTYPE(DATA, SIZE)													))

#define __GENOP(DST, SRC, SIZE) do {\
if (SIZE == 4) *((uint32_t *) DST) = __GENREV(SRC, SIZE);\
else if (SIZE == 2) *((uint16_t *) DST) = __GENREV(SRC, SIZE);\
else *((uint8_t *) DST) = __GENREV(SRC, SIZE);\
} while(0)

// Copy memory contents to buffer
#define __ToBuffer(DST, SRC, SIZE) do {\
__GENOP(DST, SRC, SIZE);\
DST += SIZE;\
} while(0)
#define ToBuffer(DST, SRC) __ToBuffer(DST, SRC, sizeof(*SRC));

#define __FromBuffer(DST, SRC, SIZE) do {\
__GENOP(DST, SRC, SIZE);\
SRC += SIZE;\
} while(0)
#define FromBuffer(DST, SRC) __FromBuffer(DST, SRC, sizeof(*DST));

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
