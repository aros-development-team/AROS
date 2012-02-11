/*
 * ntfs.handler - New Technology FileSystem handler
 *
 * Copyright © 2012 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id $
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

#include <stdarg.h>
#include <string.h>

#include "ntfs_fs.h"

#include "debug.h"

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
            DeleteIORequest((struct IORequest *)InputRequest);
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

void ErrorMessageArgs(char *fmt, IPTR *ap)
{
	struct IntuitionBase *IntuitionBase;

	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
	if (IntuitionBase) {
		struct EasyStruct es = {
			sizeof (struct EasyStruct),
			0,
			"NTFS filesystem critical error",
			NULL,
			"Ok"
		};

		es.es_TextFormat = fmt;
		EasyRequestArgs(NULL, &es, NULL, ap);
		CloseLibrary((struct Library *)IntuitionBase);
	}
}

void NTFS2DateStamp(UQUAD *NTFSTime, struct DateStamp *DS)
{
    UQUAD adjustedtime;
    UQUAD nttimeoffset;
    struct timeval tval;

//    nttimeoffset = (377 * 365 + 91) * 24 * 3600 * 10000000;
    nttimeoffset = 0x02C51CD000ULL * 10000000;
    
    adjustedtime = *NTFSTime - nttimeoffset;
    
    D(bug("[NTFS]: %s: adjusted = %d, offset = %d\n", __PRETTY_FUNCTION__, adjustedtime, nttimeoffset));

    tval.tv_secs = adjustedtime / 10000000;
    tval.tv_micro = (adjustedtime / 10) % 1000000;
    
    /* calculate days since 1978-01-01 (DOS epoch) */
    DS->ds_Days = tval.tv_secs / (60 * 60 * 24);

    /* minutes since midnight */
    DS->ds_Minute = tval.tv_secs / 60 % (24 * 60);

    /* 1/50 sec ticks since last minute */
    DS->ds_Tick =  50 * (tval.tv_secs % 60) + (tval.tv_micro / 20000);
}

APTR _AllocVecPooled(APTR mem_pool, ULONG size)
{
    APTR newvec = AllocVecPooled(mem_pool, size);
    D(bug("**** [pool:0x%p] Allocated %d bytes @ 0x%p\n", mem_pool, size, newvec));
    return newvec;
}

void _FreeVecPooled(APTR mem_pool, APTR vecaddr)
{
    D(bug("**** [pool:0x%p] Freeing 0x%p\n", mem_pool, vecaddr));
    FreeVecPooled(mem_pool, vecaddr);
}
