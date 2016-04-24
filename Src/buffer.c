#include <buffer.h>

void BUF_Init(BUF_HandleTypeDef *Handle, void *Buffer, uint16_t Size,
              uint16_t Length) {
  Handle->Start = 0;
  Handle->End = 0;
  Handle->Buffer = Buffer;
  Handle->Size = Size;
  Handle->Length = Length * Size;
}

void *BUF_Write(BUF_HandleTypeDef *Handle) {
  uint8_t *ptr = Handle->Buffer;

  ptr += Handle->End;
  Handle->End += Handle->Size;
  if (Handle->End >= Handle->Length) Handle->End = 0;

  return ptr;
}

void *BUF_Read(BUF_HandleTypeDef *Handle) {
  uint8_t *ptr = Handle->Buffer;

  // No more data to read
  if (Handle->Start == Handle->End) return NULL;

  ptr += Handle->Start;
  Handle->Start += Handle->Size;
  if (Handle->Start >= Handle->Length) Handle->Start = 0;

  return ptr;
}
