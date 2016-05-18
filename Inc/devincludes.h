#ifndef __DEVINCLUDES__
#define __DEVINCLUDES__

#include <inttypes.h>

#include "random.h"
#include "memory.h"

//#define IMPLX86

#ifndef IMPLX86
#include "stm32f4xx.h"
#else
_inline_ uint32_t __REV16(uint32_t brev) {
  return ((brev & 0xff00) >> 8) | ((brev & 0xff) << 8);
}

_inline_ uint32_t __REV(uint32_t brev) {
  return (brev >> 24) | ((brev & 0xff0000) >> 8) |
      ((brev & 0xff00) << 8) | (brev << 24);
}
#endif

#endif
