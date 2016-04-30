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
#include "crc16.h"

#define PHY_BUFFER_SIZE 2048
#define PHY_CC_DEPTH 7
#define PHY_CC_MEMORY_LENGTH (1 << (PHY_CC_DEPTH - 1))
#define PHY_CC_DECODE_RESET_COUNT 32
#define PHY_CC_DECODE_CONTINUE_COUNT 8
#define PHY_RS_N 15
#define PHY_RS_K 7

#define PHY_RS_OUTPUT_LEN(N, K, LEN) (N * __CEIL_DIV(LEN, K) - \
    (K - (LEN % K)))
#define PHY_CC_OUTPUT_LEN(LEN) (LEN * 4 + 3);

#define PHY_HEADER_LEN 4
//#define PHY_HEADER_RS_LEN PHY_RS_OUTPUT_LEN(PHY_RS_N, PHY_RS_K, \
//    PHY_HEADER_LEN)
#define PHY_HEADER_CC_LEN PHY_CC_OUTPUT_LEN(PHY_HEADER_LEN)

typedef enum {
  PHY_RX_STATUS_RESET,
  PHY_RX_STATUS_PROCESS_HEADER,
  PHY_RX_STATUS_PROCESS_PAYLOAD
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
  uint8_t Counter;
} PHY_CC_HandleTypeDef;

typedef struct {
  uint8_t DUMMY;
} PHY_RS_HandleTypeDef;

typedef struct {
  BUF_HandleTypeDef Buffer;
  uint8_t *ProcessPtr;
  uint8_t *Process;
  uint16_t ProcessLen;
  uint16_t PayloadLen;
  uint16_t ReceiveLen;
  uint16_t TotalLen;
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

typedef struct {
  uint32_t PsduLength;
  uint8_t *Psdu;
} PHY_DataTypeDef;

//extern PHY_HandleTypeDef PHY;

void PHY_Init(void);

void PHY_RX_Activate(void);
void PHY_TX_Activate(void);
void PHY_RX_SetStatus(PHY_RX_StatusTypeDef Status);
void PHY_TX_SetStatus(PHY_TX_StatusTypeDef Status);

/* Thread specific definitions */
extern osThreadId PHY_RX_ThreadId;
extern osThreadId PHY_TX_ThreadId;
void PHY_RX_Thread(const void *argument);
void PHY_TX_Thread(const void *argument);

/* APIs */
uint8_t PHY_API_SendStart(uint8_t *Data, uint16_t DataLen);
void PHY_API_SendComplete(void);
uint8_t PHY_API_DataReceived(uint8_t Data);

#endif // __PHY
