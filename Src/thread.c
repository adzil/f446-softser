#include <thread.h>

osThreadDef(PHY_Thread, osPriorityNormal, 1, 0);

void THR_Init(void) {
  // Initialize PHY
  PHY_ThreadId = osThreadCreate(osThread(PHY_Thread), NULL);
}
