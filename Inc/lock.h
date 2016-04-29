//
// Created by Adzil on 28/04/2016.
//

#ifndef __LOCK
#define __LOCK

#include <stm32f4xx.h>
#include "macros.h"
#include "cmsis_os.h"

typedef volatile uint8_t LOCK_Handle;
/*
_inline_ uint8_t LOCK_ReadB(uint8_t *Address) {
  return __LDREXB(Address);
}

_inline_ uint16_t LOCK_ReadH(uint16_t *Address) {
  return __LDREXH(Address);
}

_inline_ uint32_t LOCK_ReadW(uint32_t *Address) {
  return __LDREXW(Address);
}

_inline_ void LOCK_WriteB(uint8_t Data, uint8_t *Address) {
  while(__STREXB(Data, Address));
}

_inline_ void LOCK_WriteH(uint16_t Data, uint16_t *Address) {
  while(__STREXH(Data, Address));
}

_inline_ void LOCK_WriteW(uint32_t Data, uint32_t *Address) {
  while(__STREXW(Data, Address));
}
*/
_inline_ void LOCK_Start(LOCK_Handle *Lock) {
  uint8_t Status = 0;

  do {
    // Wait until the lock has been released
    while(__LDREXB(Lock) != 0) {
      // Give chance to another thread to release the lock
      osThreadYield();
    }
    // Try to lock the variable
    Status = __STREXB(1, Lock);
  } while (Status != 0); // Try to lock again if failed
  // Call memory barrier
  __DMB();
}

_inline_ void LOCK_End(LOCK_Handle *Lock) {
  // Call memory barrier
  __DMB();
  // Release lock
  *Lock = 0;
}

#endif //F446_SOFTSER_LOCK_H
