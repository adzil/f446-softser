/**
 * Physical Layer for VLC
 */

#ifndef __PHY
#define __PHY

#include <inttypes.h>
#include "buffer.h"
#include "cmsis_os.h"

#define PHY_BUFFER_SIZE 2048
#define PHY_CC_DEPTH 7
#define PHY_CC_MEMORY_COUNT (1 << (PHY_CC_DEPTH - 1))

typedef enum {
  PHY_RX_STATUS_RESET,
  PHY_RX_STATUS_IDLE,
  PHY_RX_STATUS_RCV_HEADER,
  PHY_RX_STATUS_RCV_PAYLOAD
} PHY_RX_StatusTypeDef;

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
  uint16_t WriteCount;
  uint16_t ReadCount;
  uint16_t Length;
  PHY_RX_StatusTypeDef Status;
} PHY_RX_HandleTypeDef;

typedef struct {
  uint8_t DUMMY;

} PHY_TX_HandleTypeDef;

typedef struct {
  PHY_CC_HandleTypeDef CC;
  PHY_RS_HandleTypeDef RS;
  PHY_RX_HandleTypeDef RX;
  PHY_TX_HandleTypeDef TX;
} PHY_HandleTypeDef;

extern PHY_HandleTypeDef PHY;

void PHY_Init(void);

void PHY_RX_DataReset(void);
uint8_t PHY_RX_DataInput(uint8_t Data);

/* Thread specific definitions */
extern osThreadId PHY_ThreadId;
void PHY_Thread(const void *argument);

/* APIs */
uint8_t PHY_API_DataReceived(uint8_t Data);

#endif // __PHY
