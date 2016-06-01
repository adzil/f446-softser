#ifndef __MAC_H__
#define __MAC_H__

#include <inttypes.h>
#include "mac-core.h"
#include "stm32f4xx.h"
#include "cmsis_os.h"
#include "phy.h"

void MAC_AppInit(void);
void MAC_AppDataReceived(uint8_t *Data, uint16_t Length);

// Thread def
void MAC_AppData(void const *argument);
void MAC_AppTimer(void const *argument);

#endif // __MAC_H__