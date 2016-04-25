#ifndef __BUFFER
#define __BUFFER

#include <inttypes.h>
#include <stddef.h>

typedef struct {
  void *Head;
  void *Tail;
  void *Start;
  void *End;
  size_t Size;
}BUF_HandleTypeDef;

void BUF_Init(BUF_HandleTypeDef *Handle, void *Buffer, uint16_t Size,
              uint16_t Length);
void *BUF_Write(BUF_HandleTypeDef *Handle);
void *BUF_Read(BUF_HandleTypeDef *Handle);
void BUF_Flush(BUF_HandleTypeDef *Handle);

#endif //__BUFFER
