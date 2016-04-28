/* Self library includes */
#include "memory.h"

static uint8_t MEM_Heap[MEM_HEAP_SIZE];
static uint8_t *MEM_SM_Head;
static uint8_t *MEM_MD_Head;
static uint8_t *MEM_LG_Head;
static uint8_t *MEM_XL_Head;

static volatile uint8_t MEM_Lock;

static void mem_init_stn(int count, int size, uint8_t **head) {
  static uint8_t *ptr = MEM_Heap;
  int i;

  // Initialize head pointer
  *head = NULL;
  // Iterate the next pointer
  for (i = 0; i < count; i++) {
    // Store the current head to the next pointer
    *((uint8_t **) ptr) = *head;
    // Move the head pointer
    *head = ptr;
    // Move the next pointer
    ptr += size + PTR_SIZE;
  }
}

void MEM_Init(void) {
  mem_init_stn(MEM_SM_COUNT, MEM_SM_SIZE, &MEM_SM_Head);
  mem_init_stn(MEM_MD_COUNT, MEM_MD_SIZE, &MEM_MD_Head);
  mem_init_stn(MEM_LG_COUNT, MEM_LG_SIZE, &MEM_LG_Head);
  mem_init_stn(MEM_XL_COUNT, MEM_XL_SIZE, &MEM_XL_Head);
}

void *MEM_Alloc(int size) {
  uint8_t **head, *ptr;

  // Resolve appropriate head pointer
  if (size <= MEM_SM_SIZE) {
    head = &MEM_SM_Head;
  } else if (size <= MEM_MD_SIZE) {
    head = &MEM_MD_Head;
  } else if (size <= MEM_LG_SIZE) {
    head = &MEM_LG_Head;
  } else if (size <= MEM_XL_SIZE) {
    head = &MEM_XL_Head;
  } else {
    // The memory requirement is too large
    return NULL;
  }

  // Lock the memory operation
  LOCK_Start(&MEM_Lock);

  // Checks for memory pool availability
  if (*head) {
    // Change current pointer to the head
    ptr = *head;
    // Change the head pointer to the next free pointer
    *head = *((uint8_t **) ptr);
    // Tag the reserved memory with the apropriate head address
    *((uint8_t ***) ptr) = head;
    // Return the reserved memory address
    ptr += PTR_SIZE;
  } else {
    // No memory can be reserved at this moment
    ptr = NULL;
  }

  // Unlock the memory operation
  LOCK_End(&MEM_Lock);

  return ptr;
}

void MEM_Free(void *inptr) {
  uint8_t *ptr;
  uint8_t **head;

  // Set the pointer offset
  ptr = (uint8_t *)inptr - PTR_SIZE;
  // Set the head pointer
  head = *((uint8_t ***) ptr);

  // Checks if the pointer is in the heap region
  if (ptr < MEM_Heap + MEM_HEAP_SIZE) {
    // Checks if the address belongs to the memory pool
    if (head == &MEM_SM_Head || head == &MEM_MD_Head ||
        head == &MEM_LG_Head || head == &MEM_XL_Head) {
      // Lock the memory operation
      LOCK_Start(&MEM_Lock);
      // Move the next pointer to the freed memory
      *((uint8_t **)ptr) = *head;
      // Move the head pointer
      *head = ptr;
      // Unlock the memory operation
      LOCK_End(&MEM_Lock);
    }
  }
}
