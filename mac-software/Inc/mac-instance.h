#ifndef __MAC_HANDLE_H__
#define __MAC_HANDLE_H__

#include "random.h"
/* Include all mac sub-functions */
#include "mac-common.h"
#include "mac-config.h"
#include "mac-frame.h"
#include "mac-memory.h"
#include "mac-queue.h"
#include "mac-pib.h"

#include "usart.h"

extern char MAC_TermBuf[256];

#if (DEV_CONFIG == 0)
#define MAC_COORDINATOR
#endif


// Transmission unit structure
typedef struct {
  //MAC_Frame *F;
  uint8_t Data[MAC_CONFIG_MAX_FRAME_BUFFER];
  LOCK_Handle Lock;
  size_t Length;
  uint16_t Retries;
  uint16_t FailedAck;
  uint8_t Sequence;
} MAC_Transmission;

// MAC Handle for single MAC instance
typedef struct {
  MAC_Pib Pib;
  MAC_Config Config;
  MAC_Transmission Tx;
  // The memory part should be on the bottom
  MAC_Mem Mem;
} MAC_Instance;

#ifndef MAC_COORDINATOR
#define MAC_DEBUG
#endif

force_inline void Log(const char *Msg) {
#ifndef MAC_COORDINATOR
  HAL_UART_Transmit(&huart2, (uint8_t *)Msg, strlen(Msg), 0xff);
#else
	(void)0;
#endif
}

force_inline void UARTData(uint8_t *Data, size_t Len) {
  HAL_UART_Transmit(&huart2, Data, Len, 0xff);
}

#ifdef MAC_DEBUG
typedef enum {
  MAC_DEBUG_RCV,
  MAC_DEBUG_SND,
  MAC_DEBUG_QUE,
  MAC_DEBUG_RDY
} MAC_DebugDir;

force_inline void MAC_DebugFrame(MAC_Instance *H, MAC_Frame *F,
                                 MAC_DebugDir Dir) {
  MAC_FrameCommand C;
  int i;

  Log("FRAME ");
  switch (Dir) {
    case MAC_DEBUG_RCV:
      Log("RCV");
      break;

    case MAC_DEBUG_SND:
      Log("SND");
      break;

    case MAC_DEBUG_QUE:
      Log("QUE");
      break;

    case MAC_DEBUG_RDY:
      Log("RDY");
      break;
  }
  Log(" - ADDRESS: ");
  switch (F->FrameControl.SrcAdrMode) {
    case MAC_ADRMODE_NOT_PRESENT:
      Log("[NA]");
      break;

    case MAC_ADRMODE_SHORT:
      sprintf(MAC_TermBuf, "[S:%x]", F->Address.Src.Short);
      Log(MAC_TermBuf);
      break;

    case MAC_ADRMODE_EXTENDED:
      sprintf(MAC_TermBuf, "[X:%x]", F->Address.Src.Extended);
      Log(MAC_TermBuf);
      break;
  }
  Log("-->");
  switch (F->FrameControl.DstAdrMode) {
    case MAC_ADRMODE_NOT_PRESENT:
      Log("[NA]");
      break;

    case MAC_ADRMODE_SHORT:
      sprintf(MAC_TermBuf, "[S:%x]", F->Address.Dst.Short);
      Log(MAC_TermBuf);
      break;

    case MAC_ADRMODE_EXTENDED:
      sprintf(MAC_TermBuf, "[X:%x]", F->Address.Dst.Extended);
      Log(MAC_TermBuf);
      break;
  }

  Log(" ACK: ");
  switch (F->FrameControl.AckRequest) {
    case MAC_ACKREQUEST_RESET:
      Log("No");
      break;

    case MAC_ACKREQUEST_SET:
      Log("Yes");
      break;
  }

  sprintf(MAC_TermBuf, " SEQN: %d TYPE: ", F->Sequence);
  Log(MAC_TermBuf);

  switch(F->FrameControl.FrameType) {
    case MAC_FRAMETYPE_DATA:
      Log("Data PAYLOAD: ");
      for (i = 0; i < F->Payload.Length; i++) {
        sprintf(MAC_TermBuf, "%2x ", F->Payload.Data[i]);
        Log(MAC_TermBuf);
      }
      break;

    case MAC_FRAMETYPE_COMMAND:
      Log("Command PAYLOAD: ");
      MAC_FrameCommandDecode(F, &C);
      switch (C.CommandId) {
        case MAC_COMMAND_ID_ASSOC_REQUEST:
          Log("AssocRequest");
          break;

        case MAC_COMMAND_ID_ASSOC_RESPONSE:
          Log("AssocResponse MSG: ");
          switch (C.AssocStatus) {
            case MAC_ASSOCSTATUS_SUCCESS:
              sprintf(MAC_TermBuf, "SUCCESS, SHORT ADR: %x", C.ShortAddress);
              Log(MAC_TermBuf);
              break;

            case MAC_ASSOCSTATUS_FAILED:
              Log("FAILED");
              break;
          }
          break;

        case MAC_COMMAND_ID_DATA_REQUEST:
          Log("DataRequest");
          break;

        case MAC_COMMAND_ID_DISCOVER_REQUEST:
          Log("DiscoverRequest");
          break;

        case MAC_COMMAND_ID_DISCOVER_RESPONSE:
          sprintf(MAC_TermBuf, "DiscoverResponse S: %x X: %x", C.ShortAddress,
                  C.ExtendedAddress);
          Log(MAC_TermBuf);

      }
      break;

    case MAC_FRAMETYPE_ACK:
      Log("Ack");
      break;
  }

  Log("\r\n");
}
#endif

void MAC_Init(MAC_Instance *H, uint32_t ExtendedAdr,
              MAC_PibVpanCoordinator VpanCoord);
void MAC_TransmitPutFrame(MAC_Instance *H, MAC_Frame *F);
void MAC_GenFrameSrcAdr(MAC_Instance *H, MAC_Frame *F);

force_inline void MAC_TransmitSendFrame(MAC_Instance *H, MAC_Frame *F) {
#ifdef MAC_DEBUG
  MAC_DebugFrame(H, F, MAC_DEBUG_RDY);
#endif
  F = MAC_QueueFrameAppend(&H->Mem.Tx, F);
  if (F) MAC_MemFrameFree(&H->Mem, F);
}

force_inline void MAC_TransmitLookupFrame(MAC_Instance *H,
                                          MAC_FrameAddressMode AdrMode,
                                          MAC_FrameAddress Adr) {
  MAC_Frame *F;

  F = MAC_QueueFrameFind(&H->Mem.Store, AdrMode, Adr);
  if (F) {
    MAC_TransmitSendFrame(H, F);
  }
}

force_inline MAC_Frame *MAC_TransmitGetFrame(MAC_Instance *H) {
  return MAC_QueueFramePop(&H->Mem.Tx);
}

force_inline void MAC_GenFrameSequence(MAC_Instance *H, MAC_Frame *F) {
  MAC_SetFrameSequence(F, H->Pib.DSN++);
}

#endif // __MAC_HANDLE_H__
