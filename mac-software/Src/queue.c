#include "queue.h"

/* These functions SHOULD ONLY BE CALLED from a locked function, otherwise
 * race condition may occur */
force_inline QUE_Item *QUE_QueueAllocItem(QUE_Queue *H) {
  QUE_Item *I;

  if (H->Free) {
    // Free memory available, take new item from free memory
    I = H->Free;
    H->Free = I->Next;
  } else {
    // No free memory, take new item from oldest queue
    I = H->First;
    H->First = I->Next;
  }

  return I;
}

force_inline QUE_Item *QUE_QueueGetItem(QUE_Queue *H) {
  QUE_Item *I;

  // No data available
  if (!H->First) return NULL;

  I = H->First;
  H->First = I->Next;

  return I;
}

force_inline void QUE_QueuePutFirstItem(QUE_Queue *H, QUE_Item *I) {
  I->Next = NULL;
  H->First = I;
  H->Last = I;
}

force_inline void QUE_QueuePutItemToFirst(QUE_Queue *H, QUE_Item *I) {
  if (H->First) {
    I->Next = H->First;
    H->First = I;
  } else {
    QUE_QueuePutFirstItem(H, I);
  }
}

force_inline void QUE_QueuePutItemToLast(QUE_Queue *H, QUE_Item *I) {
  if (H->First) {
    I->Next = NULL;
    H->Last->Next = I;
    H->Last = I;
  } else {
    QUE_QueuePutFirstItem(H, I);
  }
}

/* Public queue functions */
void QUE_QueueInit(QUE_Queue *H, QUE_Item *Mem, int Len) {
  // Initialize head pointer
  H->First = NULL;
  H->Last = NULL;
  H->Free = NULL;
  LOCK_End(&H->Lock);
  // Assign free memory
  while (Len-- > 0) {
    Mem->Next = H->Free;
    Mem->Data = NULL;
    H->Free = Mem++;
  }
}

void *QUE_QueuePush(QUE_Queue *H, void *Data) {
  QUE_Item *I;
  void *LastData;

  LOCK_Start(&H->Lock);

  I = QUE_QueueAllocItem(H);
  LastData = I->Data;
  I->Data = Data;
  QUE_QueuePutItemToFirst(H, I);

  LOCK_End(&H->Lock);

  return LastData;
}

void *QUE_QueueAppend(QUE_Queue *H, void *Data) {
  QUE_Item *I;
  void *LastData;

  LOCK_Start(&H->Lock);

  I = QUE_QueueAllocItem(H);
  LastData = I->Data;
  I->Data = Data;
  QUE_QueuePutItemToLast(H, I);

  LOCK_End(&H->Lock);

  return LastData;
}

void *QUE_QueuePop(QUE_Queue *H) {
  QUE_Item *I;
  void *Data;

  LOCK_Start(&H->Lock);

  I = QUE_QueueGetItem(H);
  if (I) {
    Data = I->Data;
    QUE_QueueFreeItem(H, I);
  } else {
    Data = NULL;
  }

  LOCK_End(&H->Lock);

  return Data;
}
