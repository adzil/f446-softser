#include <mac.h>

_inline_ uint16_t MAC_AddressLength(MAC_AddressMode Mode) {
  if (Mode == MAC_ADDRESS_SHORT)
    return sizeof(uint16_t);
  else if (Mode == MAC_ADDRESS_EXTENDED)
    return sizeof(uint32_t);
  else
    return 0;
}

// This function will create the MAC Frame
MAC_Frame *MAC_FrameAlloc(uint16_t PayloadLen) {
  MAC_Frame *F;

  // Allocate new frame
  F = MEM_Alloc(sizeof(MAC_Frame));
  if (!F)
    return NULL;
  // Allocate buffer for payload if required
  if (PayloadLen) {
    F->Payload.Data = MEM_Alloc(PayloadLen);
    if (!F->Payload.Data) {
      MEM_Free(F);
      return NULL;
    }
    F->Payload.Length = PayloadLen;
  } else {
    F->Payload.Data = NULL;
  }
  // Zero out the start payload
  F->Payload.Start = 0;

  return F;
}

// This function will free the MAC_Frame data
MAC_Status MAC_FrameFree(MAC_Frame *F) {
  // Free the payload data
  if (F->FrameControl.FrameType == MAC_FRAME_TYPE_DATA &&
      F->Payload.Length > 0) {
    MEM_Free(F->Payload.Data);
  }
  // Free the frame handle
  MEM_Free(F);

  return MAC_OK;
}

// This function will convert data stream from PHY to proper MAC_Frame format
MAC_Frame *MAC_FrameDecode(uint8_t *Data, uint16_t Len) {
  MAC_Frame *F;
  int Start, Length;
  MAC_FrameControl FrameControl;

  // FCS should be checked first before decode the data
  if (CRC_Checksum(Data, Len))
    return NULL;
  // Get the frame control
  ByteFromBuffer(&FrameControl, Data);
  // Calculate the address length
  Start = MemberSize(MAC_Frame, FrameControl) +
      MemberSize(MAC_Frame, Sequence) +
      MAC_AddressLength(FrameControl.DestinationAddressMode) +
      MAC_AddressLength(FrameControl.SourceAddressMode);
  Length = Len - (Start + sizeof(uint16_t));
  // Check for length error
  if (Length < 0) {
    return NULL;
  }
  // No error on the data, continue allocating buffer
  if (FrameControl.FrameType == MAC_FRAME_TYPE_DATA)
    F = MAC_FrameAlloc(Length);
  else
    F = MAC_FrameAlloc(0);

  if (!F) return NULL;
  // Copy the start and length number
  F->Payload.Start = Start;
  F->Payload.Length = Length;
  // Copy the frame control and sequence number
  F->FrameControl = FrameControl;
  ByteFromBuffer(&F->Sequence, Data);
  // Get the address field
  switch(F->FrameControl.DestinationAddressMode) {
    case MAC_ADDRESS_SHORT:
      WordFromBuffer(&F->Address.Destination.Short, Data);
      break;
    case MAC_ADDRESS_EXTENDED:
      DwordFromBuffer(&F->Address.Destination.Extended, Data);
      break;
    default:
      break;
  }

  switch(F->FrameControl.SourceAddressMode) {
    case MAC_ADDRESS_SHORT:
      WordFromBuffer(&F->Address.Source.Short, Data);
      break;
    case MAC_ADDRESS_EXTENDED:
      DwordFromBuffer(&F->Address.Source.Extended, Data);
      break;
    default:
      break;
  }

  // Get payload data
  switch (F->FrameControl.FrameType) {
    case MAC_FRAME_TYPE_BEACON:
      ByteFromBuffer(&F->Payload.Beacon, Data);
      break;

    case MAC_FRAME_TYPE_COMMAND:
      ByteFromBuffer(&F->Payload.Command.CommandFrameId, Data);
      switch (F->Payload.Command.CommandFrameId) {
        case MAC_COMMAND_ASSOC_RESPONSE:
          WordFromBuffer(&F->Payload.Command.ShortAddress, Data);
          ByteFromBuffer(&F->Payload.Command.Status, Data);
          break;

        default:
          break;
      }
      break;

    case MAC_FRAME_TYPE_DATA:
      if (F->Payload.Length > 0)
        memcpy(F->Payload.Data, Data, F->Payload.Length);
      break;

    default:
      break;
  }

  return F;
}

uint16_t MAC_FrameEncodeLen(MAC_Frame *F) {
  uint16_t PayloadLength;

  F->Payload.Start = MemberSize(MAC_Frame, FrameControl) +
                     MemberSize(MAC_Frame, Sequence) +
                     MAC_AddressLength(F->FrameControl.DestinationAddressMode) +
                     MAC_AddressLength(F->FrameControl.SourceAddressMode);

  switch (F->FrameControl.FrameType) {
    case MAC_FRAME_TYPE_BEACON:
      PayloadLength = 1;
      break;

    case MAC_FRAME_TYPE_ACK:
      PayloadLength = 0;
      break;

    case MAC_FRAME_TYPE_COMMAND:
      if (F->Payload.Command.CommandFrameId == MAC_COMMAND_ASSOC_RESPONSE)
        PayloadLength = 4;
      else
        PayloadLength = 1;
      break;

    case MAC_FRAME_TYPE_DATA:
      PayloadLength = F->Payload.Length;
      break;

    default:
      PayloadLength = 0;
      break;
  }

  F->Payload.Length = PayloadLength;

  return F->Payload.Start + F->Payload.Length + sizeof(uint16_t);
}

// This function will convert MAC_Frame to data stream
MAC_Status MAC_FrameEncode(MAC_Frame *F, uint8_t *Data) {
  uint16_t Len, Sum;
  uint8_t *DataPtr;

  // Check if the user has run the encode buffer length generator
  if (F->Payload.Start == 0)
    return MAC_INVALID_ROUTINE;
  // Store the data pointer
  DataPtr = Data;
  Len = F->Payload.Start + F->Payload.Length;
  // Start writing frame control and sequence number
  ByteToBuffer(Data, &F->FrameControl);
  ByteToBuffer(Data, &F->Sequence);

  // Set addressing data
  switch(F->FrameControl.DestinationAddressMode) {
    case MAC_ADDRESS_SHORT:
      WordToBuffer(Data, &F->Address.Destination.Short);
      break;
    case MAC_ADDRESS_EXTENDED:
      DwordToBuffer(Data, &F->Address.Destination.Extended);
      break;
    default:
      break;
  }

  switch(F->FrameControl.SourceAddressMode) {
    case MAC_ADDRESS_SHORT:
      WordToBuffer(Data, &F->Address.Source.Short);
      break;
    case MAC_ADDRESS_EXTENDED:
      DwordToBuffer(Data, &F->Address.Source.Extended);
      break;
    default:
      break;
  }

  switch (F->FrameControl.FrameType) {
    case MAC_FRAME_TYPE_BEACON:
      ByteToBuffer(Data, &F->Payload.Beacon);
      break;

    case MAC_FRAME_TYPE_COMMAND:
      ByteToBuffer(Data, &F->Payload.Command.CommandFrameId);
      if (F->Payload.Command.CommandFrameId == MAC_COMMAND_ASSOC_RESPONSE) {
        WordToBuffer(Data, &F->Payload.Command.ShortAddress);
        ByteToBuffer(Data, &F->Payload.Command.Status);
      }
      break;

    case MAC_FRAME_TYPE_DATA:
      if (F->Payload.Length > 0) {
        memcpy(Data, F->Payload.Data, F->Payload.Length);
        Data += F->Payload.Length;
      }
      break;

    default:
      break;
  }

  // Do the checksum
  Sum = CRC_Checksum(DataPtr, Len);
  WordToBuffer(Data, &Sum);
  // Free the MAC_Frame
  MAC_FrameFree(F);

  return MAC_OK;
}

void MAC_API_FrameReceived(uint8_t *Data, uint16_t Len) {
  MAC_Frame *F;

  // Decode the frame
  F = MAC_FrameDecode(Data, Len);
  // Do nothing if frame is not decoded properly
  if (!F) return;


}