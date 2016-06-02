#include "mac.h"

MAC_Instance MAC;
uint8_t MAC_DataBuffer[MAC_CONFIG_MAX_FRAME_BUFFER];
osThreadId MAC_AppDataTid;
osThreadDef(MAC_AppData, osPriorityNormal, 1, 0);
#ifndef MAC_COORDINATOR
osThreadId MAC_AppTimerTid;
osThreadDef(MAC_AppTimer, osPriorityNormal, 1, 0);
#endif

void MAC_AppInit(void) {
#ifdef MAC_COORDINATOR
  MAC_Init(&MAC, 0x1, MAC_PIB_VPAN_COORDINATOR);
	MAC.Pib.ShortAdr = 0x1234;
#else
  MAC_Init(&MAC, 0x2, MAC_PIB_VPAN_DEVICE);
  MAC_AppTimerTid = osThreadCreate(osThread(MAC_AppTimer), NULL);
#endif
  MAC_AppDataTid = osThreadCreate(osThread(MAC_AppData), NULL);
}

void MAC_AppDataReceived(uint8_t *Data, uint16_t Length) {
  MAC_CoreFrameReceived(&MAC, Data, Length);
}

void MAC_AppData(void const *argument) {
  size_t Length;
  MAC_Status Ret;

  while (1) {
    Ret = MAC_CoreFrameSend(&MAC, MAC_DataBuffer, &Length);

    if (Length) {
      if (Ret != MAC_STATUS_NO_DELAY)
			  osDelay(20 * ((RND_Get() % 25) + 1));
      PHY_API_SendStart(MAC_DataBuffer, Length);
			osDelay(500);
    } else {
			osThreadYield();
		}
  }
}

#ifndef MAC_COORDINATOR
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
    osDelay(5000);
  }
}
#endif
