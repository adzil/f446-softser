#ifndef __MAC__
#define __MAC__

// Library includes
#include <inttypes.h>
#include <string.h>
#include "devincludes.h"
#include "queue.h"
#include "phy.h"
#include "crc16.h"

// Mac-specific includes
#include "mac-frame.h"

// MAC constants
#define MAC_EXTENDED_ADDRESS 0x12345678
#define MAC_ACK_WAIT_DURATION 1000 // in ms
#define MAC_BACKOFF_DURATION ((uint32_t) (RND_Get() & 0x1f) * 20)
#define MAC_ACK_MAX_RETRIES 3
#define MAC_QUEUE_SIZE 8
#define MAC_DEVICE_ADDRESS_SIZE 10

#define MAC_ADDRESS_UNKNOWN 0xffff
#define MAC_USE_EXTENDED_ADDRESS 0xfffe



#endif // __MAC__