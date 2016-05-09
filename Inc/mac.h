#ifndef __MAC__
#define __MAC__

#include <inttypes.h>
#include <string.h>
#include "memory.h"
#include "macros.h"
#include "queue.h"
#ifdef APP_DEBUG
#include "bitrev.h"
#else
#include "stm32f4xx.h"
#endif
#include "crc16.h"

// MAC Constants
#define MAC_EXTENDED_ADDRESS 0x12345678
#define MAC_ACK_WAIT_DURATION 100 // in ms
#define MAC_BACKOFF_DURATION ((uint32_t) (RND_Get() & 0x1f) * 20)
#define MAC_ACK_MAX_RETRIES 3
#define MAC_QUEUE_SIZE 5
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

typedef enum {
  MAC_NOT_ASSOCIATED = 0x00,
  MAC_ASSOCIATED = 0x01
} MAC_AssociatedCoord;

typedef struct {
  MAC_AssociatedCoord AssociatedCoord;
  MAC_VPANCoordinator VPANCoordinator;
  uint32_t CoordExtendedAddress;
  uint16_t CoordShortAddress;
  uint8_t DSN;
  uint8_t BSN;
  uint16_t ShortAddress;
} MAC_PIB;

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
  MAC_FrameControl FrameControl;
  uint8_t Sequence;
} MAC_Frame;

typedef struct {
  uint8_t *Content;
  uint16_t Length;
} MAC_RawData;

typedef struct {
  QUE_Pool RX;
  QUE_Pool TX;
} MAC_Queue;

typedef struct {
  uint32_t ExtendedAddress;
  uint16_t ShortAddress;
} MAC_DeviceAddress;

typedef enum {
  MAC_TRANSMISSION_RESET,
  MAC_TRANSMISSION_BACKOFF,
  MAC_TRANSMISSION_WAIT_ACK,
  MAC_TRANSMISSION_WAIT_RESET,
  MAC_TRANSMISSION_RETRY
} MAC_TransmissionStatus;

typedef struct {
  QUE_Item *Item;
  uint8_t Retries;
  MAC_TransmissionStatus Status;
  uint32_t TickStart;
  uint32_t TickLength;
} MAC_Transmission;

typedef struct {
  MAC_PIB PIB;
  MAC_Queue Queue;
  MAC_Transmission Transmission;
  MAC_DeviceAddress *Devices;
} MAC_Handle;

void MAC_Init(void);
MAC_Frame *MAC_FrameAlloc(uint16_t PayloadLen);
MAC_Status MAC_FrameFree(MAC_Frame *F);
uint16_t MAC_FrameEncodeLen(MAC_Frame *F);
MAC_Frame *MAC_FrameDecode(uint8_t *Data, uint16_t Len);
MAC_Status MAC_FrameEncode(MAC_Frame *F, uint8_t *Data);

// API for PHY layer
MAC_Status MAC_API_DataReceived(uint8_t *Data, uint16_t Length);


#endif // __MAC__
