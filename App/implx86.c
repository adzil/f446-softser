// Implementation file for X86-Compatible

#include <stdlib.h>
#include <random.h>
#include <memory.h>

uint8_t RND_Get(void) {
  return rand() & 0xff;
}

void *MEM_Alloc(int size) {
  return malloc(size);
}

void MEM_Free(void *inptr) {
  free(inptr);
}