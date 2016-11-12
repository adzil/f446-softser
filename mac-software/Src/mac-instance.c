#include "mac-instance.h"

char MAC_TermBuf[256];

void MAC_Init(MAC_Instance *H, uint32_t ExtendedAdr,
              MAC_PibVpanCoordinator VpanCoord) {
  // Memory initialization
  MAC_MemPoolInit(&H->Mem);
  MAC_MemQueueInit(&H->Mem);
  // Pib initialization
  H->Pib.CoordShortAdr = MAC_CONST_BROADCAST_ADDRESS;
  H->Pib.CoordExtendedAdr = 0;
  H->Pib.AssociatedCoord = MAC_PIB_ASSOCIATED_RESET;
  H->Pib.ShortAdr = MAC_CONST_USE_EXTENDED_ADDRESS;
  H->Pib.DSN = (uint8_t) RND_Get();
  H->Pib.VpanCoordinator = VpanCoord;
  // Config initialization
  H->Config.ExtendedAddress = ExtendedAdr;
  // Transmission initialization
  H->Tx.Retries = 0;
  H->Tx.FailedAck = 0;
  LOCK_End(&H->Tx.Lock);
}

void MAC_TransmitPutFrame(MAC_Instance *H, MAC_Frame *F) {
  if (H->Pib.VpanCoordinator == MAC_PIB_VPAN_COORDINATOR) {
#ifdef MAC_DEBUG
    MAC_DebugFrame(H, F, MAC_DEBUG_QUE);
#endif
    F = MAC_QueueFrameAppend(&H->Mem.Store, F);
    if (F) MAC_MemFrameFree(&H->Mem, F);
  } else {
    MAC_TransmitSendFrame(H, F);
  }
}

void MAC_GenFrameSrcAdr(MAC_Instance *H, MAC_Frame *F) {
  if (H->Pib.VpanCoordinator == MAC_PIB_VPAN_COORDINATOR) {
    if (F->FrameControl.DstAdrMode == MAC_ADRMODE_SHORT)
      // No source address for short destination addresses
      MAC_SetFrameNoSrcAdr(F);
    else if (H->Pib.ShortAdr != MAC_CONST_USE_EXTENDED_ADDRESS &&
             H->Pib.ShortAdr != MAC_CONST_BROADCAST_ADDRESS)
      // Use short address if already defined
      MAC_SetFrameShortSrcAdr(F, H->Pib.ShortAdr);
    else
      // Fallback using extended address
      MAC_SetFrameExtendedSrcAdr(F, H->Config.ExtendedAddress);
  } else {
    if (H->Pib.AssociatedCoord == MAC_PIB_ASSOCIATED_SET &&
        H->Pib.ShortAdr != MAC_CONST_USE_EXTENDED_ADDRESS &&
        H->Pib.ShortAdr != MAC_CONST_BROADCAST_ADDRESS)
      // If associated and address assigned, use it
      MAC_SetFrameShortSrcAdr(F, H->Pib.ShortAdr);
    else
      // Fallback using extended address
      MAC_SetFrameExtendedSrcAdr(F, H->Config.ExtendedAddress);
  }
}
