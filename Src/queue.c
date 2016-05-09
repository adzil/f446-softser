#include <queue.h>

void QUE_Init(QUE_Pool *Pool, QUE_Item *Item, uint16_t nItems) {
  Pool->Free = NULL;

  while (nItems--) {
    Item->Next = Pool->Free;
    Pool->Free = Item++;
  }

  Pool->QueueHead = NULL;
  Pool->QueueTail = NULL;
}

void QUE_InitData(QUE_Item *Item, void *Data, uint16_t Len, uint16_t nItems) {
  uint8_t *ByteData;

  ByteData = Data;

  while (nItems--) {
    // Assign data pointer to the actual data
    Item->Data = ByteData;
    // Increment item pointer and data pointer
    Item++;
    ByteData += Len;
  }
}

QUE_Item *QUE_Put(QUE_Pool *Pool) {
  QUE_Item *Item;

  if (!Pool->Free)
    return NULL;

  // Lock the queue
  LOCK_Start(&Pool->Lock);
  // Get free item
  Item = Pool->Free;
  Pool->Free = Item->Next;
  // Fill the free item
  Item->Next = NULL;
  // Release the lock
  LOCK_End(&Pool->Lock);

  return Item;
}

void QUE_PutCommit(QUE_Pool *Pool, QUE_Item *Item) {
  // Check for item address
  if (!Item) return;
  // Lock the queue
  LOCK_Start(&Pool->Lock);
  // Check for queue head
  if (Pool->QueueHead) {
    // Head exists, continue on tail
    Pool->QueueTail->Next = Item;
    Pool->QueueTail = Item;
  } else {
    // Write new head
    Pool->QueueHead = Item;
    Pool->QueueTail = Item;
  }
  // Release the lock
  LOCK_End(&Pool->Lock);
}

QUE_Item *QUE_Get(QUE_Pool *Pool) {
  QUE_Item *Item;

  if (!Pool->QueueHead)
    return NULL;

  // Lock the queue
  LOCK_Start(&Pool->Lock);
  // Get the queue head
  Item = Pool->QueueHead;
  // Move to the next item
  Pool->QueueHead = Item->Next;
  // Release the lock
  LOCK_End(&Pool->Lock);

  return Item;
}

void QUE_GetCommit(QUE_Pool *Pool, QUE_Item *Item) {
  // Check for item address
  if (!Item) return;
  // Lock the queue
  LOCK_Start(&Pool->Lock);
  // Return back item to the free pool
  Item->Next = Pool->Free;
  Pool->Free = Item;
  // Release the lock
  LOCK_End(&Pool->Lock);
}

void QUE_Flush(QUE_Pool *Pool) {
  QUE_Item *Item;

  // Check for empty queue
  if (!Pool->QueueHead)
    return;

  // Lock the queue
  LOCK_Start(&Pool->Lock);
  // Iterate until empty queue
  do {
    // Get item at the head of the queue
    Item = Pool->QueueHead;
    Pool->QueueHead = Item->Next;
    // Release item to the free pool
    Item->Next = Pool->Free;
    Pool->Free = Item;
  } while (Pool->QueueHead);
  // Release the lock
  LOCK_End(&Pool->Lock);
}