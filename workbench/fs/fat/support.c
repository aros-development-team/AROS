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

void SendEvent(LONG event) {
    struct IOStdReq *InputRequest;
    struct MsgPort *InputPort;
    struct InputEvent *ie;

    if ((InputPort = (struct MsgPort*)CreateMsgPort())) {

        if ((InputRequest = (struct IOStdReq*)CreateIORequest(InputPort, sizeof(struct IOStdReq)))) {

            if (!OpenDevice("input.device", 0, (struct IORequest*)InputRequest, 0)) {

                if ((ie = AllocVec(sizeof(struct InputEvent), MEMF_PUBLIC | MEMF_CLEAR))) {
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
