#ifndef __QUEUE__
#define __QUEUE__

#include <inttypes.h>
#include <stddef.h>
#include "lock.h"

typedef struct QUE_Item {
  struct QUE_Item *Next;
  void *Data;
} QUE_Item;

typedef struct {
  QUE_Item *Free;
  QUE_Item *QueueHead;
  QUE_Item *QueueTail;
  LOCK_Handle Lock;
} QUE_Pool;

void QUE_Init(QUE_Pool *Pool, QUE_Item *Item, uint16_t nItems);
void QUE_InitData(QUE_Item *Item, void *Data, uint16_t Len, uint16_t nItems);
QUE_Item *QUE_Put(QUE_Pool *Pool);
void QUE_PutCommit(QUE_Pool *Pool, QUE_Item *Item);
QUE_Item *QUE_Get(QUE_Pool *Pool);
void QUE_GetCommit(QUE_Pool *Pool, QUE_Item *Item);
void QUE_Flush(QUE_Pool *Pool);

#endif // __QUEUE__
