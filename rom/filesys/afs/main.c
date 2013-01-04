/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include <proto/dos.h>
#include <proto/exec.h>

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

static void replypkt(struct DosPacket *dp, SIPTR res1)
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

static void replypkt2(struct DosPacket *dp, SIPTR res1, SIPTR res2)
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

	/* changeint task and sigbit */
	handler->port.mp_SigTask = FindTask(0);
	handler->port.mp_SigBit = SIGBREAKB_CTRL_F;

    	/* Port for device commands */
    	handler->timer_mp = CreateMsgPort();
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
    if (handler->timer_flags & TIMER_ACTIVE) {
	AbortIO((struct IORequest *)handler->timer_request);
	WaitIO((struct IORequest *)handler->timer_request);
    }
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

static BOOL AFS_close_volume(struct AFSBase *handler, struct Volume *volume, SIPTR *io_DosError)
{
    if (!volume->locklist) {
    	uninitVolume(handler, volume);
    	return TRUE;
    }

    *io_DosError = ERROR_OBJECT_IN_USE;
    return FALSE;
}

static BOOL AFS_protect_volume(struct AFSBase *handler, struct Volume *volume, BOOL on, ULONG key, SIPTR *io_DosError)
{
    if (!volume->locklist)
    {
        LONG error = writeprotectVolume(handler, volume, on, key);
        if (error == 0)
        {
            return TRUE;
        } else {
            *io_DosError = error;
            return FALSE;
        }
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
	/* D(bug("[afs] Starting timer\n")); */
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

    if (handler->timer_flags & TIMER_RESTART) {
	startFlushTimer(handler);
    } else {
	struct BlockCache *blockbuffer;

	/* D(bug("[afs] Alarm rang.\n")); */
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

static BOOL mediacheck(struct Volume *volume, SIPTR *ok, SIPTR *res2)
{
    if (!mediumPresent(&volume->ioh)) {
	*ok = DOSFALSE;
	*res2 = ERROR_NO_DISK;
	return FALSE;
    }
    return TRUE;
}

static LONG gethandletype(struct AFSBase *handler,  struct AfsHandle  *h)
{
    UBYTE buffer[sizeof(struct ExAllData) + MAXFILENAMELENGTH];
    struct ExAllData *ead = (APTR)&buffer[0];
    ULONG size = sizeof(buffer);
    ULONG mode = ED_TYPE;
    IPTR dirpos;
    LONG res2;
    
    res2 = examine(handler, h, ead, size, mode, &dirpos);
    if (res2 == 0)
    	return ead->ed_Type;
    return 0;
}

static CONST_STRPTR skipdevname(CONST_STRPTR fn)
{
    CONST_STRPTR cp;
    /* Skip past device names */
    for (cp = fn; *cp; cp++) {
    	if (*cp == '/')
    	    break;
    	if (*cp == ':') {
  	    fn = cp+1;
    	    break;
    	}
    }
    return fn;
}

/*******************************************
 Name  : AFS_work
 Descr.: main loop (get packets and answer (or not))
 Input : proc - our process structure
 Output: -
********************************************/
LONG AFS_work(struct ExecBase *SysBase)
{
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
    	return RETURN_FAIL;
    }

    volume = AFS_open_volume(handler, dp, &retval);
    if (volume == NULL) {
    	D(bug("[AFS] Can't open volume\n"));
    	replypkt2(dp, DOSFALSE, retval);
    	AFS_free(handler);
    	return RETURN_FAIL;
    }

    /* make non-packet functions to see our volume */
    NEWLIST(&handler->device_list);
    AddHead(&handler->device_list, &volume->ln);

    /* Say that we are going to persist */
    ((struct DeviceNode *)BADDR(dp->dp_Arg3))->dn_Task = mp;

    replypkt(dp, DOSTRUE);

    while (!dead) {
    	ULONG packetmask = 1L << mp->mp_SigBit;
    	ULONG timermask = 1L << handler->timer_mp->mp_SigBit;
    	ULONG changemask = 1L << SIGBREAKB_CTRL_F;
    	ULONG sigs;

    	sigs = Wait(packetmask | timermask | changemask);

    	if (sigs & timermask)
    	    onFlushTimer(handler, volume);
    	if (sigs & changemask) {
    	    checkDeviceFlags(handler);
	}

    	if (!(sigs & packetmask))
    	    continue;

    	/* DOS Packet processing */

    	while ((mn = GetMsg(mp)) != NULL) {
    	    SIPTR res2 = 0;
    	    SIPTR ok = DOSFALSE;

    	    dp = (struct DosPacket *)mn->mn_Node.ln_Name;

    	    D(bug("[AFS] packet %p:%d\n", dp, dp->dp_Type));
    	    startFlushTimer(handler);

    	    switch (dp->dp_Type) {
    	    case ACTION_DIE:
    	    	if (!AFS_close_volume(handler, volume, &res2)) {
    	    	    ok = FALSE;
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
	    	ok = res2 ? DOSFALSE : DOSTRUE;
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
	    	ok = (sameLock((struct AfsHandle*)f1->fl_Key, (struct AfsHandle*)f2->fl_Key) == LOCK_SAME) ? DOSTRUE : DOSFALSE;
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
		    struct AfsHandle  *dh;
		    struct AfsHandle  *ah;
		    ULONG mode = 0;

		    if (!mediacheck(volume, &ok, &res2))
		    	break;

		    if (dl == NULL)
		    	dh = &volume->ah;
		    else
		    	dh = (APTR)(dl->fl_Key);

		    /* MODE_* directly matches its 
		     * corresponding ACTION_* counterpart.
		     */
		    mode = dp->dp_Type;

		    fn = skipdevname(fn);

		    if (dp->dp_Type == ACTION_FINDOUTPUT) {
		        ah = openfile(handler, dh, fn, mode, 0, &res2);	
		    } else {
		        ah = openf(handler, dh, fn, mode, &res2);
		    }
		    ok = (ah != NULL) ? DOSTRUE : DOSFALSE;
		    if (ok) {
		    	if (dp->dp_Type != ACTION_FINDOUTPUT) {
		            LONG type = gethandletype(handler, ah);
		            if (type >= 0) {
		    	        /* was directory */
		    	        res2 = ERROR_OBJECT_WRONG_TYPE;
		    	        ok = DOSFALSE;
		    	        closef(handler, ah);
		    	        break;
		    	    }
		    	}
		    	fh->fh_Arg1 = (LONG)(IPTR)ah;
		    }
		    break;
		}
		case ACTION_LOCATE_OBJECT:
		{
		    struct FileLock   *dl = BADDR(dp->dp_Arg1);
		    struct FileLock   *fl;
		    CONST_STRPTR       fn = AROS_BSTR_ADDR(dp->dp_Arg2);
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
		    	mode = MODE_OLDFILE;
		    else if (dp->dp_Arg3 == ACCESS_WRITE)
		    	mode = MODE_NEWFILE;

		    fn = skipdevname(fn);

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
			ok = (SIPTR)MKBADDR(fl);
			res2 = 0;
		    } else {
			closef(handler, ah);
			ok = DOSFALSE;
			res2 = ERROR_NO_FREE_STORE;
		    }
		    break;
		}
		case ACTION_COPY_DIR:	/* Aka DupLock() */
		case ACTION_COPY_DIR_FH:
		{
		    struct FileLock   *ol = BADDR(dp->dp_Arg1);
		    struct FileLock   *fl;
		    struct AfsHandle  *oh;
		    struct AfsHandle  *ah;

		    if (!mediacheck(volume, &ok, &res2))
		    	break;

		    if (dp->dp_Type == ACTION_COPY_DIR) {
		    	oh = ol ? (APTR)ol->fl_Key : &volume->ah;
		    } else {
		    	oh = (APTR)dp->dp_Arg1;
		    }

		    ah = openf(handler, oh, "", MODE_OLDFILE, &res2);
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
		        ok = (SIPTR)MKBADDR(fl);
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
		    FreeMem(fl, sizeof(*fl));
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
		    ok = setFileSize(handler, (struct AfsHandle *)(dp->dp_Arg1),(LONG)dp->dp_Arg2, (LONG)dp->dp_Arg3, &res2);
		    break;
		case ACTION_FH_FROM_LOCK:
		{
		    struct FileHandle    *fh = BADDR(dp->dp_Arg1);
		    struct FileLock      *fl = BADDR(dp->dp_Arg2);
		    fh->fh_Arg1 = (LONG)(IPTR)fl->fl_Key;
		    FreeMem(fl, sizeof(*fl));
		    break;
		}
		case ACTION_EXAMINE_OBJECT:
		case ACTION_EXAMINE_FH:
		{
		    struct FileLock      *fl = BADDR(dp->dp_Arg1);
		    struct AfsHandle     *ah;
		    struct FileInfoBlock *fib = BADDR(dp->dp_Arg2);
		    UBYTE buffer[sizeof(struct ExAllData) + MAXFILENAMELENGTH + MAXCOMMENTLENGTH];
		    struct ExAllData     *ead = (APTR)&buffer[0];
		    ULONG size = sizeof(buffer);
		    ULONG mode = ED_OWNER;
		    SIPTR dirpos;

		    if (!mediacheck(volume, &ok, &res2))
		    	break;

		    if (dp->dp_Type == ACTION_EXAMINE_OBJECT)
		    	ah = fl ? (APTR)fl->fl_Key : &volume->ah;
		    else
		    	ah = (APTR)dp->dp_Arg1;

		    if (fib == NULL) {
			ok = DOSTRUE;
			break;
		    }

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
		    if (ead->ed_Comment) {
			strncpy(&fib->fib_Comment[1], ead->ed_Comment, sizeof(fib->fib_Comment)-1);
			fib->fib_Comment[0] = strlen(&fib->fib_Comment[1]);
		    } else {
		    	fib->fib_Comment[0] = 0;
		    }
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
		    struct FileLock   *fl;
		    struct AfsHandle  *oh;
		    struct AfsHandle  *ah;

		    if (!mediacheck(volume, &ok, &res2))
		    	break;

		    if (dp->dp_Type == ACTION_PARENT)
		    	oh = (APTR)opl->fl_Key;
		    else
		    	oh = (APTR)dp->dp_Arg1;
		    ah = openf(handler, oh, "/", MODE_OLDFILE, &res2);
		    if (ah == NULL) {
		    	ok = DOSFALSE;
		    	if (res2 == ERROR_OBJECT_NOT_FOUND && gethandletype(handler, oh) == ST_ROOT)
			    res2 = 0; /* need to return res2 = 0 (not 205) if oh == ST_ROOT */
		    	break;
		    }
		    fl = AllocMem(sizeof(*fl), MEMF_CLEAR);
		    if (fl != NULL) {
		        fl->fl_Link = BNULL;
		        fl->fl_Key = (LONG)(IPTR)ah;
		        fl->fl_Access = ACCESS_READ;
		        fl->fl_Task = mp;
		        fl->fl_Volume = MKBADDR(&volume->devicelist);
		        ok = (SIPTR)MKBADDR(fl);
		        res2 = 0;
		    } else {
		        closef(handler, ah);
		        ok = DOSFALSE;
		        res2 = ERROR_NO_FREE_STORE;
		    }
		    break;
		}
	    	case ACTION_INFO:
	    	{
	    	    struct FileLock   *opl = BADDR(dp->dp_Arg1);
	    	    struct AfsHandle  *oh;
		    if (opl == NULL)
			oh = &volume->ah;
		    else
			oh = (APTR)opl->fl_Key;
	    	    if (gethandletype(handler, oh) == 0 || !mediacheck(volume, &ok, &res2)) {
	    	    	res2 = ERROR_OBJECT_NOT_FOUND;
		    	ok = DOSFALSE;
		    }
	    	    res2 = getDiskInfo(volume, BADDR(dp->dp_Arg2));
	    	    ok = res2 ? DOSFALSE : DOSTRUE;
	    	    break;
	    	}
	    	break;
	    	case ACTION_RENAME_OBJECT:
	    	{
	    	    struct FileLock   *opl = BADDR(dp->dp_Arg1);
	    	    //unused//struct FileLock   *npl = BADDR(dp->dp_Arg3);
	    	    CONST_STRPTR       on = AROS_BSTR_ADDR(dp->dp_Arg2);
	    	    CONST_STRPTR       nn = AROS_BSTR_ADDR(dp->dp_Arg4);
	    	    struct AfsHandle  *oh;

		    if (opl == NULL)
			oh = &volume->ah;
		    else
			oh = (APTR)opl->fl_Key;
		    on = skipdevname(on);
		    nn = skipdevname(nn);
	    	    res2 = renameObject(handler, oh, on, nn);
	    	    ok = res2 ? DOSFALSE : DOSTRUE;
	    	    break;
	    	}
	    	case ACTION_RENAME_DISK:
	    	{
	    	    CONST_STRPTR       n = AROS_BSTR_ADDR(dp->dp_Arg1);
		    ok = relabel(handler, volume, n, &res2);
	    	    break;
		}
	    	case ACTION_CREATE_DIR:
	    	{
	    	    struct FileLock   *fl = BADDR(dp->dp_Arg1), *flnew;
	    	    struct AfsHandle  *h, *ah;
	    	    CONST_STRPTR       n = AROS_BSTR_ADDR(dp->dp_Arg2);
		    if (fl == NULL)
			h = &volume->ah;
		    else
			h = (APTR)fl->fl_Key;
		    n = skipdevname(n);
		    flnew = AllocMem(sizeof(*flnew), MEMF_CLEAR);
		    if (flnew != NULL) {
		    	ah = createDir(handler, h, n, 0, &res2);
		    	ok = res2 ? DOSFALSE : DOSTRUE;
		    	if (ok) {
		            flnew->fl_Link = BNULL;
		            flnew->fl_Key = (LONG)(IPTR)ah;
		            flnew->fl_Access = ACCESS_READ;
		            flnew->fl_Task = mp;
		            flnew->fl_Volume = MKBADDR(&volume->devicelist);
		            ok = (SIPTR)MKBADDR(flnew);
		        }
		    } else {
		        ok = DOSFALSE;
		        res2 = ERROR_NO_FREE_STORE;
		    }
	    	    break;
		}
	    	case ACTION_DELETE_OBJECT:
	    	{
	    	    struct FileLock   *fl = BADDR(dp->dp_Arg1);
	    	    struct AfsHandle  *h;
	    	    CONST_STRPTR       n = AROS_BSTR_ADDR(dp->dp_Arg2);
		    if (fl == NULL)
			h = &volume->ah;
		    else
			h = (APTR)fl->fl_Key;
		    n = skipdevname(n);
		    res2 = deleteObject(handler, h, n);
		    ok = res2 ? DOSFALSE : DOSTRUE;
	    	    break;
		}
	    	case ACTION_SET_COMMENT:
	    	{
	    	    struct FileLock   *fl = BADDR(dp->dp_Arg2);
	    	    struct AfsHandle  *h;
	    	    CONST_STRPTR       n = AROS_BSTR_ADDR(dp->dp_Arg3);
	    	    CONST_STRPTR       c = AROS_BSTR_ADDR(dp->dp_Arg4);
		    if (fl == NULL)
			h = &volume->ah;
		    else
			h = (APTR)fl->fl_Key;
		    n = skipdevname(n);
		    res2 = setComment(handler, h, n, c);
		    ok = res2 ? DOSFALSE : DOSTRUE;
	    	    break;
		}
	    	case ACTION_SET_PROTECT:
	    	{
	    	    struct FileLock   *fl = BADDR(dp->dp_Arg2);
	    	    struct AfsHandle  *h;
	    	    CONST_STRPTR       n = AROS_BSTR_ADDR(dp->dp_Arg3);
	    	    ULONG              p = dp->dp_Arg4;
		    if (fl == NULL)
			h = &volume->ah;
		    else
			h = (APTR)fl->fl_Key;
		    n = skipdevname(n);
		    res2 = setProtect(handler, h, n, p);
		    ok = res2 ? DOSFALSE : DOSTRUE;
	    	    break;
		}
	    	case ACTION_SET_DATE:
	    	{
	    	    struct FileLock   *fl = BADDR(dp->dp_Arg2);
	    	    struct AfsHandle  *h;
	    	    CONST_STRPTR       n = AROS_BSTR_ADDR(dp->dp_Arg3);
	    	    struct DateStamp *ds = (struct DateStamp*)dp->dp_Arg4;
		    if (fl == NULL)
			h = &volume->ah;
		    else
			h = (APTR)fl->fl_Key;
		    n = skipdevname(n);
		    res2 = setDate(handler, h, n, ds);
		    ok = res2 ? DOSFALSE : DOSTRUE;
	    	    break;
		}
		case ACTION_INHIBIT:
		    res2 = inhibit(handler, volume, dp->dp_Arg1);
		    ok = res2 ? DOSFALSE : DOSTRUE;
		    break;
		case ACTION_FORMAT:
		{
	    	    CONST_STRPTR       n = AROS_BSTR_ADDR(dp->dp_Arg1);
	    	    res2 = format(handler, volume, n, dp->dp_Arg2);
		    ok = res2 ? DOSFALSE : DOSTRUE;
	    	    break;
		}
		case ACTION_SERIALIZE_DISK:
		    ok = relabel(handler, volume, NULL, &res2);
		    break;
		case ACTION_WRITE_PROTECT:
			ok = AFS_protect_volume(handler, volume, dp->dp_Arg1, dp->dp_Arg2, &res2) ? DOSFALSE : DOSTRUE;
			break;
		default:
		    ok = DOSFALSE;
		    res2 = ERROR_ACTION_NOT_KNOWN;
		    D(bug("[AFS] %d not implemented\n", dp->dp_Type));
		    break;
		}
    	    	break;
    	    }

    	    replypkt2(dp, ok, res2);
    	}
    }

    AFS_free(handler);
    replypkt(dp, DOSTRUE);

    return RETURN_OK;
}

