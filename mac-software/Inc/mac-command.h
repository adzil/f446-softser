#ifndef __MAC_COMMAND_H__
#define __MAC_COMMAND_H__

#include <inttypes.h>
#include "random.h"
#include "mac-instance.h"

void MAC_CmdSetFrameDstFromSrc(MAC_Frame *F, MAC_Frame *SF);
void MAC_CmdAssocResponseSend(MAC_Instance *H, MAC_Frame *SF,
                              MAC_FrameAssocStatus Status, uint16_t ShortAdr);
void MAC_CmdAssocRequestSend(MAC_Instance *H);
void MAC_CmdDataRequestSend(MAC_Instance *H);
void MAC_CmdDiscoverRequestSend(MAC_Instance *H);
void MAC_CmdDiscoverResponseSend(MAC_Instance *H);
void MAC_CmdAssocRequestHandler(MAC_Instance *H, MAC_Frame *F);
void MAC_CmdAssocResponseHandler(MAC_Instance *H, MAC_Frame *F,
                                 MAC_FrameCommand *C);
void MAC_CmdDiscoverRequestHandler(MAC_Instance *H);
void MAC_CmdDiscoverResponseHandler(MAC_Instance *H, MAC_FrameCommand *C);
MAC_Status MAC_CmdFrameHandler(MAC_Instance *H, MAC_Frame *F);

#endif // __MAC_COMMAND_H__
