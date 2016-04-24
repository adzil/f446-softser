#ifndef __NIBBLE
#define __NIBBLE

#include <inttypes.h>

typedef union {
  struct {
    uint8_t Low: 4;
    uint8_t High: 4;
  };
  uint8_t Data;
} NibbleTypeDef;

inline void NibbleWrite(NibbleTypeDef *Nibble, int Id, uint8_t Data) {
  // Address Offset
  Nibble += Id;
  if (Id & 1) {
    Nibble->High = Data;
  } else {
    Nibble->Low = Data;
  }
}

inline uint8_t NibbleRead(NibbleTypeDef *Nibble, int Id) {
  // Address Offset
  Nibble += Id;
  if (Id & 1) {
    return Nibble->High;
  } else {
    return Nibble->Low;
  }
}

#endif __NIBBLE
