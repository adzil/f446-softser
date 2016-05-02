//
// Created by Adzil on 02/05/2016.
//

#ifndef __DEBUG__
#define __DEBUG__

#include <string.h>
#include "usart.h"
#include "macros.h"

_inline_ void Log(const char *msg) {
  HAL_UART_Transmit(&huart2, (uint8_t *) msg, strlen(msg), 0xffff);
}

#endif // __DEBUG__
