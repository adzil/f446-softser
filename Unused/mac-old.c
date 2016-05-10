#include <mac.h>
#include "random.h"

MAC_Handle MAC;

QUE_Item MAC_RXQueueItem[MAC_QUEUE_SIZE], MAC_TXQueueItem[MAC_QUEUE_SIZE];
MAC_RawData MAC_RXQueueData[MAC_QUEUE_SIZE];
MAC_Frame MAC_TXQueueData[MAC_QUEUE_SIZE];
MAC_FrameList MAC_FrameListMEM[MAC_QUEUE_SIZE];
MAC_DeviceAddress MAC_DeviceAddressMEM[MAC_DEVICE_ADDRESS_SIZE];

void MAC_FrameListInit(void) {
  int i;
  MAC_FrameList *List;

  List = MAC_FrameListMEM;

  MAC.FrameList.Free = NULL;
  MAC.FrameList.List = NULL;

  for (i = 0; i < MAC_QUEUE_SIZE; i++) {
    List->Next = MAC.FrameList.Free;
    MAC.FrameList.Free = List++;
  }
}

void MAC_FrameListPut(MAC_Frame *F) {
  MAC_FrameList *List;

  if (!MAC.FrameList.Free) return;

  LOCK_Start(&MAC.FrameList.Lock);
  List = MAC.FrameList.Free;
  MAC.FrameList.Free = List->Next;
  List->Frame = F;
  List->Next = MAC.FrameList.List;
  MAC.FrameList.List = List;
  LOCK_End(&MAC.FrameList.Lock);
}

MAC_Frame *MAC_FrameListSearch(MAC_AddressType Address, MAC_AddressMode Mode) {
  MAC_FrameList *List, *PrevList;
  MAC_Frame *F;

  if (!MAC.FrameList.List) return NULL;

  LOCK_Start(&MAC.FrameList.Lock);
  List = MAC.FrameList.List;
  PrevList = List;
  F = NULL;

  do {
    if (Mode == MAC_ADDRESS_SHORT) {
      if (Address.Short == List->Frame->Address.Destination.Short)
        break;
    } else {
      if (Address.Extended == List->Frame->Address.Destination.Extended)
        break;
    }
    // Continue iteration
    PrevList = List;
    List = List->Next;
  } while (List);

  // Pull out frame
  if (List) {
    PrevList->Next = List->Next;
    F = List->Frame;
    List->Next = MAC.FrameList.Free;
    MAC.FrameList.Free = List;
  }

  LOCK_End(&MAC.FrameList.Lock);

  return F;
}

_inline_ uint16_t MAC_AddressLength(MAC_AddressMode Mode) {
  if (Mode == MAC_ADDRESS_SHORT)
    return sizeof(uint16_t);
  else if (Mode == MAC_ADDRESS_EXTENDED)
    return sizeof(uint32_t);
  else
    return 0;
}

MAC_Status MAC_DevicesCheck(uint16_t Address) {
  int i;

  for (i = 0; i < MAC_DEVICE_ADDRESS_SIZE; i++) {
    if (MAC.Devices[i].ShortAddress == Address)
      return MAC_OK;
  }

  return MAC_INVALID_DEVICE_ADDRESS;
}

// TODO: Create mechanism to delete unusued devices
uint16_t MAC_DevicesAdd(uint32_t ExtendedAddress) {
  uint16_t Address;
  int i;

  // Checks if the extended address is already in the list
  for (i = 0; i < MAC_DEVICE_ADDRESS_SIZE; i++) {
    if (MAC.Devices[i].ExtendedAddress == ExtendedAddress)
      return MAC.Devices[i].ShortAddress;
  }

  // Generate new short address
  do {
    Address = (RND_Get() << 8) | RND_Get();
  } while (MAC_DevicesCheck(Address) == MAC_OK);

  // Store in address list
  for (i = 0; i < MAC_DEVICE_ADDRESS_SIZE; i++) {
    if (MAC.Devices[i].ShortAddress == MAC_ADDRESS_UNKNOWN) {
      MAC.Devices[i].ShortAddress = Address;
      MAC.Devices[i].ExtendedAddress = ExtendedAddress;
      return Address;
    }
  }

  // No more address list space
  return MAC_ADDRESS_UNKNOWN;
}

MAC_Status MAC_DataFrameAlloc(MAC_Frame *F, uint16_t PayloadLen) {
  F->Payload.Data = MEM_Alloc(PayloadLen);
  if (!F->Payload.Data) {
    return MAC_MEM_NOT_AVAIL;
  }
  F->Payload.Length = PayloadLen;

  return MAC_OK;
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
    if (MAC_DataFrameAlloc(F, PayloadLen) != MAC_OK) {
      MEM_Free(F);
      return NULL;
    }
  }
  // Zero out the start payload
  F->Payload.Start = 0;

  return F;
}

MAC_Status MAC_DataFrameFree(MAC_Frame *F) {
  // Free the payload data
  if (F->FrameControl.FrameType == MAC_FRAME_TYPE_DATA &&
      F->Payload.Length > 0) {
    MEM_Free(F->Payload.Data);
  }

  return MAC_OK;
}

// This function will free the MAC_Frame data
MAC_Status MAC_FrameFree(MAC_Frame *F) {
  MAC_DataFrameFree(F);
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
  // Send sequence number based on the frame type
  // The data and command frame will use DSN and beacon frame will use BSN
  // ACK frame should be loaded manually using to be acknowledged frame
  switch (F->FrameControl.FrameType) {
    case MAC_FRAME_TYPE_COMMAND:
    case MAC_FRAME_TYPE_DATA:
      ByteToBuffer(Data, &MAC.PIB.DSN);
      MAC.PIB.DSN++;
      break;

    case MAC_FRAME_TYPE_BEACON:
      ByteToBuffer(Data, &MAC.PIB.BSN);
      MAC.PIB.BSN++;
      break;

    case MAC_FRAME_TYPE_ACK:
      ByteToBuffer(Data, &F->Sequence);
  }

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

MAC_Status MAC_CheckFrameAddressing(MAC_Frame *F) {
  switch (F->FrameControl.FrameType) {
    case MAC_FRAME_TYPE_DATA:
    case MAC_FRAME_TYPE_COMMAND:
      // Check for destination address
      switch (F->FrameControl.DestinationAddressMode) {
        // Frame is using destination short address
        case MAC_ADDRESS_SHORT:
          // Only use short address if the device had one
          if (MAC.PIB.ShortAddress != MAC_ADDRESS_UNKNOWN &&
              MAC.PIB.ShortAddress != MAC_USE_EXTENDED_ADDRESS)
            if (F->Address.Destination.Short == MAC.PIB.ShortAddress)
              return MAC_OK;
          // Not good, return addressing error
          return MAC_INVALID_ADDRESSING;

        case MAC_ADDRESS_EXTENDED:
          // Checks if the extended address is same with the device
          if (F->Address.Destination.Extended == MAC_EXTENDED_ADDRESS)
            return MAC_OK;
          // Ooops invalid address
          return MAC_INVALID_ADDRESSING;

        default:
          break;
      }

      // No destination address, this is special case only for coordinator
      if (MAC.PIB.VPANCoordinator == MAC_VPAN_COORDINATOR) {
        // Check the source address
        switch (F->FrameControl.SourceAddressMode) {
          case MAC_ADDRESS_EXTENDED:
            // Always allow extended source address
            return MAC_OK;

          case MAC_ADDRESS_SHORT:
            // The source short address must be validated against the device
            // address list
            if (MAC_DevicesCheck(F->Address.Source.Short) == MAC_OK)
              return MAC_OK;
            // Invalid source short address, oops
            break;

          default:
            // WTF is this unknown frame
            break;
        }
      }
      // Addressing error
      return MAC_INVALID_ADDRESSING;

    case MAC_FRAME_TYPE_ACK:
      // Always allow acknowledgement frame
      return MAC_OK;

    case MAC_FRAME_TYPE_BEACON:
      // Only allow beacon on device only
      if (MAC.PIB.VPANCoordinator == MAC_VPAN_DEVICE)
        return MAC_OK;

      return MAC_INVALID_ADDRESSING;
  }

  // OMG what is this frame
  return MAC_INVALID_ADDRESSING;
}

void MAC_AckSend(MAC_Frame *SrcFrame) {
  MAC_Frame *F;
  uint8_t *Data;
  uint16_t Length;

  // Create new acknowledgement frame
  F = MAC_FrameAlloc(0);
  if (!F) return;
  F->FrameControl.AckRequest = MAC_NO_ACK;
  F->FrameControl.DestinationAddressMode = MAC_NO_ADDRESS;
  F->FrameControl.SourceAddressMode = MAC_NO_ADDRESS;
  F->FrameControl.FrameType = MAC_FRAME_TYPE_ACK;
  if (MAC.PIB.VPANCoordinator == MAC_VPAN_COORDINATOR) {
    // TODO: Check pending list
    F->FrameControl.FramePending = MAC_FRAME_PENDING;
  } else {
    F->FrameControl.FramePending = MAC_FRAME_NOT_PENDING;
  }
  F->Sequence = SrcFrame->Sequence;
  // Encode the frame
  Length = MAC_FrameEncodeLen(F);
  Data = MEM_Alloc(Length);
  // Cannot allocate data buffer
  if (!Data) return;
  // Encode frame
  MAC_FrameEncode(F, Data);
  MAC_FrameFree(F);
  // Send ack frame
  PHY_API_SendStart(MAC.Transmission.Data, MAC.Transmission.Length);
  // Free the data buffer
  MEM_Free(Data);
}

void MAC_AckReceived(MAC_Frame *Ack) {
  MAC_Frame *F;

  // No waiting frame, just go on
  if (MAC.Transmission.Status != MAC_TRANSMISSION_WAIT_ACK) return;
  // Load the mac frame from item
  F = (MAC_Frame *) MAC.Transmission.Item->Data;
  // Checks for sequence number
  if (F->Sequence == Ack->Sequence) {
    // TODO: Acknowledge success callback
    // Reset the transmission
    MAC.Transmission.Status = MAC_TRANSMISSION_WAIT_RESET;
  }
}

void MAC_DirectTransmitFrame(MAC_Frame *F) {
  QUE_Item *Item;
  MAC_Frame *Dst;

  Item = QUE_Put(&MAC.Queue.TX);
  if (!Item) return;

  Dst = (MAC_Frame *) Item->Data;


}

void MAC_TransmitFrame(MAC_Frame *F) {
  switch (MAC.PIB.VPANCoordinator) {
    case MAC_VPAN_COORDINATOR:
      // Put to the frame list pool
      MAC_FrameListPut(F);
      break;

    case MAC_VPAN_DEVICE:
      // Send directly
      break;
  }
}

void MAC_PollTransmission(void) {
  QUE_Item *Item;
  MAC_Frame *F;

  // Load the frame
  F = (MAC_Frame *) MAC.Transmission.Item->Data;

  switch (MAC.Transmission.Status) {
    case MAC_TRANSMISSION_WAIT_RESET:
      QUE_GetCommit(&MAC.Queue.TX, MAC.Transmission.Item);
      MAC.Transmission.Status = MAC_TRANSMISSION_RESET;

    case MAC_TRANSMISSION_RESET:
      // Try to get item in queue
      Item = QUE_Get(&MAC.Queue.TX);
      // If there is no queue then there's nothing to do
      if (!Item) return;
      // Load the item to the transmission item
      MAC.Transmission.Item = Item;
      // Reload retry count
      MAC.Transmission.Retries = 0;
      // Encode the frame
      MAC.Transmission.Length = MAC_FrameEncodeLen(F);
      MAC.Transmission.Data = MEM_Alloc(MAC.Transmission.Length);
      // Cannot allocate data buffer
      if (!MAC.Transmission.Data) {
        MAC.Transmission.Status = MAC_TRANSMISSION_WAIT_RESET;
        return;
      }
      MAC_FrameEncode(F, MAC.Transmission.Data);
      MAC_DataFrameFree(F);

    case MAC_TRANSMISSION_RETRY:
      // Randomize transfer time
      MAC.Transmission.TickStart = HAL_GetTick();
      MAC.Transmission.TickLength = MAC_BACKOFF_DURATION;
      MAC.Transmission.Status = MAC_TRANSMISSION_BACKOFF;

    default:
      break;
  }

  // Check if the the timer has elapsed
  if (MAC.Transmission.TickLength <
      HAL_GetTick() - MAC.Transmission.TickStart) {
    // Update the tick start
    MAC.Transmission.TickStart = HAL_GetTick();

    switch (MAC.Transmission.Status) {
      case MAC_TRANSMISSION_BACKOFF:
        // TODO: Send the data
        PHY_API_SendStart(MAC.Transmission.Data, MAC.Transmission.Length);
        // Checks if the data transmission needs acknowledgement
        if (F->FrameControl.AckRequest == MAC_ACK) {
          // Set new tick length value for wait duration
          MAC.Transmission.TickLength = MAC_ACK_WAIT_DURATION;
          MAC.Transmission.Status = MAC_TRANSMISSION_WAIT_ACK;
        } else {
          // Reset the transmission
          MAC.Transmission.Status = MAC_TRANSMISSION_WAIT_RESET;
        }
        break;

      case MAC_TRANSMISSION_WAIT_ACK:
        // Check for number of retries
        if (++MAC.Transmission.Retries >= MAC_ACK_MAX_RETRIES) {
          // TODO: Acknowledge callback error
          // Reset the transmission
          MAC.Transmission.Status = MAC_TRANSMISSION_WAIT_RESET;
        } else {
          // Retry the transmission
          MAC.Transmission.Status = MAC_TRANSMISSION_RETRY;
        }
        break;

      default:
        break;
    }
  }
}

void MAC_CreateAssociationRequest(void) {
  QUE_Item *Item;
  MAC_Frame *F;

  Item = QUE_Put(&MAC.Queue.TX);
  if (!Item) return;

  F = (MAC_Frame *) Item->Data;

  F->FrameControl.FrameType = MAC_FRAME_TYPE_COMMAND;
  F->FrameControl.FramePending = MAC_FRAME_NOT_PENDING;
  F->FrameControl.AckRequest = MAC_ACK;
  F->FrameControl.DestinationAddressMode = MAC_ADDRESS_EXTENDED;
  F->FrameControl.SourceAddressMode = MAC_ADDRESS_EXTENDED;

  F->Address.Destination.Extended = MAC.PIB.CoordExtendedAddress;
  F->Address.Source.Extended = MAC_EXTENDED_ADDRESS;

  F->Payload.Command.CommandFrameId = MAC_COMMAND_ASSOC_REQUEST;

  QUE_PutCommit(&MAC.Queue.TX, Item);

}

void MAC_CreateAssociationResponse(MAC_Frame *Src) {
  QUE_Item *Item;
  MAC_Frame *F;

  Item = QUE_Put(&MAC.Queue.TX);
  if (!Item) return;

  F = (MAC_Frame *) Item->Data;

  F->FrameControl.FrameType = MAC_FRAME_TYPE_COMMAND;
  F->FrameControl.FramePending = MAC_FRAME_NOT_PENDING;
  F->FrameControl.AckRequest = MAC_ACK;
  F->FrameControl.DestinationAddressMode = MAC_ADDRESS_EXTENDED;
  F->FrameControl.SourceAddressMode = MAC_ADDRESS_EXTENDED;

  F->Address.Destination.Extended = Src->Address.Destination.Extended;
  F->Address.Source.Extended = MAC_EXTENDED_ADDRESS;

  F->Payload.Command.CommandFrameId = MAC_COMMAND_ASSOC_RESPONSE;

  // Create
  F->Payload.Command.ShortAddress

  QUE_PutCommit(&MAC.Queue.TX, Item);

}

void MAC_API_FrameReceived(uint8_t *Data, uint16_t Len) {
  MAC_Frame *F;

  // Decode the frame
  F = MAC_FrameDecode(Data, Len);
  // Do nothing if frame is not decoded properly
  if (!F) return;

  // Check for MAC addressing
  if (MAC_CheckFrameAddressing(F) == MAC_OK) {
    // TODO: Move the bottom one to the top

  }

  // Check for acknowledgement request
  switch (F->FrameControl.FrameType) {
    case MAC_FRAME_TYPE_DATA:
    case MAC_FRAME_TYPE_COMMAND:
      if (F->FrameControl.AckRequest == MAC_ACK) {
        // Send back acknowledgement frame
        MAC_AckSend(F);
      }
      break;

    default:
      break;
  }

  // Populate MAC frame by frame type
  switch (F->FrameControl.FrameType) {
    case MAC_FRAME_TYPE_DATA:
      // Should give data to next higher layer
      break;

    case MAC_FRAME_TYPE_ACK:
      MAC_AckReceived(F);
      break;

    case MAC_FRAME_TYPE_BEACON:
      // Check for VPAN coordinator field
      if (F->Payload.Beacon.VPANCoordinator == MAC_VPAN_COORDINATOR) {
        // Add to scan list (if the device is not coordinator)
        if (MAC.PIB.VPANCoordinator == MAC_VPAN_DEVICE) {

        }
      }
      break;

    case MAC_FRAME_TYPE_COMMAND:
      // Check for command input
      break;

    default:
      break;
  }

  // Clean up the frame buffer
  MAC_FrameFree(F);
}

void MAC_PIB_Reset(void) {
  // Preload MAC sequence
  MAC.PIB.DSN = RND_Get();
  MAC.PIB.BSN = RND_Get();
  // Reset association status
  MAC.PIB.AssociatedCoord = MAC_NOT_ASSOCIATED;
  // Reset coordinator address
  MAC.PIB.CoordShortAddress = MAC_ADDRESS_UNKNOWN;
  MAC.PIB.CoordExtendedAddress = 0;
  // Reset device address
  MAC.PIB.ShortAddress = MAC_ADDRESS_UNKNOWN;
  // Set device type
  MAC.PIB.VPANCoordinator = MAC_VPAN_COORDINATOR;
}

MAC_Status MAC_API_DataReceived(uint8_t *Data, uint16_t Length) {
  QUE_Item *Item;
  MAC_RawData *RawData;

  // Get the pool item
  Item = QUE_Put(&MAC.Queue.RX);
  if (!Item)
    return MAC_QUEUE_FULL;
  // Get the raw data handle
  RawData = (MAC_RawData *) Item->Data;
  // Fill the raw data
  RawData->Content = Data;
  RawData->Length = Length;
  // Commit the queue insertion
  QUE_PutCommit(&MAC.Queue.RX, Item);
}

void MAC_DevicesInit(void) {
  int i;

  MAC.Devices = MAC_DeviceAddressMEM;
  for (i = 0; i < MAC_DEVICE_ADDRESS_SIZE; i++) {
    MAC.Devices[i].ShortAddress = MAC_ADDRESS_UNKNOWN;
  }
}

void MAC_Init(void) {
  // Reset PIB values
  MAC_PIB_Reset();
  // Initialize queue
  QUE_InitData(MAC_RXQueueItem, MAC_RXQueueData, sizeof(MAC_RawData),
               MAC_QUEUE_SIZE);
  QUE_InitData(MAC_TXQueueItem, MAC_TXQueueData, sizeof(MAC_Frame),
               MAC_QUEUE_SIZE);
  QUE_Init(&MAC.Queue.RX, MAC_RXQueueItem, MAC_QUEUE_SIZE);
  QUE_Init(&MAC.Queue.TX, MAC_TXQueueItem, MAC_QUEUE_SIZE);
  // Initialize device address list
  MAC_DevicesInit();
  // Initialize mac frame list
  MAC_FrameListInit();
}