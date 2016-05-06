#ifndef __MAC__
#define __MAC__

#define APP_DEBUG

#include <inttypes.h>
#include <string.h>
#include "memory.h"
#include "macros.h"
#ifdef APP_DEBUG
#include "bitrev.h"
#else
#include "stm32f4xx.h"
#endif
#include "crc16.h"

typedef enum {
  MAC_OK,
  MAC_INVALID_CHECKSUM,
  MAC_MEM_NOT_AVAIL,
  MAC_INVALID_FRAME_LENGTH,
  MAC_INVALID_ROUTINE
} MAC_Status;

typedef enum {
  MAC_ASSOCIATION_SUCCESS,
  MAC_ASSOCIATION_FAILED
} MAC_AssociationStatus;

typedef enum {
  MAC_NO_ADDRESS = 0x00,
  MAC_ADDRESS_SHORT = 0x02,
  MAC_ADDRESS_EXTENDED = 0x03
} MAC_AddressMode;

typedef enum {
  MAC_FRAME_TYPE_BEACON = 0x00,
  MAC_FRAME_TYPE_DATA = 0x01,
  MAC_FRAME_TYPE_ACK = 0x02,
  MAC_FRAME_TYPE_COMMAND = 0x03
} MAC_FrameType;

typedef enum {
  MAC_COMMAND_ASSOC_REQUEST = 0x00,
  MAC_COMMAND_ASSOC_RESPONSE = 0x01,
  MAC_COMMAND_DATA_REQUEST = 0x04,
  MAC_COMMAND_BEACON_REQUEST = 0x06
} MAC_CommandFrameId;

typedef enum {
  MAC_NO_ACK = 0x00,
  MAC_ACK = 0x01
} MAC_AckRequest;

typedef enum {
  MAC_FRAME_NOT_PENDING = 0x00,
  MAC_FRAME_PENDING = 0x01
} MAC_FramePending;

typedef enum {
  MAC_VPAN_DEVICE = 0x00,
  MAC_VPAN_COORDINATOR = 0x01,
} MAC_VPANCoordinator;

typedef struct {
  MAC_AddressMode DestinationAddressMode  : 2;
  MAC_AddressMode SourceAddressMode       : 2;
  MAC_AckRequest AckRequest               : 1;
  MAC_FramePending FramePending           : 1;
  MAC_FrameType FrameType                 : 2;
} MAC_FrameControl;

typedef union {
  uint16_t Short;
  uint32_t Extended;
} MAC_AddressType;

typedef struct {
  MAC_AddressType Destination;
  MAC_AddressType Source;
} MAC_AddressField;

typedef struct {
  MAC_VPANCoordinator VPANCoordinator     : 1;
} MAC_BeaconFramePayload;

typedef struct {
  MAC_CommandFrameId CommandFrameId;
  uint16_t ShortAddress;
  MAC_AssociationStatus Status;
} MAC_CommandFramePayload;

typedef struct {
  union {
    uint8_t *Data;
    MAC_BeaconFramePayload Beacon;
    MAC_CommandFramePayload Command;
  };
  int Start;
  int Length;
} MAC_FramePayload;

typedef struct {
  MAC_AddressField Address;
  MAC_FramePayload Payload;
  uint16_t FCS;
  MAC_FrameControl FrameControl;
  uint8_t Sequence;
} MAC_Frame;

MAC_Frame *MAC_FrameAlloc(uint16_t PayloadLen);
MAC_Status MAC_FrameFree(MAC_Frame *F);
uint16_t MAC_FrameEncodeLen(MAC_Frame *F);
MAC_Frame *MAC_FrameDecode(uint8_t *Data, uint16_t Len);
MAC_Status MAC_FrameEncode(MAC_Frame *F, uint8_t *Data);


#endif // __MAC__
