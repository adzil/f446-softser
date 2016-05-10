#ifndef __MAC__
#define __MAC__

// Library includes
#include <inttypes.h>
#include <string.h>
#include "memory.h"
#include "queue.h"
#include "phy.h"
#include "stm32f4xx.h"
#include "crc16.h"

// MAC constants
#define MAC_EXTENDED_ADDRESS 0x12345678
#define MAC_ACK_WAIT_DURATION 1000 // in ms
#define MAC_BACKOFF_DURATION ((uint32_t) (RND_Get() & 0x1f) * 20)
#define MAC_ACK_MAX_RETRIES 3
#define MAC_QUEUE_SIZE 8
#define MAC_DEVICE_ADDRESS_SIZE 10

#define MAC_ADDRESS_UNKNOWN 0xffff
#define MAC_USE_EXTENDED_ADDRESS 0xfffe

typedef enum {
  MAC_OK,
  MAC_INVALID_CHECKSUM,
  MAC_MEM_NOT_AVAIL,
  MAC_INVALID_FRAME_LENGTH,
  MAC_INVALID_ROUTINE,
  MAC_INVALID_ADDRESSING,
  MAC_INVALID_DEVICE_ADDRESS,
  MAC_QUEUE_FULL
} MAC_Status;

typedef enum {
  MAC_ASSOCIATION_SUCCESS,
  MAC_ASSOCIATION_FAILED
} MAC_AssociationStatus;

#endif // __MAC__