/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Paula audio.device
    Lang: English
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/resident.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/tasks.h>
#include <exec/interrupts.h>
#include <devices/audio.h>
#include <clib/alib_protos.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>

#include <hardware/custom.h>

#include "audio_device.h"

#include LC_LIBDEFS_FILE

AROS_LH1(void, beginio,
 AROS_LHA(struct IOAudio *, io, A1),
 struct AudioBase *, AudioBase, 5, Audio)
{
    AROS_LIBFUNC_INIT

    D(bug("audio beginio %x\n", io));
 
    if (io->ioa_Request.io_Flags & IOF_QUICK) {
    	return;
    } else {
	ReplyMsg((struct Message*)io);
    }
    AROS_LIBFUNC_EXIT
}	

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOAudio *, io, A1),
 struct AudioBase *, AudioBase, 6, Audio)
{
    AROS_LIBFUNC_INIT
    
    D(bug("audio abortio %x\n", io));
    
    return IOERR_NOCMD;

    AROS_LIBFUNC_EXIT
}


static int GM_UNIQUENAME(init)(LIBBASETYPEPTR AudioBase)
{

    D(bug("Audio init\n"));

    return TRUE;
}

static int GM_UNIQUENAME(open)
(
    LIBBASETYPEPTR AudioBase,
    struct IOAudio *io,
    ULONG unitnum,
    ULONG flags
)
{
    io->ioa_Request.io_Error = IOERR_OPENFAIL;

    io->ioa_Request.io_Device = (struct Device *)AudioBase;

    io->ioa_Request.io_Unit = 1;
    io->ioa_Request.io_Error = 0;

    D(bug("Audio: Open\n"));
  
    return io->ioa_Request.io_Error == 0;
}

static int GM_UNIQUENAME(close)
(
    LIBBASETYPEPTR AudioBase,
    struct IOAudio *io
)
{
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(init), 0)
ADD2OPENDEV(GM_UNIQUENAME(open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(close), 0)
