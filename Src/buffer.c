#include <buffer.h>

void BUF_Init(BUF_HandleTypeDef *Handle, void *Buffer, size_t Size,
              size_t Length) {
  Handle->Start = Buffer;
  Handle->End = Buffer + Size * Length;
  Handle->Head = Buffer;
  Handle->Tail = Buffer + Size;
  Handle->Size = Size;
}

void BUF_Next(BUF_HandleTypeDef *Handle, void **Addr) {
  *Addr += Handle->Size;
  if (*Addr == Handle->End) *Addr = Handle->Start;
}

void *BUF_Write(BUF_HandleTypeDef *Handle) {
  void *Addr;
  // Buffer is full
  if (Handle->Head == Handle->Tail) return NULL;
  Addr = Handle->Tail;
  BUF_Next(Handle, &Handle->Tail);

  return Addr;
}

void *BUF_Read(BUF_HandleTypeDef *Handle) {
  void *Addr = Handle->Head;
  void *RetAddr = Handle->Head;

  BUF_Next(Handle, &Addr);
  // No more data to read
  if (Addr == Handle->Tail) return NULL;
  Handle->Head = Addr;

  return RetAddr;
}

void BUF_Flush(BUF_HandleTypeDef *Handle) {
  Handle->Tail = Handle->Head;
  BUF_Next(Handle, &Handle->Tail);
}