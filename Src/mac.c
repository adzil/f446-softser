#include "mac.h"

MAC_Instance MAC;
osThreadId MAC_AppDataTid;
osThreadDef(MAC_AppData, osPriorityNormal, 1, 0);
#ifndef MAC_COORDINATOR
osThreadId MAC_AppTimerTid;
osThreadDef(MAC_AppTimer, osPriorityNormal, 1, 0);
#endif

void MAC_AppInit(void) {
  MAC_Init(&MAC, DEV_CONFIG + 1,
#ifdef MAC_COORDINATOR
           MAC_PIB_VPAN_COORDINATOR
#else
           MAC_PIB_VPAN_DEVICE
#endif
  );
#ifdef MAC_COORDINATOR
	MAC.Pib.ShortAdr = (RND_Get() << 8) | RND_Get();
#else
  MAC_AppTimerTid = osThreadCreate(osThread(MAC_AppTimer), NULL);
#endif
  MAC_AppDataTid = osThreadCreate(osThread(MAC_AppData), NULL);
}

void MAC_AppDataReceived(uint8_t *Data, uint16_t Length) {
  MAC_CoreFrameReceived(&MAC, Data, Length);
}

void MAC_AppData(void const *argument) {
  uint8_t *Data;
  size_t Length;
  MAC_Status Ret;
  DRV_TX_StatusTypeDef Status;

  while (1) {
    Ret = MAC_CoreFrameSend(&MAC, &Data, &Length);

    if (Ret != MAC_STATUS_NO_DATA) {
      if (Ret != MAC_STATUS_NO_DELAY)
			  osDelay(100 * ((RND_Get() % 10) + 1));
      PHY_API_SendStart(Data, Length);
      do {
        osThreadYield();
        Status = DRV_TX_GetStatus();
      } while (Status == DRV_TX_STATUS_SYNC || Status == DRV_TX_STATUS_ACTIVE);
      if (Ret == MAC_STATUS_NO_ACK)
			  osDelay(50);
      else
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
