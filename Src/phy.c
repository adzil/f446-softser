#include "phy.h"

PHY_HandleTypeDef PHY;

uint8_t PHY_RX_MEM[PHY_RX_BUFFER_SIZE];
uint8_t PHY_RX_RcvBufferMEM[PHY_RCV_BUFFER_SIZE];
uint8_t PHY_RX_RcvDecodeBufferMEM[PHY_RCV_DECODE_BUFFER_SIZE];

//extern char rcv[128];
//extern uint16_t rcvlen;
//extern osThreadId tid_sendSerial;

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

void PHY_TX_EncodeData(uint8_t *Output, uint8_t *Input, uint16_t Length) {
  uint8_t RSData[FEC_RS_BUFFER_LEN(
#ifdef DRV_TEST
	DRV_TEST
#else
	MAC_CONFIG_MAX_FRAME_BUFFER
#endif
	)];
  uint16_t RSLen;

  // Create RS data
  RSLen = FEC_RS_BUFFER_LEN(Length);
  FEC_RS_Encode(RSData, Input, Length);
  // Create CC data
#if defined(DRV_TEST) || defined(MAC_COORDINATOR)
  FEC_CC_Encode(Output, RSData, RSLen);
#else
  memcpy(Output, RSData, RSLen);
#endif
}

PHY_Status PHY_API_SendStart(uint8_t *Data, uint16_t Length) {
  uint8_t EncodedData[
#ifdef DRV_TEST
	PHY_PAYLOAD_ENC_LEN(DRV_TEST)
#else
	PHY_HEADER_ENC_LEN + PHY_PAYLOAD_MAX_ENC_LEN
#endif
	];

#ifndef DRV_TEST
  // Create header
  PHY_TX_CreateHeader(EncodedData, Length);
  // Create encoded header
  PHY_TX_EncodeData(EncodedData, EncodedData, PHY_HEADER_LEN);
#endif
  // Create encoded data
  PHY_TX_EncodeData(EncodedData
#ifndef DRV_TEST
                    + PHY_HEADER_ENC_LEN
#endif
                    , Data, Length);

  // Retry send on error
  while (DRV_API_SendStart(EncodedData,
#ifndef DRV_TEST
                           PHY_HEADER_ENC_LEN +
#endif
                           PHY_PAYLOAD_ENC_LEN(Length))) {
    osThreadYield();
  }

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
#if defined(DRV_TEST)
    FEC_CC_DecodeInit(PHY.RX.RcvBuffer, FEC_RS_BUFFER_LEN(DRV_TEST));
#elif !defined(MAC_COORDINATOR)
    FEC_CC_DecodeInit(PHY.RX.RcvBuffer, FEC_RS_BUFFER_LEN(PHY_HEADER_LEN));
#else
    FEC_BYPASS_DecodeInit(PHY.RX.RcvBuffer, PHY_HEADER_DEC_LEN);
#endif
#ifdef DRV_TEST
    PHY.RX.PayloadLen = DRV_TEST;
    PHY.RX.TotalLen = PHY_PAYLOAD_DEC_LEN(PHY.RX.PayloadLen);
    PHY_RX_SetStatus(PHY_RX_STATUS_PROCESS_PAYLOAD);
#else
    PHY_RX_SetStatus(PHY_RX_STATUS_PROCESS_HEADER);
#endif
  }

#ifndef MAC_COORDINATOR
  FEC_CC_DecodeInput(*Buffer);
#else
  FEC_BYPASS_DecodeInput(*Buffer);
#endif
  
  if (FEC_CC_DecodeComplete() == FEC_OK) {
    switch (PHY.RX.Status) {
      case PHY_RX_STATUS_PROCESS_HEADER:
        FEC_RS_Decode(PHY.RX.RcvDecodeBuffer, PHY.RX.RcvBuffer, PHY_HEADER_LEN);
        if (CRC_Checksum(PHY.RX.RcvDecodeBuffer, PHY_HEADER_LEN)) {
          PHY_RX_SetStatus(PHY_RX_STATUS_RESET);
          return;
        }

        PHY.RX.PayloadLen = __REV16(*((uint16_t *)PHY.RX.RcvDecodeBuffer));
        PHY.RX.TotalLen = PHY_HEADER_DEC_LEN +
                          PHY_PAYLOAD_DEC_LEN(PHY.RX.PayloadLen);
#ifndef MAC_COORDINATOR
        FEC_CC_DecodeInit(PHY.RX.RcvBuffer,
                          FEC_RS_BUFFER_LEN(PHY.RX.PayloadLen));
#else
        FEC_BYPASS_DecodeInit(PHY.RX.RcvBuffer,
                              PHY_PAYLOAD_DEC_LEN(PHY.RX.PayloadLen));
#endif
        PHY_RX_SetStatus(PHY_RX_STATUS_PROCESS_PAYLOAD);
        break;

      case PHY_RX_STATUS_PROCESS_PAYLOAD:
        FEC_RS_Decode(PHY.RX.RcvDecodeBuffer, PHY.RX.RcvBuffer,
                      PHY.RX.PayloadLen);
#ifdef DRV_TEST
        memcpy(TestRcv, PHY.RX.RcvDecodeBuffer, DRV_TEST);
        osSignalSet(tid_DrvTest, 1);
#else
        MAC_AppDataReceived(PHY.RX.RcvDecodeBuffer, PHY.RX.PayloadLen);
#endif
        // Initiate MAC layer payload process
        //MAC_API_DataReceived(PHY.RX.RcvDecodeBuffer, PHY.RX.PayloadLen);
				//memcpy(rcv, PHY.RX.RcvDecodeBuffer, PHY.RX.PayloadLen);
        //rcvlen = PHY.RX.PayloadLen;
			  //HAL_UART_Transmit(&huart2, PHY.RX.RcvDecodeBuffer,
        //                  PHY.RX.PayloadLen, 0xff);
				//osSignalSet(tid_sendSerial, 1);
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
