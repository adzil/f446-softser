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
  if (!F->Payload.Data) {
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
  FrameControl = *((MAC_FrameControl *)(Data++));
  // Calculate the address length
  Start = sizeof(MAC_FrameControl) +
      sizeof(uint8_t) +
      MAC_AddressLength(FrameControl.DestinationAddressMode) +
      MAC_AddressLength(FrameControl.SourceAddressMode);
  Length = Len - (Start + sizeof(uint16_t));
  // Check for length error
  if (Length < 0) {
    return NULL;
  }
  // No error on the data, continue allocating buffer
  F = MAC_FrameAlloc(Length);
  if (!F) return NULL;
  // Copy the start and length number
  F->Payload.Start = Start;
  F->Payload.Length = Length;
  // Copy the frame control and sequence number
  F->FrameControl = FrameControl;
  F->Sequence = *(Data++);
  // Get the address field
  if (F->FrameControl.DestinationAddressMode == MAC_ADDRESS_SHORT) {
    F->Address.Destination.Short = __REV16(*(uint16_t *) Data);
    Data += sizeof(uint16_t);
  } else if (F->FrameControl.DestinationAddressMode == MAC_ADDRESS_EXTENDED) {
    F->Address.Destination.Extended = __REV(*(uint32_t *) Data);
    Data += sizeof(uint32_t);
  } else {
    F->Address.Destination.Extended = 0;
  }
  if (F->FrameControl.SourceAddressMode == MAC_ADDRESS_SHORT) {
    F->Address.Source.Short = __REV16(*(uint16_t *) Data);
    Data += sizeof(uint16_t);
  } else if (F->FrameControl.SourceAddressMode == MAC_ADDRESS_EXTENDED) {
    F->Address.Source.Extended = __REV(*(uint32_t *) Data);
    Data += sizeof(uint32_t);
  } else {
    F->Address.Source.Extended = 0;
  }
  // Get payload data
  if (F->Payload.Length > 0) {
    memcpy(F->Payload.Data, Data, F->Payload.Length);
  }

  return F;
}

uint16_t MAC_FrameEncodeLen(MAC_Frame *F) {
  F->Payload.Start = sizeof(MAC_FrameControl) +
                     sizeof(uint8_t) +
                     MAC_AddressLength(F->FrameControl.DestinationAddressMode) +
                     MAC_AddressLength(F->FrameControl.SourceAddressMode);
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
  *((MAC_FrameControl *)(Data++)) = F->FrameControl;
  *(Data++) = F->Sequence;
  // Set addressing data
  if (F->FrameControl.DestinationAddressMode == MAC_ADDRESS_SHORT) {
    *((uint16_t *) Data) = __REV16(F->Address.Destination.Short);
    Data += sizeof(uint16_t);
  } else if (F->FrameControl.DestinationAddressMode == MAC_ADDRESS_EXTENDED) {
    *((uint32_t *) Data) = __REV(F->Address.Destination.Extended);
    Data += sizeof(uint32_t);
  } else {
    F->Address.Destination.Extended = 0;
  }
  if (F->FrameControl.SourceAddressMode == MAC_ADDRESS_SHORT) {
    *((uint16_t *) Data) = __REV16(F->Address.Source.Short);
    Data += sizeof(uint16_t);
  } else if (F->FrameControl.SourceAddressMode == MAC_ADDRESS_EXTENDED) {
    *((uint32_t *) Data) = __REV(F->Address.Source.Extended);
    Data += sizeof(uint32_t);
  } else {
    F->Address.Source.Extended = 0;
  }
  // Copy the payload data
  if (F->Payload.Length > 0) {
    memcpy(Data, F->Payload.Data, F->Payload.Length);
    Data += F->Payload.Length;
  }
  // Do the checksum
  Sum = CRC_Checksum(DataPtr, Len);
  *(uint16_t *) Data = __REV16(Sum);
  // Free the MAC_Frame
  MAC_FrameFree(F);

  return MAC_OK;
}