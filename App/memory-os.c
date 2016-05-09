// Replacement for memory.c
#include <stdlib.h>
#include <memory.h>

void *MEM_Alloc(int size) {
  return malloc(size);
}

void MEM_Free(void *inptr) {
  free(inptr);
}

