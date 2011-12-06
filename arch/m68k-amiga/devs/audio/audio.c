/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Paula audio.device
    Lang: English
*/

/*  TODO:
    - Channel stealing not yet supported
    - ADCMD_LOCK never returns
    - DMA wait missing (CMD_STOP and immediate CMD_START may cause glitches)
    - and lots bugs

    NOTES:
    - normally unused io->ioa_Request.io_Unit bits 4 to 7 are used to keep
      track of pending SYNCCYCLE-style requests (all cleared = done)

*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/resident.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/tasks.h>
#include <exec/interrupts.h>
#include <devices/audio.h>
#include <hardware/intbits.h>
#include <hardware/custom.h>
#include <clib/alib_protos.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>

#include "audio_intern.h"

#include LC_LIBDEFS_FILE

static const UBYTE masktoch[16] = { 0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0 };

static BOOL isplaying(struct AudioBase *ab, struct IOAudio *io, UBYTE ch)
{
    return (struct IOAudio*)ab->writelist[ch].mlh_Head->mln_Succ == io && !(ab->stopmask & (1 << ch));
}

struct IOAudio *getnextwrite(struct AudioBase *ab, UBYTE ch, BOOL second)
{
    struct IOAudio *io;
    if (ab->writelist[ch].mlh_Head->mln_Succ == NULL)
    	return NULL;
    io = (struct IOAudio*)ab->writelist[ch].mlh_Head;
    if (!second)
    	return io;
    if(ab->writelist[ch].mlh_Head->mln_Succ->mln_Succ == NULL)
    	return NULL;
    return (struct IOAudio*)io->ioa_Request.io_Message.mn_Node.ln_Succ;
}

#define HEADER \
    UBYTE mask = (UBYTE)(ULONG)io->ioa_Request.io_Unit; \
    WORD key = io->ioa_AllocKey; \
    { io->ioa_Request.io_Unit = NULL; }

static void abort_io(struct AudioBase *ab, struct IOAudio *io)
{
    UBYTE ch = masktoch[(UBYTE)(ULONG)io->ioa_Request.io_Unit & CH_MASK];
    
    if (!io)
    	return;
    if (isplaying(ab, io, ch)) {
    	D(bug("audio: ch %d aborted, io %p\n", ch, io));
    	audiohw_stop(ab, 1 << ch);
    }
    REMOVE(io);
    io->ioa_Request.io_Error = IOERR_ABORTED;
    ReplyMsg(&io->ioa_Request.io_Message);
}

static void abort_ch(struct AudioBase *ab, UBYTE ch)
{
    struct IOAudio *io;

    Disable();
    while((io = (struct IOAudio*)ab->writelist[ch].mlh_Head->mln_Succ))
   	abort_io(ab, io);
    Enable();
}

static void abort_waitcycles(struct AudioBase *ab, UBYTE mask)
{
    struct IOAudio *io, *next;
    ForeachNodeSafe(&ab->misclist, io, next) {
    	UWORD cmd = io->ioa_Request.io_Command;
    	UBYTE cmask = (UBYTE)(ULONG)io->ioa_Request.io_Unit;
    	if (cmd != ADCMD_FINISH && cmd != ADCMD_PERVOL && cmd != ADCMD_WAITCYCLE)
    	    continue;
    	if (!(cmask & mask))
    	    continue;
    	abort_io(ab, io);
    }
}

static void allocchannels(struct AudioBase *ab, struct IOAudio *io, UBYTE mask, BYTE pri)
{
    UBYTE ch;
    WORD key;
    
    key = io->ioa_AllocKey;
    while (!key)
    	key = ++ab->keygen;
    for (ch = 0; ch < NR_CH; ch++) {
    	if (mask & (1 << ch)) {
    	    ab->pri[ch] = pri;
    	    ab->key[ch] = key;
    	}
    }
    io->ioa_AllocKey = key;
    io->ioa_Request.io_Unit = (struct Unit*)(ULONG)mask;
    audiohw_reset(ab, mask);
    D(bug("audio: allocmask %02x, pri %d, %04x (%04x %04x %04x %04x)\n",
    	mask, pri, io->ioa_AllocKey, ab->key[0], ab->key[1], ab->key[2], ab->key[3]));
}

static BOOL allocaudio(struct AudioBase *ab, struct IOAudio *io)
{
    UBYTE i, ch;
    UBYTE freech;

    io->ioa_Request.io_Error = 0;
    freech = 0;
    for (ch = 0; ch < NR_CH; ch++) {
    	if (ab->key[ch] == 0)
    	    freech |= 1 << ch;
    }
    for (i = 0; i < io->ioa_Length && i < 16; i++) {
    	UBYTE mask = io->ioa_Data[i];
    	D(bug("%d: allocation mask %02x & %02x\n", i, mask, freech));
    	if (mask == 0 || (mask & freech) == mask) {
    	    allocchannels(ab, io, mask, io->ioa_Request.io_Message.mn_Node.ln_Pri);
   	    return TRUE;
    	}
    }
    io->ioa_Request.io_Error = ADIOERR_ALLOCFAILED;
    return FALSE;
}

static void setunit(struct IOAudio *io, UBYTE num)
{
    ULONG unit = (ULONG)io->ioa_Request.io_Unit;
    unit |= 1 << num;
    io->ioa_Request.io_Unit = (struct Unit*)unit;
}

static BOOL ADCMD_ALLOCATE_f(struct AudioBase *ab, struct IOAudio *io)
{
    D(bug("ADCMD_ALLOCATE %02x %04x\n", mask, key));
    allocaudio(ab, io);
    if (io->ioa_Request.io_Error == 0)
    	return TRUE;
    if (io->ioa_Request.io_Flags & ADIOF_NOWAIT)
    	return TRUE;
    AddTail((struct List*)&ab->misclist, &io->ioa_Request.io_Message.mn_Node);
    return FALSE;
}

static BOOL ADCMD_FREE_f(struct AudioBase *ab, struct IOAudio *io)
{
    HEADER
    UBYTE ch;
    struct IOAudio *node, *next;

    D(bug("ADCMD_FREE %02x %04x\n", mask, key));
    for (ch = 0; ch < NR_CH; ch++) {
    	if ((mask & (1 << ch)) && key == ab->key[ch]) {
    	    abort_waitcycles(ab, 1 << ch);
   	    ab->key[ch] = 0;
    	    audiohw_reset(ab, 1 << ch);
    	    setunit(io, ch);
 	}
    }
    if (!io->ioa_Request.io_Unit) {
    	io->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    	return TRUE;
    }
    ForeachNodeSafe(&ab->misclist, node, next) {
    	if (node->ioa_Request.io_Command == ADCMD_ALLOCATE && allocaudio(ab, node)) {
    	    REMOVE(node);
    	    ReplyMsg(&io->ioa_Request.io_Message);
    	}
    }
    return TRUE;
}

static BOOL ADCMD_LOCK_f(struct AudioBase *ab, struct IOAudio *io)
{
    HEADER
    UBYTE ch;
    
    D(bug("ADCMD_LOCK %02x %04x\n", mask, key));
    for (ch = 0; ch < NR_CH; ch++) {
    	if ((mask & (1 << ch)) && key == ab->key[ch])
     	    setunit(io, ch);
    }
    if (!io->ioa_Request.io_Unit) {
    	io->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    	return TRUE;
    }
    AddTail((struct List*)&ab->misclist, &io->ioa_Request.io_Message.mn_Node);
    return FALSE;
}    	

static BOOL ADCMD_SETPREC_f(struct AudioBase *ab, struct IOAudio *io)
{
    HEADER
    UBYTE ch;
    
    D(bug("ADCMD_SETPREC %02x %04x\n", mask, key));
    for (ch = 0; ch < NR_CH; ch++) {
    	if ((mask & (1 << ch)) && key == ab->key[ch]) {
    	    ab->pri[ch] = io->ioa_Request.io_Message.mn_Node.ln_Pri;
    	    setunit(io, ch);
    	}
    }
    if (!io->ioa_Request.io_Unit)
    	io->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    return TRUE;
}

static BOOL ADCMD_NULL_f(struct AudioBase *ab, struct IOAudio *io)
{
    HEADER
    UBYTE ch;

    for (ch = 0; ch < NR_CH; ch++) {
    	if ((mask & (1 << ch)) && key == ab->key[ch])
    	    setunit(io, ch);
    }
    if (!io->ioa_Request.io_Unit)
    	io->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    return TRUE;
}

static BOOL ADCMD_FLUSH_f(struct AudioBase *ab, struct IOAudio *io)
{
    HEADER
    UBYTE ch;

    D(bug("ADCMD_FLUSH %02x %04x\n", mask, key));
    for (ch = 0; ch < NR_CH; ch++) {
    	if ((mask & (1 << ch)) && key == ab->key[ch]) {
	    abort_ch(ab, ch);
    	    setunit(io, ch);
    	}
    }
    if (!io->ioa_Request.io_Unit)
    	io->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    return TRUE;
}

static BOOL ADCMD_FINISH_f(struct AudioBase *ab, struct IOAudio *io)
{
    HEADER
    UBYTE ch;
    BOOL ret, added;

    D(bug("ADCMD_FINISH %02x %04x\n", mask, key));
    ret = TRUE;
    added = FALSE;
    Disable();
    for (ch = 0; ch < NR_CH; ch++) {
    	if ((mask & (1 << ch)) && key == ab->key[ch]) {
    	    setunit(io, ch);
    	    if (getnextwrite(ab, ch, FALSE) && (io->ioa_Request.io_Flags & ADIOF_SYNCCYCLE)) {
    	    	if (!added) {
    	    	    AddTail((struct List*)&ab->misclist, &io->ioa_Request.io_Message.mn_Node);
    	    	    setunit(io, ch + NR_CH);
    	    	    ret = FALSE;
    	    	    added = TRUE;
    	    	}
    	    } else {
    	    	abort_ch(ab, ch);
    	    }
    	}
    }
    Enable();
    if (!io->ioa_Request.io_Unit)
    	io->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    return ret;
}

static BOOL ADCMD_RESET_f(struct AudioBase *ab, struct IOAudio *io)
{
    HEADER
    UBYTE ch;

    D(bug("ADCMD_RESET %02x %04x\n", mask, key));
    for (ch = 0; ch < NR_CH; ch++) {
    	if ((mask & (1 << ch)) && key == ab->key[ch]) {
	    ADCMD_FLUSH_f(ab, io);
	    audiohw_reset(ab, 1 << ch);
    	    setunit(io, ch);
    	    ab->stopmask &= ~(1 << ch);
    	}
    }
    if (!io->ioa_Request.io_Unit)
    	io->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    return TRUE;
}

static BOOL ADCMD_START_f(struct AudioBase *ab, struct IOAudio *io)
{
    HEADER
    UBYTE ch;
    UWORD newmask;

    D(bug("ADCMD_START %02x %04x\n", mask, key));
    newmask = 0;
    for (ch = 0; ch < NR_CH; ch++) {
    	if ((mask & (1 << ch)) && key == ab->key[ch] && (ab->stopmask & (1 << ch))) {
    	    ab->stopmask &= ~(1 << ch);
    	    newmask |= 1 << ch;
    	    setunit(io, ch);
    	}
    }
    if (!io->ioa_Request.io_Unit)
    	io->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    else
    	audiohw_start(ab, newmask);
    return TRUE;
}

static BOOL ADCMD_STOP_f(struct AudioBase *ab, struct IOAudio *io)
{
    HEADER
    UBYTE ch;
    UWORD newmask;

    D(bug("ADCMD_STOP %02x %04x\n", mask, key));
    newmask = 0;
    for (ch = 0; ch < NR_CH; ch++) {
    	if ((mask & (1 << ch)) && key == ab->key[ch]) {
    	    ab->stopmask |= 1 << ch;
    	    newmask |= 1 << ch;
    	    setunit(io, ch);
    	}
    }
    if (!io->ioa_Request.io_Unit) {
    	io->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    } else {
    	audiohw_stop(ab, newmask);
    	abort_waitcycles(ab, newmask);
    }
    return TRUE;
}

static BOOL ADCMD_PERVOL_f(struct AudioBase *ab, struct IOAudio *io)
{
    HEADER
    UBYTE ch;
    BOOL ret, added;

    D(bug("ADCMD_PERVOL %02x %04x\n", mask, key));
    ret = TRUE;
    added = FALSE;
    Disable();
    for (ch = 0; ch < NR_CH; ch++) {
    	if ((mask & (1 << ch)) && key == ab->key[ch]) {
    	    setunit(io, ch);
    	    if (getnextwrite(ab, ch, FALSE) && (io->ioa_Request.io_Flags & ADIOF_SYNCCYCLE)) {
    	    	if (!added) {
    	    	    AddTail((struct List*)&ab->misclist, &io->ioa_Request.io_Message.mn_Node);
    	    	    setunit(io, ch + NR_CH);
    	    	    ret = FALSE;
    	    	    added = TRUE;
    	    	}
    	    } else {
    	        UBYTE flags = io->ioa_Request.io_Flags;
    	        io->ioa_Request.io_Flags |= ADIOF_PERVOL;
    	        audiohw_preparepervol(ab, io, ch);
    	        io->ioa_Request.io_Flags = flags;
    	    }
    	}
    }
    Enable();
    if (!io->ioa_Request.io_Unit)
    	io->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    return ret;
}

static BOOL ADCMD_WRITE_f(struct AudioBase *ab, struct IOAudio *io)
{
    HEADER
    UBYTE ch;
    UWORD newmask;
    BOOL firstempty, secondempty;
    BOOL ret = TRUE;

    D(bug("ADCMD_WRITE %02x %04x\n", mask, key));
    Disable();
    newmask = 0;
    firstempty = secondempty = FALSE;
    for (ch = 0; ch < NR_CH; ch++) {
    	if ((mask & (1 << ch)) && key == ab->key[ch]) {
    	    setunit(io, ch);
    	    firstempty = getnextwrite(ab, ch, FALSE) == NULL;
    	    secondempty = getnextwrite(ab, ch, TRUE) == NULL;
    	    AddTail((struct List*)&ab->writelist[ch], &io->ioa_Request.io_Message.mn_Node);
    	    newmask = 1 << ch;
    	    break;
    	}
    }
    D(bug("unit=%08x 1=%d 2=%d newmask=%02x stopmask=%02x cycles=%d\n",
    	io->ioa_Request.io_Unit, firstempty, secondempty, newmask,
    	ab->stopmask, io->ioa_Cycles));
    if (!io->ioa_Request.io_Unit) {
    	io->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    } else {
    	if (!(ab->stopmask & newmask)) {
    	    if (firstempty) {
    	    	audiohw_start(ab, newmask);
    	    } else if (secondempty) {
    	    	audiohw_prepareptlen(ab, getnextwrite(ab, ch, TRUE), ch);
	    }	
    	}
    	ret = FALSE;
    }
    Enable();
    return ret;
}

static BOOL ADCMD_READ_f(struct AudioBase *ab, struct IOAudio *io)
{
    HEADER
    UBYTE ch;

    D(bug("ADCMD_READ %02x %04x\n", mask, key));
    for (ch = 0; ch < NR_CH; ch++) {
    	if ((mask & (1 << ch)) && key == ab->key[ch]) {
    	    setunit(io, ch);
    	    if (!(ab->stopmask & (1 << ch)))
    	    	io->ioa_Data = (APTR)getnextwrite(ab, ch, FALSE);
    	    break;
    	}
    }
    if (!io->ioa_Request.io_Unit)
    	io->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    return TRUE;
}

static BOOL ADCMD_WAITCYCLE_f(struct AudioBase *ab, struct IOAudio *io)
{
    HEADER
    UBYTE ch;
    BOOL ret;

    D(bug("ADCMD_WAITCYCLE %02x %04x\n", mask, key));
    ret = TRUE;
    for (ch = 0; ch < NR_CH; ch++) {
    	if ((mask & (1 << ch)) && key == ab->key[ch]) {
    	    setunit(io, ch);
    	    if (!(ab->stopmask & (1 << ch)) && getnextwrite(ab, ch, FALSE)) {
    	    	AddTail((struct List*)&ab->misclist, &io->ioa_Request.io_Message.mn_Node);
    	    	setunit(io, ch + NR_CH);
    	    	ret = FALSE;
    	    }
    	    break;
	}
    }
    if (!io->ioa_Request.io_Unit)
    	io->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    return ret;
}

static BOOL processcommand(struct AudioBase *ab, struct IOAudio *io)
{
    
    io->ioa_Request.io_Error = 0;
    switch(io->ioa_Request.io_Command)
    {
        case CMD_CLEAR:
        return ADCMD_NULL_f(ab, io);
        case CMD_FLUSH:
        return ADCMD_FLUSH_f(ab, io);
	case CMD_RESET:
	return ADCMD_RESET_f(ab, io);
	case CMD_UPDATE:
	return ADCMD_NULL_f(ab, io);
	case CMD_START:
	return ADCMD_START_f(ab, io);
	case CMD_STOP:
	return ADCMD_STOP_f(ab, io);
	case CMD_READ:
	return ADCMD_READ_f(ab, io);
	case CMD_WRITE:
	return ADCMD_WRITE_f(ab, io);

    	case ADCMD_ALLOCATE:
    	return ADCMD_ALLOCATE_f(ab, io);
    	case ADCMD_FREE:
    	return ADCMD_FREE_f(ab, io);
    	case ADCMD_LOCK:
    	return ADCMD_LOCK_f(ab, io);
    	case ADCMD_SETPREC:
    	return ADCMD_SETPREC_f(ab, io);
    	case ADCMD_WAITCYCLE:
    	return ADCMD_WAITCYCLE_f(ab, io);
    	case ADCMD_PERVOL:
    	return ADCMD_PERVOL_f(ab, io);
   	case ADCMD_FINISH:
    	return ADCMD_FINISH_f(ab, io);
    	default:
    	io->ioa_Request.io_Error = IOERR_NOCMD;
    }
    return TRUE;
}

AROS_LH1(void, beginio,
 AROS_LHA(struct IOAudio *, io, A1),
 struct AudioBase *, AudioBase, 5, Audio)
{
    AROS_LIBFUNC_INIT

    D(bug("audio beginio %p:%d\n", io, io->ioa_Request.io_Command));
 
    io->ioa_Request.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    if (processcommand(AudioBase, io)) {
    	/* TRUE = finished immediately */
   	if (!(io->ioa_Request.io_Flags & IOF_QUICK))
 	    ReplyMsg(&io->ioa_Request.io_Message);
    } else {
    	/* FALSE = async */
   	io->ioa_Request.io_Flags &= ~IOF_QUICK;
    }

    AROS_LIBFUNC_EXIT
}	

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOAudio *, io, A1),
 struct AudioBase *, AudioBase, 6, Audio)
{
    AROS_LIBFUNC_INIT

    struct IOAudio *node;
    UBYTE ch;

    D(bug("audio abortio %p\n", io));

    io->ioa_Request.io_Error = 0;

    Disable();
    for (ch = 0; ch < NR_CH; ch++) {
    	ForeachNode(&AudioBase->writelist[ch], node) {
    	    if (node == io) {
    	    	REMOVE(io);
    	    	abort_io(AudioBase, io);
    	        break;
    	    }
    	}
    }
    ForeachNode(&AudioBase->misclist, node) {
    	if (node == io) {
    	    REMOVE(io);
    	    abort_io(AudioBase, io);
    	    break;
    	}
    }
    Enable();
    	    
    return 0;

    AROS_LIBFUNC_EXIT
}


static int GM_UNIQUENAME(init)(LIBBASETYPEPTR AudioBase)
{
    NEWLIST(&AudioBase->writelist[0]);
    NEWLIST(&AudioBase->writelist[1]);
    NEWLIST(&AudioBase->writelist[2]);
    NEWLIST(&AudioBase->writelist[3]);
    NEWLIST(&AudioBase->misclist);
    AudioBase->zerosample = AllocMem(2, MEMF_CHIP | MEMF_CLEAR);
    AudioBase->keygen = 0x5a00;
    audiohw_init(AudioBase);
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
    D(bug("Audio open: pri=%d key=%04x data=%p len=%d\n",
	io->ioa_Request.io_Message.mn_Node.ln_Pri,
	io->ioa_AllocKey,
	io->ioa_Data,
	io->ioa_Length));

    io->ioa_Request.io_Error = 0;
    io->ioa_Request.io_Unit = (struct Unit*) NULL;

    if (io->ioa_Length) {
        allocaudio(AudioBase, io);
       	D(bug("new key = %04x\n", io->ioa_AllocKey));
    	io->ioa_Request.io_Device = (struct Device *)AudioBase;
    }

    return io->ioa_Request.io_Error == 0;
}

static int GM_UNIQUENAME(close)
(
    LIBBASETYPEPTR AudioBase,
    struct IOAudio *io
)
{
    UBYTE ch;
    
    for (ch = 0; ch < NR_CH; ch++) {
    	if (io->ioa_AllocKey == AudioBase->key[ch]) {
    	    abort_ch(AudioBase, ch);
    	    AudioBase->key[ch] = 0;
    	    audiohw_reset(AudioBase, 1 << ch);
    	}
    }
    io->ioa_Request.io_Unit = NULL;
    io->ioa_Request.io_Device = (struct Device*)-1;
    
    D(bug("Audio close: key = %04x\n", io->ioa_AllocKey));

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(init), 0)
ADD2OPENDEV(GM_UNIQUENAME(open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(close), 0)
