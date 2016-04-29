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

#define PHY_HEADER_LENGTH 12

typedef enum {
  PHY_RX_STATUS_RESET,
  PHY_RX_STATUS_PROCESS_HEADER,
  PHY_RX_STATUS_PROCESS_PAYLOAD
} PHY_RX_StatusTypeDef;

typedef struct {
  BUF_HandleTypeDef Buffer;
  uint16_t ReceivedLen;
  uint16_t TotalLen;
  PHY_RX_StatusTypeDef Status;
} PHY_RX_HandleTypeDef;

typedef struct {
  PHY_RX_HandleTypeDef RX;
} PHY_HandleTypeDef;

extern osThreadId PHY_RX_ThreadId;

uint8_t PHY_API_DataReceived(uint8_t Data);

void PHY_RX_Thread(const void *argument);

#endif // __PHY