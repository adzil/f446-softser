/**
 *  PHY - Physical Layer for Visible Light Communication
 */

/* Self includes */
#include <phy.h>

uint8_t PHY_RX_MEM[PHY_RX_BUFFER_SIZE];

/* Thread variables */
osThreadId PHY_RX_ThreadId;
osThreadDef(PHY_RX_Thread, osPriorityNormal, 1, 0);

/* Self handle */
PHY_HandleTypeDef PHY;

/* PHY Initialization */
void PHY_Init(void) {
  // Ring Buffer initialization
  BUF_Init(&PHY.RX.Buffer, PHY_RX_MEM, 1, PHY_RX_BUFFER_SIZE);
  // Thread initialization
  PHY_RX_ThreadId = osThreadCreate(osThread(PHY_RX_Thread), NULL);
}

_inline_ void PHY_RX_ActivateThread(void) {
  osSignalSet(PHY_RX_ThreadId, 0x01);
}

void PHY_RX_SetStatus(PHY_RX_StatusTypeDef Status) {
  PHY.RX.Status = Status;
}

void PHY_RX_Handler(void) {
  uint8_t  *Buffer;

  // Read new data from buffer
  Buffer = BUF_Read(&PHY.RX.Buffer);
  // Currently no more data to read
  if (!Buffer) return;
  if (PHY.RX.Status == PHY_RX_STATUS_RESET) {

  }
}

/* Driver API */
uint8_t PHY_API_DataReceived(uint8_t Data) {
  uint8_t *Buffer;

  if (PHY.RX.TotalLen && PHY.RX.ReceivedLen >= PHY.RX.TotalLen) return 1;
  Buffer = BUF_Write(&PHY.RX.Buffer);
  if (!Buffer) return 1;
  *Buffer = Data;
  PHY.RX.ReceivedLen++;
  PHY_RX_ActivateThread();
}

void PHY_RX_Thread(const void *argument) {
  while(1) {
    if (PHY.RX.Status == PHY_RX_STATUS_RESET) {
      // Wait for activation
      osSignalWait(0x01, osWaitForever);
    }

    PHY_RX_Handler();
  }
}