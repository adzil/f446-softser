#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <mac.h>
#include "mac.h"
#include "macros.h"

int main(void) {
  MAC_Frame *F, *G;
  //uint8_t PayloadTest[] = {0xff, 0x56, 0xab, 0xbc};
  uint8_t *EncTest;
  uint16_t EncLen;
  int i;

  // Initialize MAC layer
  MAC_Init();

  F = MAC_FrameAlloc(0);
  //memcpy(F->Payload.Data, PayloadTest, sizeof(PayloadTest));
  F->FrameControl.DestinationAddressMode = MAC_ADDRESS_SHORT;
  F->FrameControl.SourceAddressMode = MAC_NO_ADDRESS;
  F->FrameControl.AckRequest = MAC_NO_ACK;
  F->FrameControl.FramePending = MAC_FRAME_NOT_PENDING;
  F->FrameControl.FrameType = MAC_FRAME_TYPE_COMMAND;
  F->Address.Destination.Short = 0xabcd;
  F->Payload.Command.CommandFrameId = MAC_COMMAND_ASSOC_RESPONSE;
  F->Payload.Command.ShortAddress = 3450;
  F->Payload.Command.Status = MAC_ASSOCIATION_FAILED;

  EncLen = MAC_FrameEncodeLen(F);
  EncTest = MEM_Alloc(EncLen);
  MAC_FrameEncode(F, EncTest);

  G = MAC_FrameDecode(EncTest, EncLen);

  /*for (i = 0; i < G->Payload.Length; i++) {
    printf("%x ", G->Payload.Data[i]);
  }*/

  return 0;
}