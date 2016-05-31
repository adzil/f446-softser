#ifndef __MAC_FRAME__
#define __MAC_FRAME__

/* Enumeration Type Definition */
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
  MAC_FRAME_TYPE_DATA = 0x01,
  MAC_FRAME_TYPE_ACK = 0x02,
  MAC_FRAME_TYPE_COMMAND = 0x03
} MAC_FrameType;

typedef enum {
  MAC_COMMAND_ASSOC_REQUEST = 0x00,
  MAC_COMMAND_ASSOC_RESPONSE = 0x01,
  MAC_COMMAND_DATA_REQUEST = 0x04,
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

/* Frame Struct TypeDefinition */
typedef union {
  uint16_t Short;
  uint32_t Extended;
} MAC_FrameAddress;

typedef struct {
  MAC_FrameAddress Destination;
  MAC_FrameAddress Source;
} MAC_FrameAddressGroup;

typedef struct {
  MAC_CommandFrameId CommandFrameId;
  uint16_t ShortAddress;
  MAC_AssociationStatus Status;
} MAC_FrameCommandPayload;

typedef struct {
  union {
    uint8_t *Data;
    MAC_FrameCommandPayload Command;
  };
  int Start;
  int Length;
} MAC_FramePayload;

typedef struct {
  MAC_AddressMode DestinationAddressMode  : 2;
  MAC_AddressMode SourceAddressMode       : 2;
  MAC_AckRequest AckRequest               : 1;
  MAC_FramePending FramePending           : 1;
  MAC_FrameType FrameType                 : 2;
} MAC_FrameControl;

typedef struct {
  MAC_FrameAddressGroup Address;
  MAC_FramePayload Payload;
  MAC_FrameControl FrameControl;
  uint8_t Sequence;
} MAC_Frame;

#endif // __MAC_FRAME__
