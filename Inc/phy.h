/**
 * Physical Layer for VLC
 */

#ifndef __PHY
#define __PHY

#include <inttypes.h>
#include "memory.h"
#include "buffer.h"
#include "cmsis_os.h"
#include "lock.h"
#include "drv.h"
#include "usart.h"
#include "macros.h"

#define PHY_BUFFER_SIZE 2048
#define PHY_CC_DEPTH 7
#define PHY_CC_MEMORY_LENGTH (1 << (PHY_CC_DEPTH - 1))

#define PHY_HEADER_LENGTH 2

typedef enum {
  PHY_RX_STATUS_RESET,
  PHY_RX_STATUS_PROC_HEADER,
  PHY_RX_STATUS_PROC_PAYLOAD
} PHY_RX_StatusTypeDef;

typedef enum {
  PHY_TX_STATUS_RESET,
  PHY_TX_STATUS_ACTIVE
} PHY_TX_StatusTypeDef;

typedef struct {
  uint16_t *Distance;
  uint16_t *LastDistance;
  uint32_t *Data;
  uint32_t *LastData;
  uint8_t MinId;
  uint8_t State;
} PHY_CC_HandleTypeDef;

typedef struct {
  uint8_t DUMMY;
} PHY_RS_HandleTypeDef;

typedef struct {
  BUF_HandleTypeDef Buffer;
  uint8_t *WriteBuffer;
  uint8_t *WriteBufferData;
  uint16_t WriteBufferLength;
  uint16_t ReceiveLength;
  uint16_t Length;
  PHY_RX_StatusTypeDef Status;
} PHY_RX_HandleTypeDef;

typedef struct {
  // Status register emulation
  uint8_t *Payload;
  uint8_t *Header;
  struct {
    uint8_t TXComplete;
  }SR;
  PHY_TX_StatusTypeDef Status;
} PHY_TX_HandleTypeDef;

typedef struct {
  PHY_CC_HandleTypeDef CC;
  PHY_RS_HandleTypeDef RS;
  PHY_RX_HandleTypeDef RX;
  PHY_TX_HandleTypeDef TX;
} PHY_HandleTypeDef;

//extern PHY_HandleTypeDef PHY;

void PHY_Init(void);

void PHY_Activate(void);
void PHY_RX_SetStatus(PHY_RX_StatusTypeDef Status);
void PHY_TX_SetStatus(PHY_TX_StatusTypeDef Status);

/* Thread specific definitions */
extern osThreadId PHY_ThreadId;
void PHY_Thread(const void *argument);

/* APIs */
uint8_t PHY_API_SendStart(uint8_t *Data, uint16_t DataLen);
void PHY_API_SendComplete(void);
uint8_t PHY_API_DataReceived(uint8_t Data);

#endif // __PHY
