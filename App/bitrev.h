#ifndef __BITREV__
#define __BITREV__

#include <inttypes.h>
#include "macros.h"

_inline_ uint16_t __REV16(uint16_t brev) {
  return ((brev & 0xff00) >> 8) | ((brev & 0xff) << 8);
}

_inline_ uint32_t __REV(uint32_t brev) {
  return ((brev & 0xff000000) >> 24) | ((brev & 0xff0000) >> 8) |
      ((brev & 0xff00) << 8) | ((brev & 0xff) << 24);
}

#endif
