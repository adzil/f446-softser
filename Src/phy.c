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

/* Driver API */
uint8_t PHY_API_DataReceived(uint8_t) {

}

void PHY_RX_Thread(const void *argument) {

}