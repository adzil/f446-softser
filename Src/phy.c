#include <phy.h>

PHY_HandleTypeDef PHY;

uint8_t PHY_RX_MEM[PHY_BUFFER_SIZE];

/* OS Thread Handle */
osThreadId PHY_RX_ThreadId;
osThreadDef(PHY_RX_Thread, osPriorityNormal, 1, 0);

//osThreadId PHY_TX_ThreadId;
//osThreadDef(PHY_TX_Thread, osPriorityNormal, 1, 0);

void PHY_Init(void) {
  FEC_Init();

  // Initialize RS
  INIT_RS(4, 0x13, 1, 1, 8);

  BUF_Init(&PHY.RX.Buffer, PHY_RX_MEM, 1, PHY_BUFFER_SIZE);
  // Thread initialization
  PHY_RX_ThreadId = osThreadCreate(osThread(PHY_RX_Thread), NULL);
  //PHY_TX_ThreadId = osThreadCreate(osThread(PHY_TX_Thread), NULL);
}

uint8_t PHY_RX_SetProcess(uint16_t DataLen) {
  uint8_t *Data;
  uint16_t EncodeLen;

  EncodeLen = PHY_CC_ENCODE_LEN(DataLen);

  Data = MEM_Alloc(DataLen);
  if (!Data) {
    PHY_RX_SetStatus(PHY_RX_STATUS_RESET);
    return 0;
  }

  PHY.RX.Process.Data = Data;
  PHY.RX.Process.ProcessData = Data;
  PHY.RX.Process.Len = EncodeLen;
  PHY.RX.Process.ProcessLen = EncodeLen;

  return 1;
}

void PHY_RX_ClearProcess(void) {
  if (PHY.RX.Process.ProcessData) {
    MEM_Free(PHY.RX.Process.ProcessData);
  }
  PHY.RX.Process.Data = NULL;
  PHY.RX.Process.ProcessData = NULL;
  PHY.RX.Process.Len = 0;
  PHY.RX.Process.ProcessLen = 0;
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

uint8_t PHY_API_SendStart(uint8_t *Data, uint16_t DataLen) {
  uint16_t Sum;
  uint8_t *Header;
  uint16_t HeaderLen;
  uint8_t *Payload;
  uint16_t PayloadLen;

  // Packet creator
  HeaderLen = PHY_HEADER_LEN;
  Header = MEM_Alloc(PHY_HEADER_LEN);
  *((uint16_t *)Header) = __REV16(DataLen);
  // Do crc checksum
  Sum = CRC_Checksum(Header, PHY_HEADER_LEN - 2);
  *((uint16_t *)(Header + 2)) = __REV16(Sum);
  // Encode
  Header = PHY_CC_Encode(Header, &HeaderLen);
  // Encode payload
  PayloadLen = DataLen;
  Payload = PHY_CC_Encode(Data, &PayloadLen);

  DRV_API_SendStart(Header, HeaderLen, Payload, PayloadLen);
  
  MEM_Free(Header);
  MEM_Free(Payload);
  return 0;
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
  uint32_t Pop;
  uint8_t Output;
  
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
    //PHY_CC_DecodeReset();
    // Set status to Header retrieval
    if (!PHY_RX_SetProcess(PHY_HEADER_LEN)) return;
    PHY_RX_SetStatus(PHY_RX_STATUS_PROCESS_HEADER);
  }
  
  PHY_CC_DecodeInput(*Buffer >> 4);
  PHY_CC_DecodeInput(*Buffer & 0xf);
  // Check for output
  if (PHY_CC_DecodeOutput(&Output)) {
    *(PHY.RX.Process.Data++) = Output;
  }

  if (!--PHY.RX.Process.Len) {
    // Get leftover data
    Pop = PHY_CC_DecodePop();
    *(PHY.RX.Process.Data++) = (Pop >> 16) & 0xff;
    *(PHY.RX.Process.Data++) = (Pop >> 8) & 0xff;
    *(PHY.RX.Process.Data++) = Pop & 0xff;
    PHY_CC_DecodeReset();

    switch (PHY.RX.Status) {
      case PHY_RX_STATUS_PROCESS_HEADER:
        // Process header
        // Do CRC checking
        if (CRC_Checksum(PHY.RX.Process.ProcessData,
                         PHY.RX.Process.ProcessLen)) {
          assert_user(0, "CRC Check failed");
          PHY_RX_SetStatus(PHY_RX_STATUS_RESET);
          return;
        }
        PHY.RX.PayloadLen = __REV16(*((uint16_t *)PHY.RX.Process.ProcessData));
        PHY.RX.TotalLen = PHY_CC_ENCODE_LEN(PHY.RX.PayloadLen) + PHY_HEADER_CC_LEN;
        // Reserve for payload
        PHY_RX_ClearProcess();
        if (!PHY_RX_SetProcess(PHY.RX.PayloadLen)) return;
        PHY_RX_SetStatus(PHY_RX_STATUS_PROCESS_PAYLOAD);
        break;

      case PHY_RX_STATUS_PROCESS_PAYLOAD:
        HAL_UART_Transmit(&huart2, PHY.RX.Process.ProcessData,
                          PHY.RX.PayloadLen, 0xff);
        PHY_RX_ClearProcess();
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
