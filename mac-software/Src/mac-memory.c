#include "mac-memory.h"

void MAC_MemInit(MAC_MemHandle *H, void *I, size_t Size, int Length) {
  // Unlock memory handle
  LOCK_End(&H->Lock);
  H->Free = NULL;

  while (Length -- > 0) {
    *((void **) I) = H->Free;
    H->Free = I;
    I = (uint8_t *) I + Size;
  }
}

void *MAC_MemAlloc(MAC_MemHandle *H) {
  void *I;

  // Start lock mechanism
  LOCK_Start(&H->Lock);
  // Check for free memory
  if (!H->Free) return NULL;
  I = H->Free;
  // Move free pointer
  H->Free = *((void **) I);
  *((void **) I) = H;
  LOCK_End(&H->Lock);

  return (uint8_t *) I + sizeof(void *);
}

void MAC_MemFree(MAC_MemHandle *H, void *I) {
  // Check memory belonging
  I = (uint8_t *) I - sizeof(void *);
  if (*((void **) I) != H) return;

  // Start lock mechanism
  LOCK_Start(&H->Lock);
  *((void **) I) = H->Free;
  H->Free = I;
  LOCK_End(&H->Lock);
}

void MAC_MemPoolInit(MAC_Mem *M) {
  MAC_MemInit(&M->Frame,M->MAC_MemPoolCall(Frame), MAC_MemItemSize(MAC_Frame),
              MAC_CONFIG_MAX_FRAME_POOL);
  MAC_MemInit(&M->AdrList,M->MAC_MemPoolCall(AdrList),
              MAC_MemItemSize(MAC_AdrList), MAC_CONFIG_MAX_ADDRESS_LIST);
}

void MAC_MemQueueInit(MAC_Mem *M) {
  QUE_QueueInit(&M->Tx, M->QUE_MemCall(Tx), MAC_CONFIG_MAX_QUEUE_TX);
  QUE_QueueInit(&M->Store, M->QUE_MemCall(Store), MAC_CONFIG_MAX_QUEUE_STORE);
  QUE_QueueInit(&M->Address, M->QUE_MemCall(Address),
                MAC_CONFIG_MAX_QUEUE_ADDRESS);
}
