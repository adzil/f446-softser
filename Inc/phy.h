/**
 * Physical Layer for VLC
 */

#ifndef __PHY
#define __PHY

#include <inttypes.h>

#define PHY_CC_DEPTH 7
#define PHY_CC_MEMORY_COUNT (1 << (PHY_CC_DEPTH - 1))

typedef struct {
  uint16_t Distance[PHY_CC_MEMORY_COUNT];
  uint16_t LastDistance[PHY_CC_MEMORY_COUNT];
  uint32_t Data[PHY_CC_MEMORY_COUNT];
  uint32_t LastData[PHY_CC_MEMORY_COUNT];
  uint8_t MinId;
  uint8_t State;
} PHY_CC_HandleTypeDef;

typedef struct {
  uint8_t DUMMY;
} PHY_RS_HandleTypeDef;

typedef struct {
  PHY_CC_HandleTypeDef *CC;
  PHY_RS_HandleTypeDef *RS;
} PHY_HandleTypeDef;

extern PHY_HandleTypeDef hPhy;

#endif // __PHY