#include "mac.h"

osThreadId MAC_AppTimerTid;
osThreadId MAC_AppDataTid;
MAC_Instance MAC;
uint8_t MAC_DataBuffer[MAC_CONFIG_MAX_FRAME_BUFFER];

osThreadDef(MAC_AppData, osPriorityNormal, 1, 0);
osThreadDef(MAC_AppTimer, osPriorityNormal, 1, 0);

void MAC_AppInit(void) {
  MAC_Init(&MAC, 0x1, MAC_PIB_VPAN_COORDINATOR);
  MAC_AppTimerTid = osThreadCreate(osThread(MAC_AppTimer), NULL);
  MAC_AppDataTid = osThreadCreate(osThread(MAC_AppData), NULL);
}

void MAC_AppDataReceived(uint8_t *Data, uint16_t Length) {
  MAC_CoreFrameReceived(&MAC, Data, Length);
}

void MAC_AppData(void const *argument) {
  size_t Length;

  while (1) {
    MAC_CoreFrameSend(&MAC, MAC_DataBuffer, &Length);

    if (Length) {
      PHY_API_SendStart(MAC_DataBuffer, Length);
    } else {
      osThreadYield();
    }
  }
}

void MAC_AppTimer(void const *argument) {
  while (1) {
    if (MAC.Pib.AssociatedCoord == MAC_PIB_ASSOCIATED_RESET) {
      if (MAC.Pib.CoordExtendedAdr == 0 &&
          MAC.Pib.CoordShortAdr == MAC_CONST_BROADCAST_ADDRESS) {
        MAC_CmdDiscoverRequestSend(&MAC);
      } else {
        MAC_CmdAssocRequestSend(&MAC);
        osDelay(1000);
        MAC_CmdDataRequestSend(&MAC);
      }
    } else {
      MAC_CmdDataRequestSend(&MAC);
    }
    osDelay(2000);
  }
}
