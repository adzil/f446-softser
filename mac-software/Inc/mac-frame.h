#ifndef __MAC_FRAME_H__
#define __MAC_FRAME_H__

#include <inttypes.h>
#include <string.h>
#include "appdef.h"
#include "mac-config.h"
#include "mac-common.h"
#include "crc16.h"
#include "stm32f4xx.h"

/* Shorthand for frame copy */
#define MAC_ReadByte(Dst, Src) do { \
*((uint8_t *) Dst) = *((uint8_t *) Src); Src += 1; } while(0)
#define MAC_ReadWord(Dst, Src) do { \
*((uint16_t *) Dst) = __REV16(*((uint16_t *) Src)); Src += 2; } while(0)
#define MAC_ReadDword(Dst, Src) do { \
*((uint32_t *) Dst) = __REV(*((uint32_t *) Src)); Src += 4; } while(0)
#define MAC_ReadStream(Dst, Src, Len) do {\
memcpy(Dst, Src, Len); Src += Len; } while(0)

#define MAC_WriteByte(Dst, Src) do { \
*((uint8_t *) Dst) = *((uint8_t *) Src); Dst += 1; } while(0)
#define MAC_WriteWord(Dst, Src) do { \
*((uint16_t *) Dst) = __REV16(*((uint16_t *) Src)); Dst += 2; } while(0)
#define MAC_WriteDword(Dst, Src) do { \
*((uint32_t *) Dst) = __REV(*((uint32_t *) Src)); Dst += 4; } while(0)
#define MAC_WriteStream(Dst, Src, Len) do {\
memcpy(Dst, Src, Len); Dst += Len; } while(0)

/* Enumeration types */
typedef enum {
  MAC_ADRMODE_NOT_PRESENT = 0,
  MAC_ADRMODE_SHORT = 2,
  MAC_ADRMODE_EXTENDED = 3
} MAC_FrameAddressMode;

typedef enum {
  MAC_ACKREQUEST_RESET = 0,
  MAC_ACKREQUEST_SET = 1
} MAC_FrameAckRequest;

typedef enum {
  MAC_FRAMEPENDING_RESET = 0,
  MAC_FRAMEPENDING_SET = 1
} MAC_FramePending;

typedef enum {
  MAC_FRAMETYPE_DATA = 1,
  MAC_FRAMETYPE_ACK = 2,
  MAC_FRAMETYPE_COMMAND = 3
} MAC_FrameType;

typedef enum {
  MAC_COMMAND_ID_ASSOC_REQUEST = 0,
  MAC_COMMAND_ID_ASSOC_RESPONSE = 1,
  MAC_COMMAND_ID_DATA_REQUEST = 4,
  MAC_COMMAND_ID_DISCOVER_REQUEST = 10,
  MAC_COMMAND_ID_DISCOVER_RESPONSE = 11
} MAC_FrameCommandId;

typedef enum {
  MAC_ASSOCSTATUS_SUCCESS = 0,
  MAC_ASSOCSTATUS_FAILED = 1
} MAC_FrameAssocStatus;

/* Address typedef */
typedef union {
  uint16_t Short;
  uint32_t Extended;
} MAC_FrameAddress;

typedef struct {
  MAC_FrameAddress Dst;
  MAC_FrameAddress Src;
} MAC_FrameAddressField;

/* Payload typedef */
typedef struct {
  MAC_FrameCommandId CommandId;
  MAC_FrameAssocStatus AssocStatus;
  uint32_t ExtendedAddress;
  uint16_t ShortAddress;
} MAC_FrameCommand;

typedef struct {
  uint8_t Data[MAC_CONFIG_MAX_PAYLOAD_LENGTH];
  size_t Length;
} MAC_FramePayload;

/* Frame control type */
typedef struct {
  MAC_FrameAddressMode DstAdrMode : 2;
  MAC_FrameAddressMode SrcAdrMode : 2;
  MAC_FrameAckRequest AckRequest : 1;
  MAC_FramePending FramePending : 1;
  MAC_FrameType FrameType : 2;
} MAC_FrameControl;

/* General MAC Frame structure */
typedef struct {
  MAC_FrameControl FrameControl;
  MAC_FrameAddressField Address;
  MAC_FramePayload Payload;
  uint8_t Sequence;
} MAC_Frame;

/* Address list structure */
typedef struct {
  uint32_t ExtendedAdr;
  uint16_t ShortAdr;
} MAC_AdrList;

/* Getter utility */
force_inline uint16_t MAC_GetFrameAddressSize(MAC_FrameAddressMode Mode) {
  switch (Mode) {
    case MAC_ADRMODE_NOT_PRESENT:
      return 0;
    case MAC_ADRMODE_SHORT:
      return 2;
    case MAC_ADRMODE_EXTENDED:
      return 4;
    default:
      return 0;
  }
}

MAC_Status MAC_FrameEncode(MAC_Frame *F, uint8_t *Data, size_t *Len);
MAC_Status MAC_FrameDecode(MAC_Frame *F, uint8_t *Data, size_t Len);
void MAC_FrameCommandEncode(MAC_Frame *F, MAC_FrameCommand *C);
void MAC_FrameCommandDecode(MAC_Frame *F, MAC_FrameCommand *C);

/* Public Setter */
// Destination address (Frame Control & Address Field)
force_inline void MAC_SetFrameShortDstAdr(MAC_Frame *F, uint16_t Adr) {
  F->FrameControl.DstAdrMode = MAC_ADRMODE_SHORT;
  F->Address.Dst.Short = Adr;
}

force_inline void MAC_SetFrameExtendedDstAdr(MAC_Frame *F, uint32_t Adr) {
  F->FrameControl.DstAdrMode = MAC_ADRMODE_EXTENDED;
  F->Address.Dst.Extended = Adr;
}

force_inline void MAC_SetFrameNoDstAdr(MAC_Frame *F) {
  F->FrameControl.DstAdrMode = MAC_ADRMODE_NOT_PRESENT;
}

force_inline void MAC_SetFrameDstAdr(MAC_Frame *F, MAC_FrameAddressMode AdrMode,
                                     MAC_FrameAddress Address) {
  switch (AdrMode) {
    case MAC_ADRMODE_NOT_PRESENT:
      MAC_SetFrameNoDstAdr(F);
      break;

    case MAC_ADRMODE_SHORT:
      MAC_SetFrameShortDstAdr(F, Address.Short);
      break;

    case MAC_ADRMODE_EXTENDED:
      MAC_SetFrameExtendedDstAdr(F, Address.Extended);
      break;

    default:
      break;
  }
}

// Source address (Frame Control & Address Field)
force_inline void MAC_SetFrameShortSrcAdr(MAC_Frame *F, uint16_t Adr) {
  F->FrameControl.SrcAdrMode = MAC_ADRMODE_SHORT;
  F->Address.Src.Short = Adr;
}

force_inline void MAC_SetFrameExtendedSrcAdr(MAC_Frame *F, uint32_t Adr) {
  F->FrameControl.SrcAdrMode = MAC_ADRMODE_EXTENDED;
  F->Address.Src.Extended = Adr;
}

force_inline void MAC_SetFrameNoSrcAdr(MAC_Frame *F) {
  F->FrameControl.SrcAdrMode = MAC_ADRMODE_NOT_PRESENT;
}

// Ack Request (Frame Control)
force_inline void MAC_SetFrameNoAckRequest(MAC_Frame *F) {
  F->FrameControl.AckRequest = MAC_ACKREQUEST_RESET;
}

force_inline void MAC_SetFrameAckRequest(MAC_Frame *F) {
  F->FrameControl.AckRequest = MAC_ACKREQUEST_SET;
}

// Frame Pending (Frame Control)
force_inline void MAC_SetFrameNoPending(MAC_Frame *F) {
  F->FrameControl.FramePending = MAC_FRAMEPENDING_RESET;
}

force_inline void MAC_SetFramePending(MAC_Frame *F) {
  F->FrameControl.FramePending = MAC_FRAMEPENDING_SET;
}

// Frame Type (Frame Control)
force_inline void MAC_SetFrameType(MAC_Frame *F, MAC_FrameType Type) {
  F->FrameControl.FrameType = Type;
}

// Sequence number
force_inline void MAC_SetFrameSequence(MAC_Frame *F, uint8_t Seq) {
  F->Sequence = Seq;
}

// Command Frame-specific setters
force_inline void MAC_SetFrameCmdAssocRequest(MAC_FrameCommand *C) {
  C->CommandId = MAC_COMMAND_ID_ASSOC_REQUEST;
}

force_inline void MAC_SetFrameCmdAssocResponse(MAC_FrameCommand *C,
                                               uint16_t Address,
                                               MAC_FrameAssocStatus Status) {
  C->CommandId = MAC_COMMAND_ID_ASSOC_RESPONSE;
  C->ShortAddress = Address;
  C->AssocStatus = Status;
}

force_inline void MAC_SetFrameCmdDataRequest(MAC_FrameCommand *C) {
  C->CommandId = MAC_COMMAND_ID_DATA_REQUEST;
}

force_inline void MAC_SetFrameCmdDiscoverRequest(MAC_FrameCommand *C) {
  C->CommandId = MAC_COMMAND_ID_DISCOVER_REQUEST;
}

force_inline void MAC_SetFrameCmdDiscoverResponse(MAC_FrameCommand *C,
                                                  uint16_t ShortAddress,
                                                  uint32_t ExtendedAddress) {
  C->CommandId = MAC_COMMAND_ID_DISCOVER_RESPONSE;
  C->ShortAddress = ShortAddress;
  C->ExtendedAddress = ExtendedAddress;
}

#endif // __MAC_FRAME_H__
