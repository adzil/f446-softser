#include <fec.h>

FEC_CC_HandleTypeDef CC;
uint16_t FEC_CC_Distance_MEM[FEC_CC_MEMORY_LENGTH];
uint16_t FEC_CC_LastDistance_MEM[FEC_CC_MEMORY_LENGTH];
uint32_t FEC_CC_Data_MEM[FEC_CC_MEMORY_LENGTH];
uint32_t FEC_CC_LastData_MEM[FEC_CC_MEMORY_LENGTH];

const uint8_t FEC_CC_OUTPUT_TABLE[] = {
    0xf0,0xf0,0x0f,0x0f,0x3c,0x3c,0xc3,0xc3,0xf0,0xf0,0x0f,0x0f,0x3c,0x3c,0xc3,
    0xc3,0x0f,0x0f,0xf0,0xf0,0xc3,0xc3,0x3c,0x3c,0x0f,0x0f,0xf0,0xf0,0xc3,0xc3,
    0x3c,0x3c,0x0f,0x0f,0xf0,0xf0,0xc3,0xc3,0x3c,0x3c,0x0f,0x0f,0xf0,0xf0,0xc3,
    0xc3,0x3c,0x3c,0xf0,0xf0,0x0f,0x0f,0x3c,0x3c,0xc3,0xc3,0xf0,0xf0,0x0f,0x0f,
    0x3c,0x3c,0xc3,0xc3
};

_inline_ uint8_t FEC_CC_Output(uint8_t input) {
  return (input & FEC_CC_MEMORY_LENGTH) ?
    FEC_CC_OUTPUT_TABLE[input & (FEC_CC_MEMORY_LENGTH - 1)] >> 4 :
    FEC_CC_OUTPUT_TABLE[input] & 0xf;
}

_inline_ uint8_t FEC_CC_Popcnt(uint8_t input) {
  input = (input & 0x5) + ((input >> 1) & 0x5);
  input = (input & 0x3) + ((input >> 2) & 0x3);

  return input;
}

void FEC_CC_Encode(uint8_t *Output, uint8_t *Input, uint16_t Length) {
  uint8_t Byte;
  int i;

  while (Length--) {
    Byte = *(Input++);
    for (i = 8; i > 0; i--) {
      // Shift the State register
      CC.State >>= 1;

      // Shift the new data
      if (Byte & 0x80) CC.State |= (1 << (FEC_CC_DEPTH - 1));
      Byte <<= 1;

      if (i & 1) {
        *(Output++) |= FEC_CC_Output(CC.State);
      } else {
        *Output = FEC_CC_Output(CC.State) << 4;
      }
    }
  }

  // Send stop bits
  for (i = 6; i > 0; i--) {
    // Shift the State register
    CC.State >>= 1;

    if (i & 1) {
      *(Output++) |= FEC_CC_Output(CC.State);
    } else {
      *Output = FEC_CC_Output(CC.State) << 4;
    }
  }
}

void FEC_Init(void) {
  CC.Distance = FEC_CC_Distance_MEM;
  CC.LastDistance = FEC_CC_LastDistance_MEM;
  CC.Data = FEC_CC_Data_MEM;
  CC.LastData = FEC_CC_LastData_MEM;

  // Initialize RS
  INIT_RS(4, 0x13, 1, 1, 8);
}

void FEC_BYPASS_DecodeInit(uint8_t *Output, uint16_t Length) {
  CC.Output = Output;
  CC.OutputLength = Length;
}

void FEC_BYPASS_DecodeInput(uint8_t Input) {
  if (!CC.Output || CC.OutputLength == 0xffff) return;

  *(CC.Output++) = Input;

  if (!--CC.OutputLength)
    CC.OutputLength = 0xffff;
}

void FEC_CC_DecodeInit(uint8_t *Output, uint16_t Length) {
  int i;

  // Reset all distance and last data
  for (i = 0; i < FEC_CC_MEMORY_LENGTH; i++) {
    CC.Distance[i] = 0xffff;
    CC.LastDistance[i] = 0xffff;
    CC.LastData[i] = 0;
  }

  // Define the first state
  CC.LastDistance[0] = 0;
  // Reset counter
  CC.Counter = FEC_CC_DECODE_RESET_COUNT;
  // Reset output data
  CC.Output = Output;
  CC.OutputLength = FEC_CC_BUFFER_LEN(Length);
}

void FEC_CC_DecodeInputStn(uint8_t Input) {
  uint16_t MinDistance, NextDistance0, NextDistance1;
  uint8_t NextPos0, NextPos1, MinId;
  void *TempPtr;
  int i;

  // Reset the minimum distance
  MinDistance = 0xffff;
  MinId = 0;

  for (i = 0; i < FEC_CC_MEMORY_LENGTH; i++) {
    // Discard nonexistent distance paths
    if (CC.LastDistance[i] == 0xffff) continue;
    // Get hamming distance
    NextDistance0 = FEC_CC_Popcnt(FEC_CC_Output(i) ^ Input);
    NextDistance1 = FEC_CC_Popcnt(
        FEC_CC_Output(FEC_CC_MEMORY_LENGTH | i) ^ Input);
    // Get next output location
    NextPos0 = i >> 1;
    NextPos1 = NextPos0 | (1 << (FEC_CC_DEPTH - 2));
    // Replace next state distance
    if (CC.Distance[NextPos0] > CC.LastDistance[i] + NextDistance0) {
      CC.Distance[NextPos0] = CC.LastDistance[i] + NextDistance0;
      CC.Data[NextPos0] = CC.LastData[i] << 1;
    }
    if (CC.Distance[NextPos1] > CC.LastDistance[i] + NextDistance1) {
      CC.Distance[NextPos1] = CC.LastDistance[i] + NextDistance1;
      CC.Data[NextPos1] = (CC.LastData[i] << 1) | 1;
    }
    // Reset the distance
    CC.LastDistance[i] = 0xffff;
    // Get minimum distance
    if (MinDistance > CC.Distance[NextPos0]) {
      MinDistance = CC.Distance[NextPos0];
      MinId = NextPos0;
    }
    if (MinDistance > CC.Distance[NextPos1]) {
      MinDistance = CC.Distance[NextPos1];
      MinId = NextPos1;
    }
  }
  // Assign minimum Id
  CC.MinId = MinId;
  // Register write back
  TempPtr = CC.LastDistance;
  CC.LastDistance = CC.Distance;
  CC.Distance = (uint16_t *) TempPtr;
  TempPtr = CC.LastData;
  CC.LastData = CC.Data;
  CC.Data = (uint32_t *) TempPtr;
  // Decrement counter
  CC.Counter--;
}

void FEC_CC_DecodeInput(uint8_t Input) {
  uint32_t PopData;

  if (!CC.Output || CC.OutputLength == 0xffff) return;

  FEC_CC_DecodeInputStn(Input >> 4);
  FEC_CC_DecodeInputStn(Input & 0xf);

  if(!CC.Counter) {
    CC.Counter = FEC_CC_DECODE_CONTINUE_COUNT;
    *(CC.Output++) = CC.LastData[CC.MinId] >> 24;
  }

  // All input has been consumed
  if (!--CC.OutputLength) {
    CC.Counter += 6;
    PopData = CC.LastData[CC.MinId] >> 6;

    if (CC.Counter <= 8)
      *(CC.Output++) = (PopData >> 16) & 0xff;
    if (CC.Counter <= 16)
      *(CC.Output++) = (PopData >> 8) & 0xff;
    if (CC.Counter <= 24)
      *(CC.Output++) = PopData & 0xff;

    // Done CC
    CC.OutputLength = 0xffff;
  }
}

FEC_Status FEC_CC_DecodeComplete(void) {
  if (CC.OutputLength == 0xffff)
    return FEC_OK;
  else
    return FEC_ERROR;
}

// RS Utility with padding and interleaving
void FEC_RS_Encode(uint8_t *OutPtr, uint8_t *InPtr, int Length) {
  int id, datacount, rscount, incount, outcount, itercount;

  Length = FEC_RS_MULT(Length);
  // Constant property
  const int EncodedLen = FEC_RS_ENCODE_LEN(Length);
  const int Depth = __CEIL_DIV(Length, 7);
  const int PadDataLen = Length % 7;
  const int PadLen = (PadDataLen + 8) * Depth;

  // Change input and output to nibble type
  NibbleTypeDef *Out = (NibbleTypeDef *) OutPtr;
  NibbleTypeDef *In = (NibbleTypeDef *) InPtr;
  // Buffer for single RS encoding
  uint8_t Data[15];
  // Get RS loop
  rscount = Depth;
  incount = 0;
  itercount = 0;

  while (rscount-- > 0) {
    // Empty the data buffer
    memset(Data, 0, 15);
    // Check for data count
    if (rscount == 0 && PadDataLen != 0) {
      datacount = PadDataLen;
    } else {
      datacount = 7;
    }
    // Reload the data buffer
    for (id = 0; id < datacount; id++) {
      Data[id] = NibbleRead(In, incount++);
    }
    // Do the RS encoding
    ENCODE_RS(Data, Data + 7);

    // Put the data on the output buffer
    outcount = 0;
    for (id = itercount++; id < EncodedLen;) {
      if (rscount == 0 && outcount == datacount) {
        outcount = 7;
      }
      NibbleWrite(Out, id, Data[outcount++]);
      // Update the id
      if (id < PadLen || PadDataLen == 0) {
        id += Depth;
      } else {
        id += Depth - 1;
      }
    }
  }
}

// RS Utility with padding and interleaving
void FEC_RS_Decode(uint8_t *OutPtr, uint8_t *InPtr, int Length) {
  int id, rscount, incount, outcount, itercount;

  // Modify length size
  Length = FEC_RS_MULT(Length);
  // Constant property
  const int EncodedLen = FEC_RS_ENCODE_LEN(Length);
  const int Depth = __CEIL_DIV(Length, 7);
  const int PadDataLen = Length % 7;
  const int PadLen = (PadDataLen + 8) * Depth;

  // Change input and output to nibble type
  NibbleTypeDef *Out = (NibbleTypeDef *) OutPtr;
  NibbleTypeDef *In = (NibbleTypeDef *) InPtr;
  // Buffer for single RS encoding
  uint8_t Data[15];
  // Get RS loop
  rscount = Depth;
  outcount = 0;
  itercount = 0;

  while (rscount-- > 0) {
    // Empty the data buffer
    memset(Data, 0, 15);
    // Reload the data buffer
    incount = 0;
    for (id = itercount++; id < EncodedLen;) {
      // Align zeroes
      if (rscount == 0 && PadDataLen != 0) {
        if (incount == PadDataLen)
          incount = 7;
        else if (incount >= 15)
          break;
      }
      Data[incount++] = NibbleRead(In, id);
      // Update the id
      if (id < PadLen || PadDataLen == 0) {
        id += Depth;
      } else {
        id += Depth - 1;
      }
    }
    // Do the RS encoding
    DECODE_RS(Data, NULL, 0);
    // Put the data on the output buffer
    for (id = 0; id < 7; id++) {
      NibbleWrite(Out, outcount++, Data[id]);
    }
  }
}
