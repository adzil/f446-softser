#include <mac-frame.h>
#include "mac-command.h"

void MAC_CmdSetFrameDstFromSrc(MAC_Frame *F, MAC_Frame *SF) {
  // Copy addressing details from source frame
  MAC_SetFrameDstAdr(F, SF->FrameControl.SrcAdrMode, SF->Address.Src);
}

void MAC_CmdSetFrameDstToCoord(MAC_Instance *H, MAC_Frame *F) {
  if (H->Pib.AssociatedCoord == MAC_PIB_ASSOCIATED_SET) {
    MAC_SetFrameNoDstAdr(F);
  } else {
    if (H->Pib.CoordShortAdr != MAC_CONST_BROADCAST_ADDRESS &&
        H->Pib.CoordShortAdr != MAC_CONST_USE_EXTENDED_ADDRESS)
      MAC_SetFrameShortDstAdr(F, H->Pib.CoordShortAdr);
    else
      MAC_SetFrameExtendedDstAdr(F, H->Pib.CoordExtendedAdr);
  }
}

void MAC_CmdSetFrameHeader(MAC_Instance *H, MAC_Frame *F, MAC_FrameCommand *C) {
  // Set frame controls
  MAC_GenFrameSequence(H, F);

  if (C->CommandId == MAC_COMMAND_ID_DISCOVER_REQUEST) {
    MAC_SetFrameNoSrcAdr(F);
  } else {
    // Generate source addressing
    MAC_GenFrameSrcAdr(H, F);
  }

  switch (C->CommandId) {
    case MAC_COMMAND_ID_DISCOVER_RESPONSE:
    case MAC_COMMAND_ID_DISCOVER_REQUEST:
      MAC_SetFrameNoAckRequest(F);
      break;

    default:
      MAC_SetFrameAckRequest(F);
      break;
  }

  MAC_SetFrameNoPending(F);
  MAC_SetFrameType(F, MAC_FRAMETYPE_COMMAND);
}

void MAC_CmdAssocResponseSend(MAC_Instance *H, MAC_Frame *SF,
                              MAC_FrameAssocStatus Status, uint16_t ShortAdr) {
  MAC_Frame *F;
  MAC_FrameCommand C;

  F = MAC_MemFrameAlloc(&H->Mem);
  if (!F) return;
  MAC_SetFrameCmdAssocResponse(&C, ShortAdr, Status);
  MAC_CmdSetFrameDstFromSrc(F, SF);
  MAC_CmdSetFrameHeader(H, F, &C);
  MAC_FrameCommandEncode(F, &C);

  MAC_TransmitPutFrame(H, F);
}

void MAC_CmdAssocRequestSend(MAC_Instance *H) {
  MAC_Frame *F;
  MAC_FrameCommand C;

  F = MAC_MemFrameAlloc(&H->Mem);
  if (!F) return;
  MAC_SetFrameCmdAssocRequest(&C);
  MAC_CmdSetFrameDstToCoord(H, F);
  MAC_CmdSetFrameHeader(H, F, &C);
  MAC_FrameCommandEncode(F, &C);

  MAC_TransmitPutFrame(H, F);
}

void MAC_CmdDataRequestSend(MAC_Instance *H) {
  MAC_Frame *F;
  MAC_FrameCommand C;

  F = MAC_MemFrameAlloc(&H->Mem);
  if (!F) return;
  MAC_SetFrameCmdDataRequest(&C);
  MAC_CmdSetFrameDstToCoord(H, F);
  MAC_CmdSetFrameHeader(H, F, &C);
  MAC_FrameCommandEncode(F, &C);

  MAC_TransmitPutFrame(H, F);
}

void MAC_CmdDiscoverRequestSend(MAC_Instance *H) {
  MAC_Frame *F;
  MAC_FrameCommand C;

  F = MAC_MemFrameAlloc(&H->Mem);
  if (!F) return;
  // Set to broadcast address
  MAC_SetFrameCmdDiscoverRequest(&C);
  MAC_SetFrameShortDstAdr(F, MAC_CONST_BROADCAST_ADDRESS);
  MAC_CmdSetFrameHeader(H, F, &C);
  MAC_FrameCommandEncode(F, &C);

  MAC_TransmitPutFrame(H, F);
}

void MAC_CmdDiscoverResponseSend(MAC_Instance *H) {
  MAC_Frame *F;
  MAC_FrameCommand C;

  F = MAC_MemFrameAlloc(&H->Mem);
  if (!F) return;
  // Set to broadcast address
  MAC_SetFrameCmdDiscoverResponse(&C, H->Pib.ShortAdr,
                                  H->Config.ExtendedAddress);
  MAC_SetFrameNoDstAdr(F);
  MAC_CmdSetFrameHeader(H, F, &C);
  MAC_FrameCommandEncode(F, &C);

  // Immediate send frame
  MAC_TransmitSendFrame(H, F);
}

void MAC_CmdAssocRequestHandler(MAC_Instance *H, MAC_Frame *F) {
  MAC_AdrList *A;
  MAC_FrameAddress Adr;

  // Check for existing address on list
  A = MAC_QueueAdrListFind(&H->Mem.Address, MAC_ADRMODE_EXTENDED,
                           F->Address.Src);
  if (A) {
    Adr.Short = A->ShortAdr;
  } else {
    do {
      // Keep generating short address if exists on the list
      Adr.Short = (RND_Get() << 8) | RND_Get(); // TODO: Change to uC specific random
      A = MAC_QueueAdrListFind(&H->Mem.Address, MAC_ADRMODE_SHORT, Adr);
    } while (A || Adr.Short == MAC_CONST_USE_EXTENDED_ADDRESS ||
             Adr.Short == MAC_CONST_BROADCAST_ADDRESS ||
             Adr.Short == H->Pib.ShortAdr);
  }

  A = MAC_MemAdrListAlloc(&H->Mem);
  if (!A) {
    MAC_CmdAssocResponseSend(H, F, MAC_ASSOCSTATUS_FAILED,
                              MAC_CONST_BROADCAST_ADDRESS);
  } else {

    A->ExtendedAdr = F->Address.Src.Extended;
    A->ShortAdr = Adr.Short;
    A = MAC_QueueAdrListAppend(&H->Mem.Address, A);
    if (A) MAC_MemAdrListFree(&H->Mem, A);
    MAC_CmdAssocResponseSend(H, F, MAC_ASSOCSTATUS_SUCCESS, Adr.Short);

    sprintf(MAC_TermBuf, "Joined Device X: %x S: %x\r\n",
            F->Address.Src.Extended, Adr.Short);
    Log(MAC_TermBuf);
  }
}

void MAC_CmdAssocResponseHandler(MAC_Instance *H, MAC_Frame *F,
                                 MAC_FrameCommand *C) {
  if (C->AssocStatus == MAC_ASSOCSTATUS_SUCCESS) {
    // Set coordinator address
    if (F->FrameControl.SrcAdrMode == MAC_ADRMODE_SHORT) {
      H->Pib.CoordShortAdr = F->Address.Src.Short;
    } else if (F->FrameControl.SrcAdrMode == MAC_ADRMODE_EXTENDED) {
      H->Pib.CoordShortAdr = MAC_CONST_USE_EXTENDED_ADDRESS;
      H->Pib.CoordExtendedAdr = F->Address.Src.Extended;
    } else {
      return;
    }
    // Set PIB to associated
    H->Pib.AssociatedCoord = MAC_PIB_ASSOCIATED_SET;
    H->Pib.ShortAdr = C->ShortAddress;

    __GPIO_WRITE(GPIOA, 5, GPIO_PIN_SET);
    sprintf(MAC_TermBuf, "Joined Coord. with Addr. %x\r\n", C->ShortAddress);
    Log(MAC_TermBuf);
  } else {
    H->Pib.AssociatedCoord = MAC_PIB_ASSOCIATED_RESET;
  }
}

void MAC_CmdDataRequestHandler(MAC_Instance *H, MAC_Frame *F) {
  MAC_TransmitLookupFrame(H, F->FrameControl.SrcAdrMode, F->Address.Src);
}

void MAC_CmdDiscoverRequestHandler(MAC_Instance *H) {
  MAC_CmdDiscoverResponseSend(H);
}

void MAC_CmdDiscoverResponseHandler(MAC_Instance *H, MAC_FrameCommand *C) {
  // Only set new coordinator address when not associated
  if (H->Pib.AssociatedCoord == MAC_PIB_ASSOCIATED_RESET) {
    H->Pib.CoordShortAdr = C->ShortAddress;
    H->Pib.CoordExtendedAdr = C->ExtendedAddress;

    sprintf(MAC_TermBuf, "Discovered Coord. X: %x S: %x\r\n", C->ShortAddress,
            C->ExtendedAddress);
    Log(MAC_TermBuf);
  }
}

MAC_Status MAC_CmdFrameHandler(MAC_Instance *H, MAC_Frame *F) {
  MAC_FrameCommand C;

  // Decode command frame
  MAC_FrameCommandDecode(F, &C);
  switch (C.CommandId) {
    case MAC_COMMAND_ID_ASSOC_REQUEST:
      // All association request should come from extended address
      if (F->FrameControl.SrcAdrMode != MAC_ADRMODE_EXTENDED)
        return MAC_STATUS_INVALID_SOURCE;
      // All association request should be processed only by coordinator
      if (H->Pib.VpanCoordinator != MAC_PIB_VPAN_COORDINATOR)
        return MAC_STATUS_INVALID_COMMAND;
      MAC_CmdAssocRequestHandler(H, F);
      break;

    case MAC_COMMAND_ID_ASSOC_RESPONSE:
      // Only device should respond to association response
      if (H->Pib.VpanCoordinator == MAC_PIB_VPAN_COORDINATOR)
        return MAC_STATUS_INVALID_COMMAND;
      MAC_CmdAssocResponseHandler(H, F, &C);
      break;

    case MAC_COMMAND_ID_DATA_REQUEST:
      // Only coordinator should process data request
      if (H->Pib.VpanCoordinator != MAC_PIB_VPAN_COORDINATOR)
        return MAC_STATUS_INVALID_COMMAND;
      MAC_CmdDataRequestHandler(H, F);
      break;

    case MAC_COMMAND_ID_DISCOVER_REQUEST:
      // Only coord should response to discover request
      if (H->Pib.VpanCoordinator != MAC_PIB_VPAN_COORDINATOR)
        return MAC_STATUS_INVALID_COMMAND;
      MAC_CmdDiscoverRequestHandler(H);
      break;

    case MAC_COMMAND_ID_DISCOVER_RESPONSE:
      // Only device should act on discover response
      if (H->Pib.VpanCoordinator == MAC_PIB_VPAN_COORDINATOR)
        return MAC_STATUS_INVALID_COMMAND;
      MAC_CmdDiscoverResponseHandler(H, &C);

    default:
      return MAC_STATUS_INVALID_COMMAND;
  }

  return MAC_STATUS_OK;
}
