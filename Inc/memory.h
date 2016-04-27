#ifndef __MEMORY
#define __MEMORY

#include <inttypes.h>
#include <stddef.h>

#define MEM_SM_SIZE 64
#define MEM_SM_COUNT 32
#define MEM_MD_SIZE 256
#define MEM_MD_COUNT 32
#define MEM_LG_SIZE 512
#define MEM_LG_COUNT 16
#define MEM_XL_SIZE 1024
#define MEM_XL_COUNT 8

#define PTR_SIZE sizeof(void *)

// Heap size is total bytes + linked list pointer
#define MEM_HEAP_SIZE \
        (MEM_SM_SIZE + PTR_SIZE) * MEM_SM_COUNT + \
        (MEM_MD_SIZE + PTR_SIZE) * MEM_MD_COUNT + \
        (MEM_LG_SIZE + PTR_SIZE) * MEM_LG_COUNT + \
        (MEM_XL_SIZE + PTR_SIZE) * MEM_XL_COUNT

void MEM_Init(void);
void *MEM_Alloc(int size);
void MEM_Free(void *inptr);

#endif // __MEMORY
