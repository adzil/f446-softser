#include "mac-frame.h"

/* Encode the structured frame into data stream */
MAC_Status MAC_FrameEncode(MAC_Frame *F, uint8_t *Data, size_t *Len) {
  uint8_t *DataPtr;
  uint16_t Checksum;

  // Check for data availability
  if (!F) return MAC_STATUS_INVALID_PARAM;

  // Check for data length
  if (F->Payload.Length > MAC_CONFIG_MAX_PAYLOAD_LENGTH)
    return MAC_STATUS_INVALID_LENGTH;

  // Store the data pointer
  DataPtr = Data;
  // Copy the frame control and sequence
  MAC_WriteByte(Data, &F->FrameControl);
  MAC_WriteByte(Data, &F->Sequence);
  // Copy the address
  switch (F->FrameControl.DstAdrMode) {
    case MAC_ADRMODE_SHORT:
      MAC_WriteWord(Data, &F->Address.Dst.Short);
      break;
    case MAC_ADRMODE_EXTENDED:
      MAC_WriteDword(Data, &F->Address.Dst.Extended);
      break;
    default:
      break;
  }
  switch (F->FrameControl.SrcAdrMode) {
    case MAC_ADRMODE_SHORT:
      MAC_WriteWord(Data, &F->Address.Src.Short);
      break;
    case MAC_ADRMODE_EXTENDED:
      MAC_WriteDword(Data, &F->Address.Src.Extended);
      break;
    default:
      break;
  }

  // Copy the payload data, if available
  if (F->Payload.Length > 0) {
    MAC_WriteStream(Data, F->Payload.Data, F->Payload.Length);
  }

  // Calculate checksum
  Checksum = CRC_Checksum(DataPtr, (size_t)(Data - DataPtr));
  MAC_WriteWord(Data, &Checksum);

  *Len = Data - DataPtr;

  return MAC_STATUS_OK;
}

/* Decodes the data stream into structured frame */
MAC_Status MAC_FrameDecode(MAC_Frame *F, uint8_t *Data, size_t Len) {
  size_t StartLen;

  // Check for frame buffer length
  if (Len > MAC_CONFIG_MAX_FRAME_BUFFER)
    return MAC_STATUS_INVALID_LENGTH;
  // Check the checksum first before process the data
  if (CRC_Checksum(Data, Len)) return MAC_STATUS_INVALID_CHECKSUM;
  // Get frame control and byte sequence
  MAC_ReadByte(&F->FrameControl, Data);
  MAC_ReadByte(&F->Sequence, Data);

  // Update the payload info
  StartLen = MAC_GetFrameAddressSize(F->FrameControl.DstAdrMode) +
             MAC_GetFrameAddressSize(F->FrameControl.SrcAdrMode) + 2;
  // Check for length issues
  if (StartLen + 2 > Len) return MAC_STATUS_INVALID_LENGTH;
  // Calculate the payload length (Total - Start - 2byte FCS)
  F->Payload.Length = Len - StartLen - 2;

  // Get the address
  switch (F->FrameControl.DstAdrMode) {
    case MAC_ADRMODE_SHORT:
      MAC_ReadWord(&F->Address.Dst.Short, Data);
      break;
    case MAC_ADRMODE_EXTENDED:
      MAC_ReadDword(&F->Address.Dst.Extended, Data);
      break;
    default:
      break;
  }
  switch (F->FrameControl.SrcAdrMode) {
    case MAC_ADRMODE_SHORT:
      MAC_ReadWord(&F->Address.Src.Short, Data);
      break;
    case MAC_ADRMODE_EXTENDED:
      MAC_ReadDword(&F->Address.Src.Extended, Data);
      break;
    default:
      break;
  }

  // Get the data (if there is data available)
  if (F->Payload.Length > 0) {
    // Copy the payload contents
    MAC_ReadStream(F->Payload.Data, Data, F->Payload.Length);
  }

  return MAC_STATUS_OK;
}

void MAC_FrameCommandEncode(MAC_Frame *F, MAC_FrameCommand *C) {
  uint8_t *Data;

  // Set data pointer
  Data = F->Payload.Data;

  // Get the command ID
  MAC_WriteByte(Data, &C->CommandId);
  F->Payload.Length = 1;
  // Check for command ID
  switch (C->CommandId) {
    case MAC_COMMAND_ID_ASSOC_RESPONSE:
      MAC_WriteWord(Data, &C->ShortAddress);
      MAC_WriteByte(Data, &C->AssocStatus);
      break;

    case MAC_COMMAND_ID_DISCOVER_RESPONSE:
      MAC_WriteWord(Data, &C->ShortAddress);
      MAC_WriteDword(Data, &C->ExtendedAddress);
      break;

    default:
      break;
  }

  // Calculate payload length
  F->Payload.Length = Data - F->Payload.Data;
}

void MAC_FrameCommandDecode(MAC_Frame *F, MAC_FrameCommand *C) {
  uint8_t *Data;

  // Set data pointer
  Data = F->Payload.Data;
  // Zero out existing data on command frame
  C->CommandId = MAC_COMMAND_ID_ASSOC_REQUEST;
  C->AssocStatus = MAC_ASSOCSTATUS_SUCCESS;

  // Get the command ID
  MAC_ReadByte(&C->CommandId, Data);
  // Check for command ID
  switch (C->CommandId) {
    case MAC_COMMAND_ID_ASSOC_RESPONSE:
      MAC_ReadWord(&C->ShortAddress, Data);
      MAC_ReadByte(&C->AssocStatus, Data);
      break;

    case MAC_COMMAND_ID_DISCOVER_RESPONSE:
      MAC_ReadWord(&C->ShortAddress, Data);
      MAC_ReadDword(&C->ExtendedAddress, Data);
      break;

    default:
      break;
  }
}
