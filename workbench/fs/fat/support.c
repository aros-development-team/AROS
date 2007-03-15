/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#include <exec/types.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>

#include <proto/intuition.h>
#include <proto/exec.h>

#include <string.h>

#include "fat_fs.h"

/*-------------------------------------------------------------------------*/

void ReturnPacket(struct DosPacket *packet, LONG res1, LONG res2) {
    struct MsgPort *mp = packet->dp_Port;

    kprintf("Returning packet: %lx %lx\n", res1, res2);

    packet->dp_Port = glob->ourport;
    packet->dp_Res1 = res1;
    packet->dp_Res2 = res2;

    PutMsg(mp, packet->dp_Link);
}

struct DosPacket *GetPacket(struct MsgPort *port) {
    struct Message *msg;

    if((msg = GetMsg(port)))
        return (struct DosPacket *) msg->mn_Node.ln_Name;

    return NULL;
}

/*-------------------------------------------------------------------------*/  

void SendEvent(LONG event) {
    struct IOStdReq *InputRequest;
    struct MsgPort *InputPort;
    struct InputEvent *ie;

    if ((InputPort = (struct MsgPort*)CreateMsgPort())) {

        if ((InputRequest = (struct IOStdReq*)CreateIORequest(InputPort, sizeof(struct IOStdReq)))) {

            if (!OpenDevice("input.device", 0, (struct IORequest*)InputRequest, 0)) {

                if ((ie = AllocVec(sizeof(struct InputEvent), MEMF_PUBLIC))) {
                    ie->ie_Class = event;
                    InputRequest->io_Command = IND_WRITEEVENT;
                    InputRequest->io_Data = ie;
                    InputRequest->io_Length = sizeof(struct InputEvent);

                    DoIO((struct IORequest*)InputRequest);

                    FreeVec(ie);
                }
                CloseDevice((struct IORequest*)InputRequest);
            }
            DeleteIORequest (InputRequest);
        }
        DeleteMsgPort (InputPort);
    }
}

int ErrorReq (STRPTR text, ULONG args[]) {
    struct EasyStruct req = {
        sizeof(struct EasyStruct),
        0,
        "SGIXFilesystem Error",
        text,
        "Ok" /* see REQ_ defines at the top of the file */
    };

    if (IntuitionBase->FirstScreen != NULL)
        return EasyRequestArgs(NULL, &req, NULL, args);

    return 0;
}

/*-------------------------------------------------------------------------*/  

int ilog2(ULONG data) {
    int bitoffset = 31;
    ULONG bitmask = 1 << bitoffset;

    do {
        if ((data & bitmask) != 0)
            return bitoffset;

    bitoffset--;
    bitmask >>= 1;
  } while (bitmask != 0);

  return 0;
}

/*-----------------------------------------------------------------------*/

/*
 *  Debug stuff....
 */
#ifdef __DEBUG__

static UBYTE debugbuff[0xff];
void knprints(UBYTE *name, LONG namelen)
{
    CopyMem(name, debugbuff, namelen);
    debugbuff[namelen] = '\0';
    kprintf("\"%s\"\n", (LONG)debugbuff);
}

#endif
