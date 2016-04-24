/**
 * Physical Layer for VLC
 */

#ifndef __PHY
#define __PHY

#include <inttypes.h>
#include "buffer.h"

#define PHY_BUFFER_SIZE 2048
#define PHY_CC_DEPTH 7
#define PHY_CC_MEMORY_COUNT (1 << (PHY_CC_DEPTH - 1))

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
  BUF_HandleTypeDef RXBuf;
  BUF_HandleTypeDef TXBuf;
}PHY_Data_HandleTypeDef;

typedef struct {
  PHY_CC_HandleTypeDef CC;
  PHY_RS_HandleTypeDef RS;
  PHY_Data_HandleTypeDef Data;
} PHY_HandleTypeDef;

extern PHY_HandleTypeDef PHY;

void PHY_Init(void);
void PHY_Data_Write(uint8_t Data);
uint8_t *PHY_Data_Read(void);

#endif // __PHY
