#include "mac-queue.h"

MAC_Frame *MAC_QueueFrameFind(QUE_Queue *H, MAC_FrameAddressMode AdrMode,
                              MAC_FrameAddress Address) {
  MAC_Frame *F;

  if (AdrMode != MAC_ADRMODE_EXTENDED && AdrMode != MAC_ADRMODE_SHORT)
    return NULL;

  LOCK_Start(&H->Lock);
  QUE_QueueFindInit(H);

  do {
    F = (MAC_Frame *) QUE_QueueFind(H);
    if (!F) break;
    if (F->FrameControl.DstAdrMode == AdrMode) {
      if (AdrMode == MAC_ADRMODE_SHORT) {
        if (F->Address.Dst.Short == Address.Short) break;
      } else {
        if (F->Address.Dst.Extended == Address.Extended) break;
      }
    }
  } while(1);

  QUE_QueueFindPop(H);
  LOCK_End(&H->Lock);

  return F;
}

MAC_AdrList *MAC_QueueAdrListFind(QUE_Queue *H, MAC_FrameAddressMode AdrMode,
                                  MAC_FrameAddress Address) {
  MAC_AdrList *A;

  if (AdrMode != MAC_ADRMODE_EXTENDED && AdrMode != MAC_ADRMODE_SHORT)
    return NULL;

  LOCK_Start(&H->Lock);
  QUE_QueueFindInit(H);

  do {
    A = (MAC_AdrList *) QUE_QueueFind(H);
    if (!A) break;
    if (AdrMode == MAC_ADRMODE_SHORT) {
      if (A->ShortAdr == Address.Short) break;
    } else {
      if (A->ExtendedAdr == Address.Extended) break;
    }
  } while(1);

  LOCK_End(&H->Lock);

  return A;
}
