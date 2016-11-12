#ifndef __APPDEF_H__
#define __APPDEF_H__

#include <inttypes.h>

#define force_inline static inline __attribute__((always_inline))

/* Reverse utility */
/*
static uint32_t __REV(uint32_t DIN) {
  return (DIN >> 24) | ((DIN & 0xff0000) >> 8) | ((DIN & 0xff00) << 8) |
         (DIN << 24);
}

static uint32_t __REV16(uint32_t DIN) {
  return ((DIN & 0xff00) >> 8) | ((DIN & 0xff) << 8);
}
*/

#endif // __APPDEF_H__
