#ifndef __MEMORY
#define __MEMORY

#include <inttypes.h>
#include <stddef.h>

#define MEM_XXS_SIZE 8
#define MEM_XXS_COUNT 256
#define MEM_XS_SIZE 16
#define MEM_XS_COUNT 128
#define MEM_SM_SIZE 32
#define MEM_SM_COUNT 128
#define MEM_MD_SIZE 64
#define MEM_MD_COUNT 64
#define MEM_LG_SIZE 128
#define MEM_LG_COUNT 32
#define MEM_XL_SIZE 256
#define MEM_XL_COUNT 16
#define MEM_XXL_SIZE 1024
#define MEM_XXL_COUNT 8

#define PTR_SIZE sizeof(void *)

// Heap size is total bytes + linked list pointer
#define MEM_HEAP_SIZE \
        (MEM_XXS_SIZE + PTR_SIZE) * MEM_XXS_COUNT + \
        (MEM_XS_SIZE + PTR_SIZE) * MEM_XS_COUNT + \
        (MEM_SM_SIZE + PTR_SIZE) * MEM_SM_COUNT + \
        (MEM_MD_SIZE + PTR_SIZE) * MEM_MD_COUNT + \
        (MEM_LG_SIZE + PTR_SIZE) * MEM_LG_COUNT + \
        (MEM_XL_SIZE + PTR_SIZE) * MEM_XL_COUNT + \
        (MEM_XXL_SIZE + PTR_SIZE) * MEM_XXL_COUNT

void MEM_Init(void);
void *MEM_Alloc(int size);
void MEM_Free(void *inptr);

#endif // __MEMORY
