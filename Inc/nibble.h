#ifndef __NIBBLE
#define __NIBBLE

#pragma anon_unions

#include <inttypes.h>
#include <macros.h>

typedef union {
  struct {
    uint8_t Low: 4;
    uint8_t High: 4;
  };
  uint8_t Data;
} NibbleTypeDef;

_inline_ void NibbleWrite(NibbleTypeDef *Nibble, int Id, uint8_t Data) {
  // Address Offset
  Nibble += (Id >> 1);
  if (Id & 1) {
    Nibble->Low = Data;
  } else {
    Nibble->High = Data;
  }
}

_inline_ uint8_t NibbleRead(NibbleTypeDef *Nibble, int Id) {
  // Address Offset
  Nibble += (Id >> 1);
  if (Id & 1) {
    return Nibble->Low;
  } else {
    return Nibble->High;
  }
}

#endif //__NIBBLE
