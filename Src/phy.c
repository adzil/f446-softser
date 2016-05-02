#include <phy.h>

PHY_HandleTypeDef PHY;

uint8_t PHY_RX_MEM[PHY_RX_BUFFER_SIZE];
uint8_t PHY_RX_RcvBufferMEM[PHY_RCV_BUFFER_SIZE];
uint8_t PHY_RX_RcvDecodeBufferMEM[PHY_RCV_DECODE_BUFFER_SIZE];

/* OS Thread Handle */
osThreadId PHY_RX_ThreadId;
osThreadDef(PHY_RX_Thread, osPriorityNormal, 1, 0);

//osThreadId PHY_TX_ThreadId;
//osThreadDef(PHY_TX_Thread, osPriorityNormal, 1, 0);

void PHY_Init(void) {
  // Initialize forward error correctors
  FEC_Init();
  // Initialize buffer
  BUF_Init(&PHY.RX.Buffer, PHY_RX_MEM, 1, PHY_RX_BUFFER_SIZE);
  PHY.RX.RcvBuffer = PHY_RX_RcvBufferMEM;
  PHY.RX.RcvDecodeBuffer = PHY_RX_RcvDecodeBufferMEM;
  // Thread initialization
  PHY_RX_ThreadId = osThreadCreate(osThread(PHY_RX_Thread), NULL);
  //PHY_TX_ThreadId = osThreadCreate(osThread(PHY_TX_Thread), NULL);
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

void PHY_TX_CreateHeader(uint8_t *Header, uint16_t Length) {
  uint16_t Sum;

  // Add length
  *((uint16_t *) Header) = __REV16(Length);
  // Calculate checksum
  Sum = CRC_Checksum(Header, 2);
  // Add checksum
  *((uint16_t *) (Header + 2)) = __REV16(Sum);
}

PHY_Status PHY_TX_EncodeData(uint8_t *Output, uint8_t *Input, uint16_t Length) {
  uint8_t *RSData;
  uint16_t RSLen;

  // Create RS data
  RSLen = FEC_RS_BUFFER_LEN(Length);
  RSData = MEM_Alloc(RSLen);
  if (!RSData) {
    return PHY_MEM_NOT_AVAIL;
  }
  FEC_RS_Encode(RSData, Input, Length);
  // Create CC data
  FEC_CC_Encode(Output, RSData, RSLen);
  // Free memory
  MEM_Free(RSData);

  return PHY_OK;
}

PHY_Status PHY_API_SendStart(uint8_t *Data, uint16_t Length) {
  uint8_t *RawHeader;
  uint8_t *Header;
  uint8_t *Payload;
  uint16_t HeaderLen;
  uint16_t PayloadLen;

  // Create header
  RawHeader = MEM_Alloc(PHY_HEADER_LEN);
  if (!RawHeader) return PHY_MEM_NOT_AVAIL;
  PHY_TX_CreateHeader(RawHeader, Length);
  // Create encoded header
  HeaderLen = FEC_CC_BUFFER_LEN(FEC_RS_BUFFER_LEN(PHY_HEADER_LEN));
  Header = MEM_Alloc(HeaderLen);
  if (!Header) {
    MEM_Free(RawHeader);
    return PHY_MEM_NOT_AVAIL;
  }
  if (PHY_TX_EncodeData(Header, RawHeader, PHY_HEADER_LEN)) {
    MEM_Free(RawHeader);
    MEM_Free(Header);
    return PHY_MEM_NOT_AVAIL;
  }
  MEM_Free(RawHeader);
  // Create encoded data
  PayloadLen = FEC_CC_BUFFER_LEN(FEC_RS_BUFFER_LEN(Length));
  Payload = MEM_Alloc(PayloadLen);
  if (!Payload) {
    MEM_Free(Header);
    return PHY_MEM_NOT_AVAIL;
  }
  if (PHY_TX_EncodeData(Payload, Data, Length)) {
    MEM_Free(Header);
    MEM_Free(Payload);
    return PHY_MEM_NOT_AVAIL;
  }

  // Retry send on error
  while (DRV_API_SendStart(Header, HeaderLen, Payload, PayloadLen)) {
    osThreadYield();
  }

  // Free memory
  MEM_Free(Header);
  MEM_Free(Payload);

  return PHY_OK;
}

uint8_t PHY_API_DataReceived(uint8_t Data) {
  uint8_t *Buffer;

  if (PHY.RX.TotalLen && PHY.RX.ReceiveLen >= PHY.RX.TotalLen)
    return 1;
  Buffer = BUF_Write(&PHY.RX.Buffer);
  if (!Buffer) {
    // Oops! buffer is full
    PHY_RX_SetStatus(PHY_RX_STATUS_RESET);
    return 1;
  }
  *Buffer = Data;
  PHY.RX.ReceiveLen++;
  return 0;
}

// Read data from Ring buffer and CC decode it
void PHY_RX_Handler(void) {
  uint8_t *Buffer;
  
  // Checks for buffer input
  Buffer = BUF_Read(&PHY.RX.Buffer);
  // Discard handler on empty buffer
  if (!Buffer) {
    // Give chance for another thread first
    osThreadYield();
    return;
  }

  // Check for last status
  if (PHY.RX.Status == PHY_RX_STATUS_RESET) {
    // Initialize for Header retrieval
    FEC_CC_DecodeInit(PHY.RX.RcvBuffer, FEC_RS_BUFFER_LEN(PHY_HEADER_LEN));
    PHY_RX_SetStatus(PHY_RX_STATUS_PROCESS_HEADER);
  }

  FEC_CC_DecodeInput(*Buffer);
  
  if (FEC_CC_DecodeComplete() == FEC_OK) {
    switch (PHY.RX.Status) {
      case PHY_RX_STATUS_PROCESS_HEADER:
        FEC_RS_Decode(PHY.RX.RcvDecodeBuffer, PHY.RX.RcvBuffer, PHY_HEADER_LEN);
        if (CRC_Checksum(PHY.RX.RcvDecodeBuffer, PHY_HEADER_LEN)) {
          PHY_RX_SetStatus(PHY_RX_STATUS_RESET);
          return;
        }

        PHY.RX.PayloadLen = __REV16(*((uint16_t *)PHY.RX.RcvDecodeBuffer));
        PHY.RX.TotalLen =
            FEC_CC_BUFFER_LEN(FEC_RS_BUFFER_LEN(PHY_HEADER_LEN)) +
            FEC_CC_BUFFER_LEN(FEC_RS_BUFFER_LEN(PHY.RX.PayloadLen));

        FEC_CC_DecodeInit(PHY.RX.RcvBuffer,
                          FEC_RS_BUFFER_LEN(PHY.RX.PayloadLen));
        PHY_RX_SetStatus(PHY_RX_STATUS_PROCESS_PAYLOAD);
        break;

      case PHY_RX_STATUS_PROCESS_PAYLOAD:
        FEC_RS_Decode(PHY.RX.RcvDecodeBuffer, PHY.RX.RcvBuffer,
                      PHY.RX.PayloadLen);
        HAL_UART_Transmit(&huart2, PHY.RX.RcvDecodeBuffer,
                          PHY.RX.PayloadLen, 0xff);
        PHY_RX_SetStatus(PHY_RX_STATUS_RESET);
        break;

      default:
        break;
    }
  }
}

void PHY_RX_Thread(const void *argument) {
  // Endless loop
  while (1) {
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
  }
}
