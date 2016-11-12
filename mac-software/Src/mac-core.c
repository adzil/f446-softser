#include <mac-frame.h>
#include "mac-core.h"

void MAC_GenFrameAck(MAC_Frame *SF) {
  MAC_Frame F;
  uint8_t Data[MAC_CONFIG_MAX_FRAME_BUFFER];
  size_t Length;

  if (SF->FrameControl.AckRequest == MAC_ACKREQUEST_SET &&
      SF->FrameControl.FrameType != MAC_FRAMETYPE_ACK) {
    MAC_SetFrameNoDstAdr(&F);
    MAC_SetFrameNoSrcAdr(&F);
    MAC_SetFrameSequence(&F, SF->Sequence);
    MAC_SetFrameNoAckRequest(&F);
    MAC_SetFrameNoPending(&F);
    MAC_SetFrameType(&F, MAC_FRAMETYPE_ACK);
    F.Payload.Length = 0;

    MAC_FrameEncode(&F, Data, &Length);
    PHY_API_SendStart(Data, Length);

    //MAC_TransmitPutFrame(H, F);
  }
}

void MAC_GenTxData(MAC_Instance *H, uint8_t *Data, size_t Length) {
  MAC_Frame *F;

  F = MAC_MemFrameAlloc(&H->Mem);
  if(!F) return;

  MAC_SetFrameNoDstAdr(F);
  MAC_GenFrameSrcAdr(H, F);
  MAC_GenFrameSequence(H, F);
  MAC_SetFrameAckRequest(F);
  MAC_SetFrameNoPending(F);
  MAC_SetFrameType(F, MAC_FRAMETYPE_DATA);
  memcpy(F->Payload.Data, Data, Length);
  F->Payload.Length = Length;

  MAC_TransmitPutFrame(H, F);
}

void MAC_CoreFrameReceived(MAC_Instance *H, uint8_t *Data, size_t Length) {
  MAC_Frame *F;
  //int i;

  // Abort frame decoding on zero-length
  if (!Length) return;

  // Allocate new frame
  F = MAC_MemFrameAlloc(&H->Mem);
  if (!F) return;

  // Convert received data to frame structure
  if (MAC_FrameDecode(F, Data, Length) != MAC_STATUS_OK) {
    MAC_MemFrameFree(&H->Mem, F);
    return;
  }

  // Check frame addressing
  if (MAC_CoreCheckAddressing(H, F) != MAC_STATUS_OK) {
    MAC_MemFrameFree(&H->Mem, F);
    return;
  }
  // Check if the packet need to be acknowledged
  MAC_GenFrameAck(F);

  // Process the packet
  switch (F->FrameControl.FrameType) {
    case MAC_FRAMETYPE_DATA:
/*
      Log("Data from ");
      switch (F->FrameControl.SrcAdrMode) {
        case MAC_ADRMODE_NOT_PRESENT:
          Log("Coord.");
          break;

        case MAC_ADRMODE_SHORT:
          sprintf(MAC_TermBuf, "[S: %x]", F->Address.Src.Short);
          Log(MAC_TermBuf);
          break;

        case MAC_ADRMODE_EXTENDED:
          sprintf(MAC_TermBuf, "[X: %x]", F->Address.Src.Extended);
          Log(MAC_TermBuf);
          break;
      }
      Log(" PAYLOAD: ");
      for (i = 0; i < F->Payload.Length; i++) {
        sprintf(MAC_TermBuf, "%2x ", F->Payload.Data[i]);
        Log(MAC_TermBuf);
      }
      Log("\r\n");*/
      UARTData(F->Payload.Data, F->Payload.Length);
      break;

    case MAC_FRAMETYPE_COMMAND:
      MAC_CmdFrameHandler(H, F);
      break;
/*
    case MAC_FRAMETYPE_ACK:
      // Check for Back-Ack
      //MAC_CoreFrameBackAck(H, F);
      break;*/

    default:
      break;
  }

#ifdef MAC_DEBUG
  MAC_DebugFrame(H, F, MAC_DEBUG_RCV);
#endif

  MAC_MemFrameFree(&H->Mem, F);
}

MAC_Status MAC_CoreFrameBackAck(MAC_Instance *H, MAC_Frame *F) {
  LOCK_Start(&H->Tx.Lock);

  if (H->Tx.Retries == 0) {
    LOCK_End(&H->Tx.Lock);
    return MAC_STATUS_INVALID_DESTINATION;
  }

  if (H->Tx.Sequence == F->Sequence) {
    H->Tx.Retries = 0;
    H->Tx.FailedAck = 0;
  } else {
		LOCK_End(&H->Tx.Lock);
		return MAC_STATUS_INVALID_DESTINATION;
	}

  LOCK_End(&H->Tx.Lock);
	
	return MAC_STATUS_OK;
}

MAC_Status MAC_CoreFrameSend(MAC_Instance *H, uint8_t **Data, size_t *Len) {
  MAC_Frame *F;
  MAC_Status Ret = MAC_STATUS_OK;

  LOCK_Start(&H->Tx.Lock);

  if (H->Tx.Retries == 0) {
    F = MAC_TransmitGetFrame(H);
    if (!F) {
      LOCK_End(&H->Tx.Lock);
      return MAC_STATUS_NO_DATA;
    }

    if (F->FrameControl.AckRequest == MAC_ACKREQUEST_SET)
      H->Tx.Retries = MAC_CONFIG_MAX_RETRIES;
    else
      Ret = MAC_STATUS_NO_ACK;

    if (F->FrameControl.FrameType == MAC_FRAMETYPE_ACK)
      Ret = MAC_STATUS_NO_DELAY;

    H->Tx.Sequence = F->Sequence;

#ifdef MAC_DEBUG
    MAC_DebugFrame(H, F, MAC_DEBUG_SND);
#endif

    MAC_FrameEncode(F, H->Tx.Data, &H->Tx.Length);
    MAC_MemFrameFree(&H->Mem, F);
  }

  *Data = H->Tx.Data;
  *Len = H->Tx.Length;

  if (H->Tx.Retries > 0)
    if (--H->Tx.Retries == 0)
      if (++H->Tx.FailedAck >= MAC_CONFIG_MAX_ACK_ERROR) {
        H->Tx.FailedAck = 0;
				if (H->Pib.VpanCoordinator == MAC_PIB_VPAN_DEVICE) {
					H->Pib.AssociatedCoord = MAC_PIB_ASSOCIATED_RESET;
					H->Pib.CoordExtendedAdr = 0;
					H->Pib.CoordShortAdr = MAC_CONST_BROADCAST_ADDRESS;
          __GPIO_WRITE(GPIOA, 5, GPIO_PIN_RESET);
					Log("Disconnected from coord.\r\n");
				}
      }

  LOCK_End(&H->Tx.Lock);

  return Ret;
}

MAC_Status MAC_CoreCheckAddressing(MAC_Instance *H, MAC_Frame *F) {
  MAC_FrameCommand C;

  // Ack frame only
  if (F->FrameControl.FrameType == MAC_FRAMETYPE_ACK) {
    return MAC_CoreFrameBackAck(H, F);
  }

  // Check the destination addressing
  switch (F->FrameControl.DstAdrMode) {
    case MAC_ADRMODE_NOT_PRESENT:
      if (H->Pib.VpanCoordinator == MAC_PIB_VPAN_COORDINATOR) {
        // Unstated destination address only available on coordinator
        if (MAC_QueueAdrListFind(&H->Mem.Address, F->FrameControl.SrcAdrMode,
                                 F->Address.Src))
          // Check if the source already in the address list
          break;
      } else {
        if (F->FrameControl.FrameType == MAC_FRAMETYPE_COMMAND) {
          MAC_FrameCommandDecode(F, &C);
          if (C.CommandId == MAC_COMMAND_ID_DISCOVER_RESPONSE)
            break;
        }
      }
      return MAC_STATUS_INVALID_DESTINATION;

    case MAC_ADRMODE_SHORT:
      if (H->Pib.VpanCoordinator == MAC_PIB_VPAN_COORDINATOR) {
        // Coordinator side
        if (F->Address.Dst.Short == MAC_CONST_BROADCAST_ADDRESS)
          // Accept all broadcast address
          break;
        if (F->Address.Dst.Short != MAC_CONST_USE_EXTENDED_ADDRESS &&
            H->Pib.ShortAdr == F->Address.Dst.Short)
          // Accept matched short address
          break;
      } else {
        // Device side
        if (H->Pib.AssociatedCoord == MAC_PIB_ASSOCIATED_SET &&
            H->Pib.ShortAdr == F->Address.Dst.Short)
          // Accept when associated and matched short address
          break;
      }
      return MAC_STATUS_INVALID_DESTINATION;

    case MAC_ADRMODE_EXTENDED:
      if (F->Address.Dst.Extended == H->Config.ExtendedAddress)
        // Allow all matched extended addresses
        break;
      return MAC_STATUS_INVALID_DESTINATION;
  }

  // Check source addressing
  if (H->Pib.VpanCoordinator == MAC_PIB_VPAN_COORDINATOR) {
    // Check addressing on coordinator
    switch (F->FrameControl.SrcAdrMode) {
      case MAC_ADRMODE_NOT_PRESENT:
        if (F->FrameControl.FrameType == MAC_FRAMETYPE_COMMAND) {
          MAC_FrameCommandDecode(F, &C);
          if (C.CommandId == MAC_COMMAND_ID_DISCOVER_REQUEST)
            break;
        }
        // This is unacceptable
        return MAC_STATUS_INVALID_SOURCE;

      case MAC_ADRMODE_SHORT:
        if (MAC_QueueAdrListFind(&H->Mem.Address, MAC_ADRMODE_SHORT,
                                 F->Address.Src))
          break;
        return MAC_STATUS_INVALID_SOURCE;

      case MAC_ADRMODE_EXTENDED:
        // Well, accept all extended address source
        break;
    }
  } else {
    // Check addressing on device
    switch (F->FrameControl.SrcAdrMode) {
      case MAC_ADRMODE_NOT_PRESENT:
        if (H->Pib.AssociatedCoord == MAC_PIB_ASSOCIATED_SET)
          // Accept frame from coordinator
          break;
        return MAC_STATUS_INVALID_SOURCE;

      case MAC_ADRMODE_SHORT:
        // Only receive short address from coordinator, if associated
        if (H->Pib.AssociatedCoord == MAC_PIB_ASSOCIATED_RESET)
          break;
        else
          if (H->Pib.CoordShortAdr != MAC_CONST_USE_EXTENDED_ADDRESS &&
              H->Pib.CoordShortAdr != MAC_CONST_BROADCAST_ADDRESS &&
              H->Pib.CoordShortAdr == F->Address.Src.Short)
            break;
        return MAC_STATUS_INVALID_SOURCE;

      case MAC_ADRMODE_EXTENDED:
        // Well, accept all extended address source
        break;
    }
  }

  return MAC_STATUS_OK;
}
