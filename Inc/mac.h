#ifndef __MAC__
#define __MAC__

#include <inttypes.h>
#include <string.h>
#include "memory.h"
#include "macros.h"
#include "stm32f4xx.h"
#include "crc16.h"

typedef enum {
  MAC_OK,
  MAC_INVALID_CHECKSUM,
  MAC_MEM_NOT_AVAIL,
  MAC_INVALID_FRAME_LENGTH
} MAC_Status;

typedef enum {
  MAC_ADDRESS_SHORT = 0x00,
  MAC_ADDRESS_EXTENDED = 0x01
} MAC_AddressModeEnum;

typedef enum {
  MAC_FRAME_TYPE_BEACON = 0x00,
  MAC_FRAME_TYPE_DATA = 0x01,
  MAC_FRAME_TYPE_ACK = 0x02,
  MAC_FRAME_TYPE_COMMAND = 0x03
} MAC_FrameTypeEnum;

typedef enum {
  MAC_COMMAND_ASSOC_REQUEST = 0x00,
  MAC_COMMAND_ASSOC_RESPONSE = 0x01,
  MAC_COMMAND_DATA_REQUEST = 0x04,
  MAC_COMMAND_BEACON_REQUEST = 0x06
} MAC_CommandIdEnum;

typedef struct {
  MAC_AddressModeEnum AddressMode :1;
  uint8_t AckRequest :1;
  uint8_t FramePending :1;
  uint8_t Coordinator :1;
  uint8_t Broadcast :1;
  MAC_FrameTypeEnum FrameType :3;
} MAC_FrameControlTypeDef;

typedef union {
  uint16_t Short;
  uint32_t Extended;
} MAC_AddressFieldTypeDef;

typedef struct {
  void *Payload;
  uint8_t *DataPtr;
  MAC_AddressFieldTypeDef Address;
  uint16_t PayloadLen;
  uint8_t Sequence;
  MAC_FrameControlTypeDef FrameControl;
} MAC_FrameTypeDef;

#endif // __MAC__
