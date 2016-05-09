//
// Created by Adzil on 28/04/2016.
//

#ifndef __LOCK
#define __LOCK

#include "macros.h"
#ifndef APP_DEBUG
#include <stm32f4xx.h>
#include "cmsis_os.h"
#endif

typedef volatile uint8_t LOCK_Handle;

#ifndef APP_DEBUG
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
#else
_inline_ void LOCK_Start(LOCK_Handle *Lock) {
  *Lock = 1;
}

_inline_ void LOCK_End(LOCK_Handle *Lock) {
  *Lock = 0;
}
#endif

#endif //F446_SOFTSER_LOCK_H
