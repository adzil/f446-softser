#ifndef __MAC_CORE_H__
#define __MAC_CORE_H__

#include <inttypes.h>
#include "mac-instance.h"
#include "mac-command.h"
#include "phy.h"

void MAC_CoreFrameReceived(MAC_Instance *H, uint8_t *Data, size_t Length);
MAC_Status MAC_CoreFrameBackAck(MAC_Instance *H, MAC_Frame *F);
MAC_Status MAC_CoreFrameSend(MAC_Instance *H, uint8_t **Data, size_t *Len);
MAC_Status MAC_CoreCheckAddressing(MAC_Instance *H, MAC_Frame *F);
void MAC_GenTxData(MAC_Instance *H, uint8_t *Data, size_t Length);

#endif // __MAC_CORE_H__
