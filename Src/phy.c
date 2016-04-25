#include <phy.h>

PHY_HandleTypeDef PHY;
uint16_t PHY_CC_Distance_MEM[PHY_CC_MEMORY_COUNT];
uint16_t PHY_CC_LastDistance_MEM[PHY_CC_MEMORY_COUNT];
uint32_t PHY_CC_Data_MEM[PHY_CC_MEMORY_COUNT];
uint32_t PHY_CC_LastData_MEM[PHY_CC_MEMORY_COUNT];

uint8_t PHY_RX_MEM[PHY_BUFFER_SIZE];
uint8_t PHY_TX_MEM[PHY_BUFFER_SIZE];

/* Function prototypes */
uint8_t PHY_CC_Output(uint8_t input);
uint8_t PHY_CC_Popcnt(uint8_t input);

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
  PHY.CC.Distance = PHY_CC_Distance_MEM;
  PHY.CC.LastDistance = PHY_CC_LastDistance_MEM;
  PHY.CC.Data = PHY_CC_Data_MEM;
  PHY.CC.LastData = PHY_CC_LastData_MEM;

  BUF_Init(&PHY.RX.Buffer, PHY_RX_MEM, 1, PHY_BUFFER_SIZE);
  BUF_Init(&PHY.TX.Buffer, PHY_TX_MEM, 1, PHY_BUFFER_SIZE);
}

void PHY_CC_EncodeReset(void) {
  // Reset the state to 0
  PHY.CC.State = 0;
}

uint8_t PHY_CC_Encode(uint8_t Input) {
  // Shift the State register
  PHY.CC.State >>= 1;
  // Shift the new data
  if (Input) PHY.CC.State |= (1 << (PHY_CC_DEPTH - 1));
  // Return the output state
  return PHY_CC_Output(PHY.CC.State);
}

void PHY_CC_DecodeReset(void) {
  int i;

  // Reset all distance and last data
  for (i = 0; i < PHY_CC_MEMORY_COUNT; i++) {
    PHY.CC.Distance[i] = 0xffff;
    PHY.CC.LastDistance[i] = 0xffff;
    PHY.CC.LastData[i] = 0;
  }

  // Define the first state
  PHY.CC.LastDistance[0] = 0;
}

void PHY_CC_DecodeInput(uint8_t Input) {
  uint16_t MinDistance, NextDistance0, NextDistance1;
  uint8_t NextPos0, NextPos1, MinId;
  void *TempPtr;
  int i;

  // Reset the minimum distance
  MinDistance = 0xffff;
  MinId = 0;

  for (i = 0; i < PHY_CC_MEMORY_COUNT; i++) {
    // Discard nonexistent distance paths
    if (PHY.CC.LastDistance[i] == 0xffff) continue;
    // Get hamming distance
    NextDistance0 = PHY_CC_Popcnt(PHY_CC_Output(i) ^ Input);
    NextDistance1 = PHY_CC_Popcnt(
        PHY_CC_Output(PHY_CC_MEMORY_COUNT | i) ^ Input);
    // Get next output location
    NextPos0 = i >> 1;
    NextPos1 = NextPos0 | (1 << (PHY_CC_DEPTH - 2));
    // Replace next state distance
    if (PHY.CC.Distance[NextPos0] > PHY.CC.LastDistance[i] + NextDistance0) {
      PHY.CC.Distance[NextPos0] = PHY.CC.LastDistance[i] + NextDistance0;
      PHY.CC.Data[NextPos0] = PHY.CC.LastData[i] << 1;
    }
    if (PHY.CC.Distance[NextPos1] > PHY.CC.LastDistance[i] + NextDistance1) {
      PHY.CC.Distance[NextPos1] = PHY.CC.LastDistance[i] + NextDistance1;
      PHY.CC.Data[NextPos1] = (PHY.CC.LastData[i] << 1) | 1;
    }
    // Reset the distance
    PHY.CC.LastDistance[i] = 0xff;
    // Get minimum distance
    if (MinDistance > PHY.CC.Distance[NextPos0]) {
      MinDistance = PHY.CC.Distance[NextPos0];
      MinId = NextPos0;
    }
    if (MinDistance > PHY.CC.Distance[NextPos1]) {
      MinDistance = PHY.CC.Distance[NextPos1];
      MinId = NextPos1;
    }
  }
  // Assign minimum Id
  PHY.CC.MinId = MinId;
  // Register write back
  TempPtr = PHY.CC.LastDistance;
  PHY.CC.LastDistance = PHY.CC.Distance;
  PHY.CC.Distance = (uint16_t *) TempPtr;
  TempPtr = PHY.CC.LastData;
  PHY.CC.LastData = PHY.CC.Data;
  PHY.CC.Data = (uint32_t *) TempPtr;
}

void PHY_RX_Reset(void) {
  BUF_Flush(&PHY.RX.Buffer);
  PHY.RX.WriteCount = 0;
  PHY.RX.ReadCount = 0;
  PHY.RX.Length = 0;
}

uint8_t PHY_RX_Write(uint8_t Data) {
  uint8_t *Buffer;

  if (PHY.RX.Length && PHY.RX.WriteCount > PHY.RX.Length) return 1;
  Buffer = BUF_Write(&PHY.RX.Buffer);
  if (!Buffer) return 1;
  *Buffer = Data;
  PHY.RX.WriteCount++;
  return 0;
}

uint8_t PHY_RX_Read(uint8_t *Data) {
  uint8_t *Buffer;

  if (PHY.RX.Length && PHY.RX.ReadCount > PHY.RX.Length) return 1;
  Buffer = BUF_Read(&PHY.RX.Buffer);
  if (!Buffer) return 1;
  *Data = *Buffer;
  PHY.RX.ReadCount++;
  return 0;
}