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
#include "fec.h"

#define PHY_BUFFER_SIZE 2048

#define PHY_CC_ENCODE_LEN(LEN) (LEN * 4 + 3)
#define PHY_CC_DECODE_LEN(LEN) ((LEN - 3) / 4)

#define PHY_HEADER_LEN 4
//#define PHY_HEADER_RS_LEN PHY_RS_OUTPUT_LEN(PHY_RS_N, PHY_RS_K, \
//    PHY_HEADER_LEN)
#define PHY_HEADER_CC_LEN PHY_CC_ENCODE_LEN(PHY_HEADER_LEN)

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
  BUF_HandleTypeDef Buffer;
  uint16_t ReceiveLen;
  uint16_t TotalLen;
  uint16_t PayloadLen;
  struct {
    uint8_t *ProcessData;
    uint8_t *Data;
    uint16_t ProcessLen;
    uint16_t Len;
  } Process;
  PHY_RX_StatusTypeDef Status;
} PHY_RX_HandleTypeDef;

typedef struct {
  PHY_RX_HandleTypeDef RX;
} PHY_HandleTypeDef;

//extern PHY_HandleTypeDef PHY;

void PHY_Init(void);

void PHY_RX_Activate(void);
void PHY_RX_SetStatus(PHY_RX_StatusTypeDef Status);
void PHY_TX_SetStatus(PHY_TX_StatusTypeDef Status);

/* Thread specific definitions */
extern osThreadId PHY_RX_ThreadId;
extern osThreadId PHY_TX_ThreadId;
void PHY_RX_Thread(const void *argument);
void PHY_TX_Thread(const void *argument);

/* APIs */
uint8_t PHY_API_SendStart(uint8_t *Data, uint16_t DataLen);
uint8_t PHY_API_DataReceived(uint8_t Data);

#endif // __PHY
