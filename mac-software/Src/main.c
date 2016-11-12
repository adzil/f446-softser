#include "mac-core.h"

void Broadcaster(MAC_Instance *H, int SendId, int Count) {
  uint8_t Buffer[MAC_CONFIG_MAX_FRAME_BUFFER];
  size_t Length;
  int i;

  MAC_CoreFrameSend(&H[SendId], Buffer, &Length);

  for (i = 0; i < Count; i++) {
    // Skip receiving on same instance
    if (i == SendId) continue;
    MAC_CoreFrameReceived(&H[i], Buffer, Length);
  }
}

int main(void) {
  MAC_Instance Device[5];
  uint8_t Buffer[128];
  size_t Length;

  MAC_Init(&Device[0], 0x1, MAC_PIB_VPAN_COORDINATOR);
  MAC_Init(&Device[1], 0x2, MAC_PIB_VPAN_DEVICE);
  MAC_Init(&Device[2], 0x3, MAC_PIB_VPAN_DEVICE);
  MAC_Init(&Device[3], 0x4, MAC_PIB_VPAN_DEVICE);
  MAC_Init(&Device[4], 0x5, MAC_PIB_VPAN_DEVICE);

  Device[0].Pib.ShortAdr = 0x1a1b;

  // Send discover request
  MAC_CmdDiscoverRequestSend(&Device[1]);
  Broadcaster(Device, 1, 5);
  // Retrieve discover response
  Broadcaster(Device, 0, 5);
  // Association request
  MAC_CmdAssocRequestSend(&Device[1]);
  Broadcaster(Device, 1, 5);
  // Get acknowledgement frame
  Broadcaster(Device, 0, 5);
  // Get data from coord
  MAC_CmdDataRequestSend(&Device[1]);
  Broadcaster(Device, 1, 5);
  // Get ack & data
  Broadcaster(Device, 0, 5);
  Broadcaster(Device, 0, 5);
  Broadcaster(Device, 1, 5);
  Broadcaster(Device, 0, 5);
  Broadcaster(Device, 0, 5);

/*
  printf("- Association Request Test\n");

  for (i = 0; i < 3; i++) {
    MAC_CmdAssocRequestSend(&Device[i]);
    MAC_CoreFrameSend(&Device[i], Buffer, &Length);
    MAC_CoreFrameReceived(&Coord, Buffer, Length);
  }

  printf("- Data Request Test\n");

  for (i = 0; i < 3; i++) {
    MAC_CmdDataRequestSend(&Device[i]);
    MAC_CoreFrameSend(&Device[i], Buffer, &Length);
    MAC_CoreFrameReceived(&Coord, Buffer, Length);
  }

  printf("- Association Response Test\n");

  for (i = 0; i < 3; i++) {
    MAC_CoreFrameSend(&Coord, Buffer, &Length);
    MAC_CoreFrameReceived(&Device[0], Buffer, Length);
    MAC_CoreFrameReceived(&Device[1], Buffer, Length);
    MAC_CoreFrameReceived(&Device[2], Buffer, Length);
  }

  printf("- Coordinator to Device Transmission Test\n");
  printf("  Coordinator will begin to create and queue new data frame with "
             "destination to Device 1.\n");
*/
  /*
  // Create dummy data frame
  F = MAC_MemFrameAlloc(&Coord.Mem);
  if (F) {
    MAC_SetFrameShortDstAdr(F, Device[0].Pib.ShortAdr);
    MAC_GenFrameSrcAdr(&Coord, F);
    MAC_GenFrameSequence(&Coord, F);
    MAC_SetFrameNoAckRequest(F);
    MAC_SetFrameNoPending(F);
    MAC_SetFrameType(F, MAC_FRAMETYPE_DATA);

    memcpy(F->Payload.Data, "Test Data", 9);
    F->Payload.Length = 9;

    MAC_TransmitPutFrame(&Coord, F);
  }

  F = MAC_MemFrameAlloc(&Coord.Mem);
  if (F) {
    MAC_SetFrameShortDstAdr(F, Device[2].Pib.ShortAdr);
    MAC_GenFrameSrcAdr(&Coord, F);
    MAC_GenFrameSequence(&Coord, F);
    MAC_SetFrameNoAckRequest(F);
    MAC_SetFrameNoPending(F);
    MAC_SetFrameType(F, MAC_FRAMETYPE_DATA);

    memcpy(F->Payload.Data, "New Data", 8);
    F->Payload.Length = 8;

    MAC_TransmitPutFrame(&Coord, F);
  }

  printf("- Collective data request\n"
             "   Coordinator will only send data to device 1 "
             "(from prev queue).\n");

  for (i = 0; i < 3; i++) {
    MAC_CmdDataRequestSend(&Device[i]);
    MAC_CoreFrameSend(&Device[i], Buffer, &Length);
    MAC_CoreFrameReceived(&Coord, Buffer, Length);
  }

  printf("- Collective data retrieval\n"
             "   Device MAC will filter out data only to Device 1.\n");

  for (i = 0; i < 3; i++) {
    MAC_CoreFrameSend(&Coord, Buffer, &Length);
    MAC_CoreFrameReceived(&Device[0], Buffer, Length);
    MAC_CoreFrameReceived(&Device[1], Buffer, Length);
    MAC_CoreFrameReceived(&Device[2], Buffer, Length);
  }

  printf("- Device to Coordinator Transmission Test\n"
         "  Device 2 will create new data frame with dst to Coordinator.\n");

  // Send data from client
  F = MAC_MemFrameAlloc(&Device[2].Mem);
  if (F) {
    MAC_SetFrameNoDstAdr(F);
    MAC_GenFrameSrcAdr(&Device[2], F);
    MAC_GenFrameSequence(&Device[2], F);
    MAC_SetFrameNoAckRequest(F);
    MAC_SetFrameNoPending(F);
    MAC_SetFrameType(F, MAC_FRAMETYPE_DATA);

    memcpy(F->Payload.Data, "Reply Data", 10);
    F->Payload.Length = 10;

    MAC_TransmitPutFrame(&Device[2], F);
  }


  printf("- Receive event from coordinator\n");

  MAC_CoreFrameSend(&Device[2], Buffer, &Length);
  MAC_CoreFrameReceived(&Coord, Buffer, Length);
*/
  return 0;
}
