//
// Created by Adzil on 28/04/2016.
//

#ifndef __LOCK
#define __LOCK

#include <stm32f4xx.h>

inline void LOCK_Start(volatile uint8_t *Lock) {
  uint8_t Status = 0;

  do {
    // Wait until the lock has been released
    while(__LDREXB(Lock) != 0);
    // Try to lock the variable
    Status = __STREXB(1, Lock);
  } while (Status != 0); // Try to lock again if failed
  // Call memory barrier
  __DMB();
}

inline void LOCK_End(volatile uint8_t *Lock) {
  // Call memory barrier
  __DMB();
  // Release lock
  *Lock = 0;
}

#endif //F446_SOFTSER_LOCK_H
