#ifndef __BUFFER
#define __BUFFER

#include <inttypes.h>
#include <stddef.h>
#include <macros.h>

typedef struct {
  uint8_t *Buffer;
  uint16_t Start;
  uint16_t End;
  uint16_t Size;
  uint16_t Length;
}BUF_HandleTypeDef;

/* Inline functions */
_inline_ uint16_t BUF_Next (BUF_HandleTypeDef *Handle ,uint16_t Current) {
  return (Current + Handle->Size) % Handle->Length;
}

uint16_t BUF_Next(BUF_HandleTypeDef *Handle ,uint16_t Current);
void BUF_Init(BUF_HandleTypeDef *Handle, uint8_t *Buffer, uint16_t Size,
              uint16_t Length);
void *BUF_Write(BUF_HandleTypeDef *Handle);
void *BUF_Read(BUF_HandleTypeDef *Handle);
void BUF_Flush(BUF_HandleTypeDef *Handle);

#endif //__BUFFER
