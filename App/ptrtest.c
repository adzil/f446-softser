#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <mac.h>
#include "mac.h"
#include "macros.h"

int main(void) {
  MAC_Frame *F, *G;
  uint8_t PayloadTest[] = {0xff, 0x56, 0xab, 0xbc};
  uint8_t *EncTest;
  uint16_t EncLen;
  int i;

  F = MAC_FrameAlloc(sizeof(PayloadTest));
  memcpy(F->Payload.Data, PayloadTest, sizeof(PayloadTest));
  F->FrameControl.DestinationAddressMode = MAC_ADDRESS_SHORT;
  F->FrameControl.SourceAddressMode = MAC_NO_ADDRESS;
  F->FrameControl.AckRequest = MAC_NO_ACK;
  F->FrameControl.FramePending = MAC_FRAME_NOT_PENDING;
  F->FrameControl.FrameType = MAC_FRAME_TYPE_DATA;
  F->Address.Destination.Short = 0xabcd;
  F->Sequence = 0;

  EncLen = MAC_FrameEncodeLen(F);
  EncTest = MEM_Alloc(EncLen);
  MAC_FrameEncode(F, EncTest);

  G = MAC_FrameDecode(EncTest, EncLen);

  for (i = 0; i < G->Payload.Length; i++) {
    printf("%x ", G->Payload.Data[i]);
  }

  return 0;
}