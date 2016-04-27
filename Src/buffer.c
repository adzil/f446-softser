#include <buffer.h>

/* Private function prototypes */
uint16_t BUF_Next(BUF_HandleTypeDef *Handle ,uint16_t Current);

/* Inline functions */
inline uint16_t BUF_Next(BUF_HandleTypeDef *Handle ,uint16_t Current) {
  return (Current + Handle->Size) % Handle->Length;
}

void BUF_Init(BUF_HandleTypeDef *Handle, uint8_t *Buffer, uint16_t Size,
              uint16_t Length) {
  Handle->Buffer = Buffer;
  Handle->Length = Length * Size;
  Handle->Size = Size;
  Handle->Start = 0;
  Handle->End = 0;
}

void *BUF_Write(BUF_HandleTypeDef *Handle) {
  uint16_t Next;
  void *Ptr;

  // Get next address
  Next = BUF_Next(Handle, Handle->End);
  // Checks if the buffer is full
  if (Next == Handle->Start) return NULL;
  Ptr = Handle->Buffer + Handle->End;
  Handle->End = Next;

  return Ptr;
}

void *BUF_Read(BUF_HandleTypeDef *Handle) {
  uint16_t Next;
  void *Ptr;

  // Checks if the buffer is empty
  if (Handle->Start == Handle->End) return NULL;
  // Get next address
  Next = BUF_Next(Handle, Handle->Start);
  Ptr = Handle->Buffer + Handle->Start;
  Handle->Start = Next;

  return Ptr;
}

void BUF_Flush(BUF_HandleTypeDef *Handle) {
  Handle->Start = Handle->End;
}
