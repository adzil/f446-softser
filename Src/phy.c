#include <phy.h>

PHY_HandleTypeDef PHY;
uint16_t PHY_CC_Distance_MEM[PHY_CC_MEMORY_LENGTH];
uint16_t PHY_CC_LastDistance_MEM[PHY_CC_MEMORY_LENGTH];
uint32_t PHY_CC_Data_MEM[PHY_CC_MEMORY_LENGTH];
uint32_t PHY_CC_LastData_MEM[PHY_CC_MEMORY_LENGTH];

uint8_t PHY_RX_MEM[PHY_BUFFER_SIZE];

/* OS Thread Handle */
osThreadId PHY_RX_ThreadId;
osThreadDef(PHY_RX_Thread, osPriorityNormal, 1, 0);

osThreadId PHY_TX_ThreadId;
osThreadDef(PHY_TX_Thread, osPriorityNormal, 1, 0);

const uint8_t PHY_CC_OUTPUT_TABLE[] = {
    0xf0,0xf0,0x0f,0x0f,0x3c,0x3c,0xc3,0xc3,0xf0,0xf0,0x0f,0x0f,0x3c,0x3c,0xc3,
    0xc3,0x0f,0x0f,0xf0,0xf0,0xc3,0xc3,0x3c,0x3c,0x0f,0x0f,0xf0,0xf0,0xc3,0xc3,
    0x3c,0x3c,0x0f,0x0f,0xf0,0xf0,0xc3,0xc3,0x3c,0x3c,0x0f,0x0f,0xf0,0xf0,0xc3,
    0xc3,0x3c,0x3c,0xf0,0xf0,0x0f,0x0f,0x3c,0x3c,0xc3,0xc3,0xf0,0xf0,0x0f,0x0f,
    0x3c,0x3c,0xc3,0xc3
};

_inline_ uint8_t PHY_CC_Output(uint8_t input) {
  return (input & PHY_CC_MEMORY_LENGTH) ?
         PHY_CC_OUTPUT_TABLE[input & (PHY_CC_MEMORY_LENGTH - 1)] >> 4 :
         PHY_CC_OUTPUT_TABLE[input] & 0xf;
}

_inline_ uint8_t PHY_CC_Popcnt(uint8_t input) {
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
  // Thread initialization
  PHY_RX_ThreadId = osThreadCreate(osThread(PHY_RX_Thread), NULL);
  PHY_TX_ThreadId = osThreadCreate(osThread(PHY_TX_Thread), NULL);
}

void PHY_RX_Activate(void) {
  // Set signal to activate thread
  osSignalSet(PHY_RX_ThreadId, 0x01);
}

void PHY_TX_Activate(void) {
  // Set signal to activate thread
  osSignalSet(PHY_TX_ThreadId, 0x01);
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
  for (i = 0; i < PHY_CC_MEMORY_LENGTH; i++) {
    PHY.CC.Distance[i] = 0xffff;
    PHY.CC.LastDistance[i] = 0xffff;
    PHY.CC.LastData[i] = 0;
  }

  // Define the first state
  PHY.CC.LastDistance[0] = 0;
  // Reset counter
  PHY.CC.Counter = PHY_CC_DECODE_RESET_COUNT;
}

void PHY_CC_DecodeInput(uint8_t Input) {
  uint16_t MinDistance, NextDistance0, NextDistance1;
  uint8_t NextPos0, NextPos1, MinId;
  void *TempPtr;
  int i;

  // Reset the minimum distance
  MinDistance = 0xffff;
  MinId = 0;

  for (i = 0; i < PHY_CC_MEMORY_LENGTH; i++) {
    // Discard nonexistent distance paths
    if (PHY.CC.LastDistance[i] == 0xffff) continue;
    // Get hamming distance
    NextDistance0 = PHY_CC_Popcnt(PHY_CC_Output(i) ^ Input);
    NextDistance1 = PHY_CC_Popcnt(
        PHY_CC_Output(PHY_CC_MEMORY_LENGTH | i) ^ Input);
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
  // Decrement counter
  PHY.CC.Counter--;
}

uint8_t PHY_CC_DecodeOutput(uint8_t *Data) {
  if (!PHY.CC.Counter) {
    PHY.CC.Counter = PHY_CC_DECODE_CONTINUE_COUNT;
    *Data = PHY.CC.LastDistance[PHY.CC.MinId] >> 24;
    return 1;
  } else {
    return 0;
  }
}

uint32_t PHY_CC_DecodePop(void) {
  return PHY.CC.LastDistance[PHY.CC.MinId] >> 6;
}

// Receiver handler
void PHY_RX_Handler(void) {
  uint8_t *Buffer;
  uint8_t Output;
  uint32_t Pop;
  uint16_t Length;

  // Checks for buffer input
  Buffer = BUF_Read(&PHY.RX.Buffer);
  // Discard handler on empty buffer
  if (!Buffer) return;
  // Jump to process header on reset
  if (PHY.RX.Status == PHY_RX_STATUS_RESET) {
    PHY.RX.ProcessPtr = MEM_Alloc(PHY_HEADER_CC_LEN);
    if (!PHY.RX.ProcessPtr) {
      PHY_RX_SetStatus(PHY_RX_STATUS_RESET);
      return;
    }
    PHY.RX.Process = PHY.RX.ProcessPtr;
    PHY.RX.ProcessLen = PHY_HEADER_CC_LEN;

    PHY_CC_DecodeReset();

    PHY_RX_SetStatus(PHY_RX_STATUS_PROC_HEADER);
  }

  // Do CC Decode
  PHY_CC_DecodeInput(*Buffer >> 4);
  PHY_CC_DecodeInput(*Buffer & 0xf);
  // Check for output
  if (PHY_CC_DecodeOutput(&Output)) {
    *PHY.RX.Process++ = Output;
  }

  // Next state decisions
  if (!--PHY.RX.ProcessLen) {
    // Get leftover data
    Pop = PHY_CC_DecodePop();
    *PHY.RX.Process++ = (Pop >> 16) & 0xff;
    *PHY.RX.Process++ = (Pop >> 8) & 0xff;
    *PHY.RX.Process++ = Pop & 0xff;

    switch (PHY.RX.Status) {
      case PHY_RX_STATUS_PROC_HEADER:
        Length = __REV16(*((uint16_t *)PHY.RX.ProcessPtr));
        MEM_Free(PHY.RX.ProcessPtr);
        PHY.RX.ProcessPtr = MEM_Alloc(Length);
        if (!PHY.RX.ProcessPtr) {
          PHY_RX_SetStatus(PHY_RX_STATUS_RESET);
          break;
        }
        PHY.RX.Process = PHY.RX.ProcessPtr;
        PHY.RX.ProcessLen = Length;
        PHY.RX.TotalLen = Length + PHY_HEADER_LENGTH;

        PHY_RX_SetStatus(PHY_RX_STATUS_PROC_PAYLOAD);
        break;

      case PHY_RX_STATUS_PROC_PAYLOAD:
        HAL_UART_Transmit(&huart2, PHY.RX.ProcessPtr,
                          PHY.RX.Length - PHY_HEADER_LENGTH, 0xff);
        MEM_Free(PHY.RX.ProcessPtr);

        PHY_RX_SetStatus(PHY_RX_STATUS_RESET);
        break;

      default:
        // Keep compiler happy
        break;
    }
  }
}

void PHY_RX_SetStatus(PHY_RX_StatusTypeDef Status) {
  PHY.RX.Status = Status;

  switch (PHY.RX.Status) {
    case PHY_RX_STATUS_RESET:
      BUF_Flush(&PHY.RX.Buffer);
      PHY.RX.ReceiveLen = 0;
      PHY.RX.TotalLen = 0;
      DRV_API_ReceiveComplete();
      break;

    default:
      // Keep compiler happy :)
      break;
  }
}

void PHY_SR_Handler(void) {
  if (PHY.TX.SR.TXComplete) {
    // Free TX data memory
    MEM_Free(PHY.TX.Header);
    MEM_Free(PHY.TX.Payload);
    // Set back TX status
    PHY_TX_SetStatus(PHY_TX_STATUS_RESET);
    // Clear TX complete signal
    PHY.TX.SR.TXComplete = 0;
  }
}

void PHY_TX_SetStatus(PHY_TX_StatusTypeDef Status) {
  PHY.TX.Status = Status;
}

uint8_t PHY_API_SendStart(uint8_t *Data, uint16_t DataLen) {
  // Check if there is ongoing send process
  if (PHY.TX.Status != PHY_TX_STATUS_RESET) return 1;

  PHY_TX_SetStatus(PHY_TX_STATUS_ACTIVE);

  PHY.TX.Header = MEM_Alloc(PHY_HEADER_LENGTH);
  if (!PHY.TX.Header) {
    MEM_Free(Data);
    PHY_TX_SetStatus(PHY_TX_STATUS_RESET);
    return 1;
  }
  *((uint16_t *)PHY.TX.Header) = __REV16(DataLen);
  PHY.TX.Payload = Data;
  DRV_API_SendStart(PHY.TX.Header, PHY_HEADER_LENGTH, Data, DataLen);
  return 0;
}

void PHY_API_SendComplete(void) {
  PHY.TX.SR.TXComplete = 1;
  PHY_TX_Activate();
}

uint8_t PHY_API_DataReceived(uint8_t Data) {
  uint8_t *Buffer;

  if (PHY.RX.TotalLen && PHY.RX.ReceiveLen >= PHY.RX.TotalLen)
    return 1;
  Buffer = BUF_Write(&PHY.RX.Buffer);
  if (!Buffer) return 1;
  *Buffer = Data;
  PHY.RX.ReceiveLen++;
  PHY_RX_Activate();
  return 0;
}

void PHY_RX_Thread(const void *argument) {
  // Endless loop
  while (1) {
    // Wait for activation if RX is sleeping
    if (PHY.RX.Status == PHY_RX_STATUS_RESET) {
      osSignalWait(0x01, osWaitForever);
    }
    // PHY Data handler
    PHY_RX_Handler();
  }
}

void PHY_TX_Thread(const void *argument) {
  // Endless loop
  while (1) {
    // Wait for activation if RX is sleeping
    if (PHY.RX.Status == PHY_TX_STATUS_RESET) {
      osSignalWait(0x01, osWaitForever);
    }
    // PHY Data handler
    PHY_SR_Handler();
  }
}