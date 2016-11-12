#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <inttypes.h>
#include <stddef.h>
#include "lock.h"
#include "appdef.h"

typedef struct QUE_Item {
  struct QUE_Item *Next;
  void *Data;
} QUE_Item;

typedef struct {
  QUE_Item *First;
  QUE_Item *Last;
  QUE_Item *Free;
  QUE_Item *Find;
  QUE_Item *FindLast;
  LOCK_Handle Lock;
} QUE_Queue;

#define QUE_MemCall(Name) QuePool##Name
#define QUE_MemAlloc(Name, Len) QUE_Item QUE_MemCall(Name)[Len]

void QUE_QueueInit(QUE_Queue *H, QUE_Item *Mem, int Len);
void *QUE_QueuePush(QUE_Queue *H, void *Data);
void *QUE_QueueAppend(QUE_Queue *H, void *Data);
void *QUE_QueuePop(QUE_Queue *H);

/* Inline functions to data-independent list search */
force_inline void QUE_QueueFreeItem(QUE_Queue *H, QUE_Item *I) {
  I->Next = H->Free;
  I->Data = NULL;
  H->Free = I;
}

force_inline void QUE_QueueFindInit(QUE_Queue *H) {
  // Initialize find pointer
  H->Find = NULL;
}

force_inline void *QUE_QueueFind(QUE_Queue *H) {
  // Iterate list
  if (H->Find) {
    H->FindLast = H->Find;
    H->Find = H->Find->Next;
  } else {
    H->Find = H->First;
    H->FindLast = NULL;
  }

  if (!H->Find) return NULL;
  return H->Find->Data;
}

force_inline void QUE_QueueFindPop(QUE_Queue *H) {
  // Check if the item has been found
  if (H->Find) {
    // Fix last item linkage
    if (H->FindLast) {
      H->FindLast->Next = H->Find->Next;
    }
    // Fix queue pointer
    if (H->First == H->Find) {
      H->First = H->Find->Next;
    }
    if (H->Last == H->Find) {
      H->Last = H->FindLast;
    }
    // Free queue
    QUE_QueueFreeItem(H, H->Find);
  }
}

#endif // __QUEUE_H__
