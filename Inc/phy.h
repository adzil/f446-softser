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
  BUF_HandleTypeDef Buffer;
  uint16_t WriteCount;
  uint16_t ReadCount;
  uint16_t Length;
} PHY_RX_HandleTypeDef;

typedef struct {
  uint8_t DUmmy;

} PHY_TX_HandleTypeDef;

typedef struct {
  PHY_CC_HandleTypeDef CC;
  PHY_RS_HandleTypeDef RS;
  PHY_RX_HandleTypeDef RX;
  PHY_TX_HandleTypeDef TX;
} PHY_HandleTypeDef;

extern PHY_HandleTypeDef PHY;

void PHY_Init(void);

void PHY_RX_Reset(void);
uint8_t PHY_RX_Write(uint8_t Data);
uint8_t PHY_RX_Read(uint8_t *Data);

#endif // __PHY
