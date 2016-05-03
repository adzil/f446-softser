#include <mac.h>

_inline_ uint16_t MAC_AddressLength(MAC_FrameTypeDef *Frame) {
  return (Frame->FrameControl.AddressMode == MAC_ADDRESS_SHORT) ? 2 : 4;
}

_inline uint16_t MAC_FrameLength(MAC_FrameTypeDef *Frame) {
  return MAC_AddressLength(Frame) + Frame->PayloadLen + 4;
}

MAC_Status MAC_FrameDecode(MAC_FrameTypeDef *Frame, uint8_t *Data,
                           uint16_t Length) {
  // Check for data error
  if (CRC_Checksum(Data, Length))
    return MAC_INVALID_CHECKSUM;
  // Save the data pointer
  Frame->DataPtr = Data;
  // Decode the frame control and sequence number
  Frame->FrameControl = *((MAC_FrameControlTypeDef *)(Data++));
  Frame->Sequence = *(Data++);
  // Get the address
  if (Frame->FrameControl.AddressMode == MAC_ADDRESS_SHORT) {
    Frame->Address.Short = __REV16(*((uint16_t *) Data));
    Data += 2;
  } else {
    Frame->Address.Extended = __REV(*((uint32_t *) Data));
    Data += 4;
  }
  // Get the data
  Frame->PayloadLen = Length - MAC_AddressLength(Frame) - 4;
  Frame->Payload = Data;

  return MAC_OK;
}

MAC_Status MAC_FrameEncode(MAC_FrameTypeDef *Frame, uint8_t *Data) {
  // Save the data pointer
  Frame->DataPtr = Data;
  // Encode the frame control and sequence number
  *((MAC_FrameControlTypeDef *)(Data++)) = Frame->FrameControl;
  *(Data++) = Frame->Sequence;
  // Set the address
  if (Frame->FrameControl.AddressMode == MAC_ADDRESS_SHORT) {
    *((uint16_t *) Data) = __REV16(Frame->Address.Short);
    Data += 2;
  } else {
    *((uint32_t *) Data) = __REV(Frame->Address.Extended);
    Data += 4;
  }
  // Copy the data
  memcpy(Data, Frame->Payload, Frame->PayloadLen);
  Data += Frame->PayloadLen;
  // Save the checksum
  *((uint16_t *) Data) = __REV16(CRC_Checksum(Data,
                                              MAC_FrameLength(Frame) - 2));
}