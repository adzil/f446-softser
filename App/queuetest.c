#include <stdio.h>
#include <queue.h>

typedef struct {
  uint32_t a;
  uint16_t b;
} DataTypedef;

QUE_Item myItems[10];
DataTypedef myData[10];
QUE_Pool myPool;

int main(void) {
  int i;
  QUE_Item *item;
  DataTypedef *data;
  uint32_t test;

  QUE_InitData(myItems, myData, sizeof(DataTypedef), 10);
  QUE_Init(&myPool, myItems, 10);

  for (i = 0; i < 10; i++) {
    item = QUE_Put(&myPool);
    data = item->Data;
    data->a = i;
    data->b = (9 - i);
    QUE_PutCommit(&myPool, item);
    if (i == 5) QUE_Flush(&myPool);
  }

  for (i = 0; i < 10; i++) {
    item = QUE_Get(&myPool);
    if (!item) break;
    data = item->Data;
    printf("%d - %d, %d\n", i, data->a, data->b);
    QUE_GetCommit(&myPool, item);
  }

  test = 2U - 0xfffffffc;
  printf("%u\n", test);

  return 0;
}