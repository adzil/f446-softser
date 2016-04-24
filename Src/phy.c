#include <phy.h>
#include "phy.h"

PHY_HandleTypeDef hPhy;
PHY_CC_HandleTypeDef hPhyCC;

const uint8_t PHY_CC_OUTPUT_TABLE[] = {
    0xf0,0xf0,0x0f,0x0f,0x3c,0x3c,0xc3,0xc3,0xf0,0xf0,0x0f,0x0f,0x3c,0x3c,0xc3,
    0xc3,0x0f,0x0f,0xf0,0xf0,0xc3,0xc3,0x3c,0x3c,0x0f,0x0f,0xf0,0xf0,0xc3,0xc3,
    0x3c,0x3c,0x0f,0x0f,0xf0,0xf0,0xc3,0xc3,0x3c,0x3c,0x0f,0x0f,0xf0,0xf0,0xc3,
    0xc3,0x3c,0x3c,0xf0,0xf0,0x0f,0x0f,0x3c,0x3c,0xc3,0xc3,0xf0,0xf0,0x0f,0x0f,
    0x3c,0x3c,0xc3,0xc3
};

inline uint8_t PHY_CC_Output(uint8_t input) {
  return (input & PHY_CC_MEMORY_COUNT) ?
         PHY_CC_OUTPUT_TABLE[input & (PHY_CC_MEMORY_COUNT - 1)] >> 4 :
         PHY_CC_OUTPUT_TABLE[input] & 0xf;
}

inline uint8_t PHY_CC_Popcnt(uint8_t input) {
  input = (input & 0x5) + ((input >> 1) & 0x5);
  input = (input & 0x3) + ((input >> 2) & 0x3);

  return input;
}

void PHY_Init(void) {
  hPhy.CC = &hPhyCC;
}

void PHY_CC_EncodeReset(PHY_HandleTypeDef *Handle) {
  PHY_CC_HandleTypeDef *CC = Handle->CC;

  // Reset the state to 0
  CC->State = 0;
}

uint8_t PHY_CC_Encode(PHY_HandleTypeDef *Handle, uint8_t Input) {
  PHY_CC_HandleTypeDef *CC = Handle->CC;

  // Shift the State register
  CC->State >>= 1;
  // Shift the new data
  if (Input) CC->State |= (1 << (PHY_CC_DEPTH - 1));
  // Return the output state
  return PHY_CC_Output(CC->State);
}

void PHY_CC_DecodeReset(PHY_HandleTypeDef *Handle) {
  PHY_CC_HandleTypeDef *CC = Handle->CC;
  int i;

  // Reset all distance and last data
  for (i = 0; i < PHY_CC_MEMORY_COUNT; i++) {
    CC->Distance[i] = -1;
    CC->LastDistance[i] = -1;
    CC->LastData[i] = 0;
  }

  // Define the first state
  CC->LastDistance[0] = 0;
}

uint8_t PHY_CC_DecodeInput(PHY_HandleTypeDef *Handle, uint8_t Input) {
  PHY_CC_HandleTypeDef *CC = Handle->CC;
  uint16_t MinDistance, NextDistance0, NextDistance1;
  uint8_t NextPos0, NextPos1, MinId;
  void *TempPtr;
  int i;

  // Reset the minimum distance
  MinDistance = -1;
  MinId = 0;

  for (i = 0; i < PHY_CC_MEMORY_COUNT; i++) {
    // Discard nonexistent distance paths
    if (CC->LastDistance[i] == -1) continue;
    // Get hamming distance
    NextDistance0 = PHY_CC_Popcnt(PHY_CC_Output(i) ^ Input);
    NextDistance1 = PHY_CC_Popcnt(
        PHY_CC_Output(PHY_CC_MEMORY_COUNT | i) ^ Input);
    // Get next output location
    NextPos0 = i >> 1;
    NextPos1 = NextPos0 | (1 << (PHY_CC_DEPTH - 2));
    // Replace next state distance
    if (CC->Distance[NextPos0] > CC->LastDistance[i] + NextDistance0) {
      CC->Distance[NextPos0] = CC->LastDistance[i] + NextDistance0;
      CC->Data[NextPos0] = CC->LastData[i] << 1;
    }
    if (CC->Distance[NextPos1] > CC->LastDistance[i] + NextDistance1) {
      CC->Distance[NextPos1] = CC->LastDistance[i] + NextDistance1;
      CC->Data[NextPos1] = (CC->LastData[i] << 1) | 1;
    }
    // Reset the distance
    CC->LastDistance[i] = -1;
    // Get minimum distance
    if (MinDistance > CC->Distance[NextPos0]) {
      MinDistance = CC->Distance[NextPos0];
      MinId = NextPos0;
    }
    if (MinDistance > CC->Distance[NextPos1]) {
      MinDistance = CC->Distance[NextPos1];
      MinId = NextPos1;
    }
  }
  // Assign minimum Id
  CC->MinId = MinId;
  // Register write back
  TempPtr = CC->LastDistance;
  CC->LastDistance = CC->Distance;
  CC->Distance = (uint16_t *) TempPtr;
  TempPtr = CC->LastData;
  CC->LastData = CC->Data;
  CC->Data = (uint32_t *) TempPtr;
}