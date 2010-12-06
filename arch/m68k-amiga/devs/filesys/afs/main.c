/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/filesystem.h>

#include <aros/macros.h>
#include <aros/debug.h>

#include "afshandler.h"
#include "cache.h"
#include "error.h"
#include "filehandles1.h"
#include "filehandles2.h"
#include "filehandles3.h"
#include "misc.h"
#include "volumes.h"

#include "baseredef.h"

static void replypkt(struct DosPacket *dp, LONG res1)
{
	struct MsgPort *mp;
	struct Message *mn;
	
	mp = dp->dp_Port;
	mn = dp->dp_Link;
	mn->mn_Node.ln_Name = (char*)dp;
	dp->dp_Port = &((struct Process*)FindTask(NULL))->pr_MsgPort;
	dp->dp_Res1 = res1;
	PutMsg(mp, mn);
}

static void replypkt2(struct DosPacket *dp, LONG res1, LONG res2)
{
	dp->dp_Res2 = res2;
	replypkt(dp, res1);
}

static struct AFSBase *AFS_alloc(void)
{
    struct AFSBase *handler;

    handler = AllocMem(sizeof(*handler), MEMF_ANY | MEMF_CLEAR);
    if (handler == NULL)
    	return NULL;

    handler->dosbase = (struct DosLibrary *)OpenLibrary("dos.library",0);
    if (handler->dosbase != NULL) {

    	/* Port for device commands */
    	handler->timer_mp = CreateMsgPort();
#if 0
    	NEWLIST(&handler->port.mp_MsgList);
    	handler->port.mp_Node.ln_Type = NT_MSGPORT;
    	handler->port.mp_SigBit = SIGBREAKB_CTRL_F;
    	handler->port.mp_SigTask = task;
    	handler->port.mp_Flags = PA_IGNORE;
#endif

	/* Open timer */
	handler->timer_request = (struct timerequest *)
	    CreateIORequest(handler->timer_mp, sizeof(struct timerequest));
	if (handler->timer_request != NULL) {
	    if (OpenDevice("timer.device", UNIT_VBLANK,
			(APTR)handler->timer_request, 0) == 0) {
		return handler;
	    }
	    DeleteIORequest((struct IORequest *)handler->timer_request);
	}
	CloseLibrary((struct Library *)handler->dosbase);
    }

    return NULL;
}

static void AFS_free(struct AFSBase *handler)
{
    CloseDevice((struct IORequest *)handler->timer_request);
    DeleteIORequest((struct IORequest *)handler->timer_request);
    CloseLibrary((struct Library *)handler->dosbase);
    FreeMem(handler, sizeof(*handler));
}

static struct Volume *AFS_open_volume(struct AFSBase *handler, struct DosPacket *dp, LONG *error)
{
    struct FileSysStartupMsg *fssm = BADDR(dp->dp_Arg2);

    D(bug("AFS: Volume on Device %b, Unit %ld, Flags %ld, Environ %p\n",
    		fssm->fssm_Device, fssm->fssm_Unit, 
    		fssm->fssm_Flags, BADDR(fssm->fssm_Environ)));
    return initVolume(handler, 
    	    NULL, AROS_BSTR_ADDR(fssm->fssm_Device),
    	    fssm->fssm_Unit,
    	    fssm->fssm_Flags,
    	    BADDR(fssm->fssm_Environ),
    	    error);
}

static BOOL AFS_close_volume(struct AFSBase *handler, struct Volume *volume, LONG *io_DosError)
{
    if (!volume->locklist) {
    	uninitVolume(handler, volume);
    	return TRUE;
    }

    *io_DosError = ERROR_OBJECT_IN_USE;
    return FALSE;
}

static VOID startFlushTimer(struct AFSBase *handler)
{
    struct timerequest *request;

    /* Set up delay for next flush */
    if (handler->timer_flags & TIMER_ACTIVE) {
	handler->timer_flags |= TIMER_RESTART;
    } else {
	D(bug("[afs] Starting timer\n"));
	request = handler->timer_request;
	request->tr_node.io_Command = TR_ADDREQUEST;
	request->tr_time.tv_secs = 1;
	request->tr_time.tv_micro = 0;
	SendIO((struct IORequest *)handler->timer_request);
	handler->timer_flags = TIMER_ACTIVE;
    }
}

static VOID onFlushTimer(struct AFSBase *handler, struct Volume *volume)
{
    handler->timer_flags &= ~TIMER_ACTIVE;

    if (handler->timer_flags & TIMER_RESTART)
	startFlushTimer(handler);
    else {
	struct Volume *volume;
	struct BlockCache *blockbuffer;

	D(bug("[afs] Alarm rang.\n"));
	if ((volume->dostype == 0x444f5300) && mediumPresent(&volume->ioh))
	{
	    /* Check if adding volume node needs to be retried */
	    if (volume->volumenode == NULL)
		    attemptAddDosVolume(handler, volume);

	    flushCache(handler, volume);
	    blockbuffer = getBlock(handler, volume, volume->rootblock);
	    if ((blockbuffer->flags & BCF_WRITE) != 0)
	    {
		    writeBlock(handler, volume, blockbuffer, -1);
		    blockbuffer->flags &= ~BCF_WRITE;
	    }
	    if (volume->ioh.flags & IOHF_MOTOR_OFF) {
		    D(bug("[afs 0x%08lX] turning off motor\n", volume));
		    motorOff(handler, &volume->ioh);
		    volume->ioh.flags &= ~IOHF_MOTOR_OFF;
	    }
	}
    }
}

static BOOL mediacheck(struct Volume *volume, LONG *ok, LONG *res2)
{
    if (!mediumPresent(&volume->ioh)) {
	*ok = DOSFALSE;
	*res2 = ERROR_NO_DISK;
	return FALSE;
    }
    return TRUE;
}

/*******************************************
 Name  : AFS_work
 Descr.: main loop (get packets and answer (or not))
 Input : proc - our process structure
 Output: -
********************************************/
void AFS_work(void) {
    struct MsgPort *mp;
    struct DosPacket *dp;
    struct Message *mn;
    struct AFSBase *handler;
    struct Volume *volume;
    LONG retval;
    BOOL dead = FALSE;

    D(bug("[AFS] started\n"));
    mp = &((struct Process *)FindTask(NULL))->pr_MsgPort;
    WaitPort(mp);
    dp = (struct DosPacket *)GetMsg(mp)->mn_Node.ln_Name;
    D(bug("[AFS] start message recevied. port=%p, path='%b'\n", mp, dp->dp_Arg1));

    handler = AFS_alloc();

    if (handler == NULL) {
    	D(bug("[AFS] Can't allocate an instance of the handler for this volume\n"));
    	replypkt(dp, DOSFALSE);
    	return ;
    }

    volume = AFS_open_volume(handler, dp, &retval);
    if (volume == NULL) {
    	D(bug("[AFS] Can't open volume\n"));
    	replypkt2(dp, DOSFALSE, retval);
    	AFS_free(handler);
    	return;
    }

    /* Say that we are going to persist */
    ((struct DeviceNode *)BADDR(dp->dp_Arg3))->dn_Task = mp;

    replypkt(dp, DOSTRUE);

    while (!dead) {
    	ULONG packetmask = 1L << mp->mp_SigBit;
    	ULONG timermask = 1L << handler->timer_mp->mp_SigBit;
    	ULONG sigs;

    	sigs = Wait(packetmask | timermask);

    	if (sigs & timermask)
    	    onFlushTimer(handler, volume);

    	if (!(sigs & packetmask))
    	    continue;

    	/* DOS Packet processing */

    	while ((mn = GetMsg(mp)) != NULL) {
    	    LONG res2 = 0;
    	    LONG ok = DOSFALSE;

    	    dp = (struct DosPacket *)mn->mn_Node.ln_Name;

    	    D(bug("[AFS] packet %p:%d\n", dp, dp->dp_Type));
    	    startFlushTimer(handler);

    	    switch (dp->dp_Type) {
    	    case ACTION_DIE:
    	    	if (!AFS_close_volume(handler, volume, &res2)) {
    	    	    ok = DOSFALSE;
    	    	    break;
    	    	}
    	    	dead = DOSTRUE;
    	    	ok = TRUE;
    	    	break;
    	    case ACTION_IS_FILESYSTEM:
    	    	ok = TRUE;
    	    	break;
	    case ACTION_INHIBIT:
	    	res2 = inhibit(handler, volume, dp->dp_Arg1);
	    	if (res2 != 0)
	    	    ok = DOSFALSE;
	    	break;
	    case ACTION_END:
	    	closef(handler, (struct AfsHandle *)dp->dp_Arg1);
	    	ok = DOSTRUE;
	    	break;
	    case ACTION_MORE_CACHE:
		{
		    LONG numbuff = dp->dp_Arg1;

		    if (numbuff) {
		    	volume->numbuffers += numbuff;

		    	if (volume->numbuffers < 1)
		    	    volume->numbuffers = 1;

		    	flushCache(handler, volume);
		    	Forbid();
		    	freeCache(handler, volume->blockcache);
		    	for (;;) {
		    	    volume->blockcache = initCache(handler, volume, volume->numbuffers);
		    	    if (volume->blockcache)
		    	    	break;
		    	    volume->numbuffers /= 2;
		    	    if (volume->numbuffers < 1)
		    	    	volume->numbuffers = 1;
		    	}
		    }
		    ok = DOSTRUE;
		    res2 = volume->numbuffers;
		}
		break;
	    case ACTION_DISK_INFO:
	    	ok = getDiskInfo(volume, BADDR(dp->dp_Arg1)) ? DOSFALSE : DOSTRUE;
	    	break;
	    case ACTION_SAME_LOCK:
	    {
		struct FileLock   *f1 = BADDR(dp->dp_Arg1);
		struct FileLock   *f2 = BADDR(dp->dp_Arg2);
	    	ok = sameLock((struct AfsHandle*)f1->fl_Key, (struct AfsHandle*)f2->fl_Key);
		break;
	    }
    	    default:

		switch (dp->dp_Type) {
		case ACTION_FINDINPUT:
		case ACTION_FINDOUTPUT:
		case ACTION_FINDUPDATE:
		{
		    struct FileHandle *fh = BADDR(dp->dp_Arg1);
		    struct FileLock   *dl = BADDR(dp->dp_Arg2);
		    CONST_STRPTR       fn = AROS_BSTR_ADDR(dp->dp_Arg3);
		    CONST_STRPTR       cp;
		    struct AfsHandle  *dh;
		    struct AfsHandle  *ah;
		    ULONG mode = 0;

		    if (!mediacheck(volume, &ok, &res2))
		    	break;

		    if (dl == NULL)
		    	dh = &volume->ah;
		    else
		    	dh = (APTR)(dl->fl_Key);

		    if (dp->dp_Type == ACTION_FINDINPUT)
			mode = FMF_MODE_OLDFILE;
		    if (dp->dp_Type == ACTION_FINDOUTPUT)
			mode = FMF_MODE_NEWFILE;
		    if (dp->dp_Type == ACTION_FINDUPDATE)
			mode = FMF_MODE_READWRITE;

		    /* Skip past device names */
		    for (cp = fn; *cp; cp++) {
		    	if (*cp == '/')
		    	    break;
		    	if (*cp == ':') {
		    	    fn = cp+1;
		    	    break;
		    	}
		    }

		    ah = openf(handler, dh, fn, mode, &res2);
		    ok = (ah != NULL) ? DOSTRUE : DOSFALSE;
		    if (ok)
			fh->fh_Arg1 = (LONG)(IPTR)ah;
		    break;
		}
		case ACTION_LOCATE_OBJECT:
		{
		    struct FileLock   *dl = BADDR(dp->dp_Arg1);
		    struct FileLock   *fl;
		    CONST_STRPTR       fn = AROS_BSTR_ADDR(dp->dp_Arg2);
		    CONST_STRPTR       cp;
		    struct AfsHandle  *dh;
		    struct AfsHandle  *ah;
		    ULONG mode = 0;

		    if (!mediacheck(volume, &ok, &res2))
		    	break;

		    if (dl == NULL)
		    	dh = &volume->ah;
		    else
		    	dh = (APTR)(dl->fl_Key);

		    if (dp->dp_Arg3 == ACCESS_READ)
		    	mode |= FMF_MODE_OLDFILE;
		    else if (dp->dp_Arg3 == ACCESS_WRITE)
		    	mode |= FMF_MODE_NEWFILE;

		    /* Skip past device names */
		    for (cp = fn; *cp; cp++) {
		    	if (*cp == '/')
		    	    break;
		    	if (*cp == ':') {
		    	    fn = cp+1;
		    	    break;
		    	}
		    }

		    ah = openf(handler, dh, fn, mode, &res2);
		    if (ah == NULL) {
		    	ok = DOSFALSE;
		    	break;
		    }
		    fl = AllocMem(sizeof(*fl), MEMF_CLEAR);
		    if (fl != NULL) {
			fl->fl_Link = BNULL;
			fl->fl_Key = (LONG)(IPTR)ah;
			fl->fl_Access = dp->dp_Arg3;
			fl->fl_Task = mp;
			fl->fl_Volume = MKBADDR(&volume->devicelist);
			ok = MKBADDR(fl);
			res2 = 0;
		    } else {
			closef(handler, ah);
			ok = DOSFALSE;
			res2 = ERROR_NO_FREE_STORE;
		    }
		    break;
		}
		case ACTION_COPY_DIR:	/* Aka DupLock() */
		{
		    struct FileLock   *ol = BADDR(dp->dp_Arg1);
		    struct FileLock   *fl;
		    struct AfsHandle  *oh;
		    struct AfsHandle  *ah;
		    ULONG mode = 0;

		    if (!mediacheck(volume, &ok, &res2))
		    	break;

		    if (ol == NULL) {
		    	ok = DOSTRUE;
		    	res2 = BNULL;
		    	break;
		    }

		    if (ol->fl_Access == ACCESS_READ)
		    	mode |= FMF_MODE_OLDFILE;
		    else if (ol->fl_Access == ACCESS_WRITE)
		    	mode |= FMF_MODE_NEWFILE;

		    oh = (APTR)ol->fl_Key;
		    ah = openf(handler, oh, "", mode, &res2);
		    if (ah == NULL) {
		    	ok = DOSFALSE;
		    	break;
		    }
		    fl = AllocMem(sizeof(*fl), MEMF_CLEAR);
		    if (fl != NULL) {
		        fl->fl_Link = BNULL;
		        fl->fl_Key = (LONG)(IPTR)ah;
		        fl->fl_Access = ol->fl_Access;
		        fl->fl_Task = mp;
		        fl->fl_Volume = MKBADDR(&volume->devicelist);
		        ok = MKBADDR(fl);
		        res2 = 0;
		    } else {
		        closef(handler, ah);
		        ok = DOSFALSE;
		        res2 = ERROR_NO_FREE_STORE;
		    }
		    break;
		}
		case ACTION_FREE_LOCK:
		{
		    struct FileLock  *fl;
		    struct AfsHandle *ah;

		    if (!mediacheck(volume, &ok, &res2))
		    	break;

		    fl = BADDR(dp->dp_Arg1);
		    if (fl == NULL) {
		    	ok = DOSTRUE;
		    	break;
		    }
		    ah = (APTR)(fl->fl_Key);
		    closef(handler, ah);
		    ok = DOSTRUE;
		    break;
		}
		case ACTION_READ:
		    if (!mediacheck(volume, &ok, &res2))
		    	break;
		    ok = readf(handler, (struct AfsHandle *)(dp->dp_Arg1), (APTR)dp->dp_Arg2, (LONG)dp->dp_Arg3, &res2);
		    break;
		case ACTION_WRITE:
		    if (!mediacheck(volume, &ok, &res2))
		    	break;
		    ok = writef(handler, (struct AfsHandle *)(dp->dp_Arg1), (APTR)dp->dp_Arg2, (LONG)dp->dp_Arg3, &res2);
		    break;
		case ACTION_SEEK:
		    if (!mediacheck(volume, &ok, &res2))
		    	break;
		    ok = seek(handler, (struct AfsHandle *)(dp->dp_Arg1), (LONG)dp->dp_Arg2, (LONG)dp->dp_Arg3, &res2);
		    break;
		case ACTION_SET_FILE_SIZE:
		    if (!mediacheck(volume, &ok, &res2))
		    	break;
		    ok = setFileSize(handler, (struct AfsHandle *)(((struct FileHandle *)(dp->dp_Arg1))->fh_Arg1),(LONG)dp->dp_Arg2, (LONG)dp->dp_Arg3, &res2);
		    break;
		case ACTION_EXAMINE_OBJECT:
		{
		    struct FileLock      *fl = BADDR(dp->dp_Arg1);
		    struct AfsHandle     *ah;
		    struct FileInfoBlock *fib = BADDR(dp->dp_Arg2);
		    UBYTE buffer[sizeof(struct ExAllData) + MAXFILENAMELENGTH + MAXCOMMENTLENGTH];
		    struct ExAllData     *ead = (APTR)&buffer[0];
		    ULONG size = sizeof(buffer);
		    ULONG mode = ED_OWNER;
		    ULONG dirpos;

		    if (!mediacheck(volume, &ok, &res2))
		    	break;

		    if (fib == NULL) {
			ok = DOSTRUE;
			break;
		    }

		    if (fl == NULL)
			ah = &volume->ah;
		    else
			ah = (APTR)fl->fl_Key;

		    res2 = examine(handler, ah, ead, size, mode, &dirpos);
		    if (res2 != 0) {
			ok = DOSFALSE;
			break;
		    }

		    if (ah == &volume->ah) {
			fib->fib_DirEntryType = ST_ROOT;
		    } else {
			fib->fib_DirEntryType = ead->ed_Type;
		    }

		    fib->fib_DiskKey = ah->header_block;
		    strncpy(&fib->fib_FileName[1], ead->ed_Name, sizeof(fib->fib_FileName)-1);
		    fib->fib_FileName[0] = strlen(&fib->fib_FileName[1]);
		    fib->fib_Protection = ead->ed_Prot;
		    fib->fib_EntryType = fib->fib_DirEntryType;
		    fib->fib_Size = ead->ed_Size;
		    fib->fib_NumBlocks = (ead->ed_Size + ah->volume->sectorsize - 1) / ah->volume->sectorsize;
		    fib->fib_Date.ds_Days = ead->ed_Days;
		    fib->fib_Date.ds_Minute = ead->ed_Mins;
		    fib->fib_Date.ds_Tick = ead->ed_Ticks;
		    strncpy(&fib->fib_Comment[1], ead->ed_Comment, sizeof(fib->fib_Comment)-1);
		    fib->fib_Comment[0] = strlen(&fib->fib_Comment[1]);
		    fib->fib_OwnerUID = ead->ed_OwnerUID;
		    fib->fib_OwnerGID = ead->ed_OwnerGID;
		    ok = DOSTRUE;
		    break;
		}
		case ACTION_EXAMINE_NEXT:
		{
		    struct FileLock      *fl = BADDR(dp->dp_Arg1);
		    struct AfsHandle     *ah;
		    struct FileInfoBlock *fib = BADDR(dp->dp_Arg2);

		    if (!mediacheck(volume, &ok, &res2))
		    	break;

		    if (fib == NULL) {
			ok = DOSTRUE;
			break;
		    }

		    if (fl == NULL)
			ah = &volume->ah;
		    else
			ah = (APTR)fl->fl_Key;

		    res2 = examineNext(handler, ah, fib);
		    if (res2 != 0) {
			ok = DOSFALSE;
			break;
		    }

		    if (ah == &volume->ah) {
			fib->fib_DirEntryType = ST_ROOT;
		    }
		    ok = DOSTRUE;
		    break;
		}
		case ACTION_PARENT:
		case ACTION_PARENT_FH:
		{
		    struct FileLock   *opl = BADDR(dp->dp_Arg1);
		    struct FileHandle *oph = BADDR(dp->dp_Arg1);
		    struct FileLock   *fl;
		    struct AfsHandle  *oh;
		    struct AfsHandle  *ah;

		    if (!mediacheck(volume, &ok, &res2))
		    	break;

		    if (dp->dp_Type == ACTION_PARENT)
		    	oh = (APTR)opl->fl_Key;
		    else
		    	oh = (APTR)oph->fh_Arg1;
		    ah = openf(handler, oh, "/", FMF_MODE_OLDFILE, &res2);
		    if (ah == NULL) {
		    	ok = DOSFALSE;
		    	break;
		    }
		    fl = AllocMem(sizeof(*fl), MEMF_CLEAR);
		    if (fl != NULL) {
		        fl->fl_Link = BNULL;
		        fl->fl_Key = (LONG)(IPTR)ah;
		        fl->fl_Access = ACCESS_READ;
		        fl->fl_Task = mp;
		        fl->fl_Volume = MKBADDR(&volume->devicelist);
		        ok = MKBADDR(fl);
		        res2 = 0;
		    } else {
		        closef(handler, ah);
		        ok = DOSFALSE;
		        res2 = ERROR_NO_FREE_STORE;
		    }
		    break;
		}
		default:
		    ok = DOSFALSE;
		    res2 = ERROR_NOT_IMPLEMENTED;
		    D(bug("[AFS] %d not implemented\n", dp->dp_Type));
		    break;
		}
    	    	break;
    	    }

    	    replypkt2(dp, ok, res2);
    	}
    }

    AFS_free(handler);
    replypkt(dp, DOSFALSE);
    return;
}

