#ifndef __PHY
#define __PHY

#include <stm32f4xx.h>
#include <stm32f4xx_hal.h>
#include <inttypes.h>
#include <drv.h>
#include <memory.h>
#include <buffer.h>
#include <cmsis_os.h>
#include <macros.h>

#define PHY_RX_BUFFER_SIZE 2048

typedef struct {
  BUF_HandleTypeDef Buffer;
} PHY_RX_HandleTypeDef;

typedef struct {
  PHY_RX_HandleTypeDef RX;
} PHY_HandleTypeDef;

extern osThreadId PHY_RX_ThreadId;

void PHY_RX_Thread(const void *argument);

#endif // __PHY