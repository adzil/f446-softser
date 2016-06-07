#include "mac.h"

MAC_Instance MAC;
osThreadId MAC_AppDataTid;
osThreadDef(MAC_AppData, osPriorityNormal, 1, 0);
#ifndef MAC_COORDINATOR
osThreadId MAC_AppTimerTid;
osThreadDef(MAC_AppTimer, osPriorityNormal, 1, 0);
osThreadId MAC_AppButtonTid;
osThreadDef(MAC_AppButton, osPriorityNormal, 1, 0);
#endif

uint8_t DummyBPM[] = {DEV_CONFIG, 1, 0, 60};
uint8_t DummyTemp[] = {DEV_CONFIG, 2, 3, 0};
uint8_t SendAlert[] = {DEV_CONFIG, 3, 0, 0};

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
  MAC_AppButtonTid = osThreadCreate(osThread(MAC_AppButton), NULL);
#endif
  MAC_AppDataTid = osThreadCreate(osThread(MAC_AppData), NULL);
}

void MAC_AppDataReceived(uint8_t *Data, uint16_t Length) {
  MAC_CoreFrameReceived(&MAC, Data, Length);
}

void MAC_AppData(void const *argument) {
  uint8_t *Data;
	uint32_t Ticks;
  size_t Length;
  MAC_Status Ret;
  DRV_TX_StatusTypeDef Status;

  while (1) {
    Ret = MAC_CoreFrameSend(&MAC, &Data, &Length);

    if (Ret != MAC_STATUS_NO_DATA) {
#ifndef MAC_COORDINATOR
      if (Ret != MAC_STATUS_NO_DELAY)
				//osDelay(500);
			  osDelay(25 * ((RND_Get() % 20) + 1));
#endif
      PHY_API_SendStart(Data, Length);
      do {
        osThreadYield();
        Status = DRV_TX_GetStatus();
      } while (Status == DRV_TX_STATUS_SYNC || Status == DRV_TX_STATUS_ACTIVE  || Status == DRV_TX_STATUS_STOP);
      if (Ret == MAC_STATUS_OK) {
        Ticks = HAL_GetTick();
#ifndef MAC_COORDINATOR
        while(HAL_GetTick() - Ticks < 1000)
#else
        while (HAL_GetTick() - Ticks < 500)
#endif
        {
					if (!MAC.Tx.Retries) break;
					osThreadYield();
				}
			}
    } else {
#ifndef MAC_COORDINATOR
      osSignalSet(MAC_AppTimerTid, 1);
#endif
			osThreadYield();
		}
  }
}

#ifndef MAC_COORDINATOR
void MAC_AppTimer(void const *argument) {
  while (1) {
    if (MAC.Pib.AssociatedCoord == MAC_PIB_ASSOCIATED_RESET) {
      osDelay(2000);
      if (MAC.Pib.CoordExtendedAdr == 0 &&
          MAC.Pib.CoordShortAdr == MAC_CONST_BROADCAST_ADDRESS) {
        MAC_CmdDiscoverRequestSend(&MAC);
      } else {
        MAC_CmdAssocRequestSend(&MAC);
				osDelay(2000);
        MAC_CmdDataRequestSend(&MAC);
				osDelay(5000);
      }
    } else {
			osDelay(5000);
      //MAC_CmdDataRequestSend(&MAC);
			//osDelay(2500);
			DummyBPM[3] = 58 + (RND_Get() % 15);
			DummyTemp[3] = 70 + (RND_Get() % 10);
      MAC_GenTxData(&MAC, DummyBPM, sizeof(DummyBPM));
      MAC_GenTxData(&MAC, DummyTemp, sizeof(DummyTemp));
    }
		osSignalWait(1, osWaitForever);
  }
}

void MAC_AppButton(void const *argument) {
  while (1) {
    if (!__GPIO_READ(GPIOC, 13)) {
      MAC_GenTxData(&MAC, SendAlert, sizeof(SendAlert));
      osDelay(1000);
    }
  }
}
#endif
