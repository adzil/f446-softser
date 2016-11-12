#ifndef __MAC_MEMORY_H__
#define __MAC_MEMORY_H__

#include <stddef.h>
#include "mac-frame.h"
#include "mac-config.h"
#include "queue.h"
#include "lock.h"

#define MAC_MemPoolCall(Name) MemPool##Name
#define MAC_MemPoolAlloc(Name, Item, Len) \
  uint8_t MAC_MemPoolCall(Name)[(sizeof(Item) + sizeof(void *)) * Len]
#define MAC_MemItemSize(Item) (sizeof(Item) + sizeof(void *))

typedef struct {
  void *Free;
  LOCK_Handle Lock;
} MAC_MemHandle;

typedef struct {
  MAC_MemHandle Frame;
  MAC_MemHandle AdrList;
  QUE_Queue Tx;
  QUE_Queue Store;
  QUE_Queue Address;
  // Memory pool should be located at the end of the stack
  MAC_MemPoolAlloc(Frame, MAC_Frame, MAC_CONFIG_MAX_FRAME_POOL);
  MAC_MemPoolAlloc(AdrList, MAC_AdrList, MAC_CONFIG_MAX_ADDRESS_LIST);
  QUE_MemAlloc(Tx, MAC_CONFIG_MAX_QUEUE_TX);
  QUE_MemAlloc(Store, MAC_CONFIG_MAX_QUEUE_STORE);
  QUE_MemAlloc(Address, MAC_CONFIG_MAX_QUEUE_ADDRESS);
} MAC_Mem;

void MAC_MemInit(MAC_MemHandle *H, void *I, size_t Size, int Length);
void *MAC_MemAlloc(MAC_MemHandle *H);
void MAC_MemFree(MAC_MemHandle *H, void *I);

void MAC_MemPoolInit(MAC_Mem *M);
void MAC_MemQueueInit(MAC_Mem *M);

/* Type override for specific buffer requirement */
force_inline MAC_Frame *MAC_MemFrameAlloc(MAC_Mem *M) {
  return (MAC_Frame *) MAC_MemAlloc(&M->Frame);
}

force_inline void MAC_MemFrameFree(MAC_Mem *M, MAC_Frame *F) {
  MAC_MemFree(&M->Frame, (void *) F);
}

force_inline MAC_AdrList *MAC_MemAdrListAlloc(MAC_Mem *M) {
  return (MAC_AdrList *) MAC_MemAlloc(&M->AdrList);
}

force_inline void MAC_MemAdrListFree(MAC_Mem *M, MAC_AdrList *A) {
  MAC_MemFree(&M->AdrList, (void *) A);
}

#endif // __MAC_MEMORY_H__
