#ifndef __BUFFER
#define __BUFFER

#include <inttypes.h>
#include <stddef.h>

typedef struct {
  uint8_t *Head;
  uint8_t *Tail;
  uint8_t *Start;
  uint8_t *End;
  uint16_t Size;
}BUF_HandleTypeDef;

void BUF_Init(BUF_HandleTypeDef *Handle, uint8_t *Buffer, uint16_t Size,
              uint16_t Length);
void *BUF_Write(BUF_HandleTypeDef *Handle);
void *BUF_Read(BUF_HandleTypeDef *Handle);
void BUF_Flush(BUF_HandleTypeDef *Handle);

#endif //__BUFFER
