#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/alerts.h>
#include <utility/tagitem.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <devices/conunit.h>

#include <stddef.h>
#include <string.h>

#include "con_handler_intern.h"
#include "support.h"

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

static char *BSTR2C(BSTR srcs)
{
    UBYTE *src = BADDR(srcs);
    char *dst;

    dst = AllocVec(src[0] + 1, MEMF_ANY);
    if (!dst)
        return NULL;
    memcpy(dst, src + 1, src[0]);
    dst[src[0]] = 0;
    return dst;
}
static WORD isdosdevicec(CONST_STRPTR s)
{
    UBYTE b = 0;
    while (s[b])
    {
        if (s[b] == ':')
            return b;
        b++;
    }
    return -1;
}

#define ioReq(x) ((struct IORequest *)x)

static const struct NewWindow default_nw =
{
    0,              /* LeftEdge */
    0,              /* TopEdge */
    -1,             /* Width */
    -1,             /* Height */
    1,              /* DetailPen */
    0,              /* BlockPen */
    0,              /* IDCMP */
    WFLG_DEPTHGADGET   |
    WFLG_SIZEGADGET    |
    WFLG_DRAGBAR       |
    WFLG_SIZEBRIGHT    |
    WFLG_SMART_REFRESH |
    WFLG_ACTIVATE,
    0,              /* FirstGadget */
    0,              /* CheckMark */
    "CON:",         /* Title */
    0,              /* Screen */
    0,              /* Bitmap */
    100,            /* MinWidth */
    70,             /* MinHeight */
    32767,          /* MaxWidth */
    32767,          /* MaxHeight */
    WBENCHSCREEN    /* type */
};


static LONG MakeConWindow(struct filehandle *fh)
{
    LONG err = 0;

    if (fh->otherwindow == NULL)
    {
        struct TagItem win_tags[] =
        {
            { WA_PubScreen,         0 },
            { WA_AutoAdjust,        TRUE },
            { WA_PubScreenName,     0 },
            { WA_PubScreenFallBack, TRUE },
            { TAG_DONE }
        };

        win_tags[2].ti_Data = (IPTR) fh->screenname;
        D(bug("[contask] Opening window on screen %s, IntuitionBase = 0x%p\n", fh->screenname, IntuitionBase));

        /*  Autoadjust doesn't enforce the window's width and height to be larger than
         minwidth and minheight, so we set it here to avoid crashes in devs/console
         if a user does e.g. dir >con:0/0/0/0
         */
        fh->nw.Width = fh->nw.Width > fh->nw.MinWidth ? fh->nw.Width : -1;
        fh->nw.Height = fh->nw.Height == -1 || fh->nw.Height > fh->nw.MinHeight ? fh->nw.Height : fh->nw.MinHeight;

        fh->window = OpenWindowTagList(&fh->nw, (struct TagItem *) win_tags);
    }
    else
    {
        D(bug("[contask] Using window %p\n", fh->otherwindow));
        fh->window = fh->otherwindow;
    }

    if (fh->window)
    {
        D(bug("contask: window opened\n"));
        fh->conreadio->io_Data = (APTR) fh->window;
        fh->conreadio->io_Length = sizeof(struct Window);

        if (0 == OpenDevice("console.device", CONU_SNIPMAP, ioReq(fh->conreadio), 0))
        {
            const UBYTE lf_on[] =
            { 0x9B, 0x32, 0x30, 0x68 }; /* Set linefeed mode    */

            D(bug("contask: device opened\n"));

            fh->flags |= FHFLG_CONSOLEDEVICEOPEN;

            fh->conwriteio = *fh->conreadio;
            fh->conwriteio.io_Message.mn_ReplyPort = fh->conwritemp;

            /* Turn the console into LF+CR mode so that both
             linefeed and carriage return is done on
             */
            fh->conwriteio.io_Command = CMD_WRITE;
            fh->conwriteio.io_Data = (APTR) lf_on;
            fh->conwriteio.io_Length = 4;

            DoIO(ioReq(&fh->conwriteio));

        } /* if (0 == OpenDevice("console.device", CONU_STANDARD, ioReq(fh->conreadio), 0)) */
        else
        {
            err = ERROR_INVALID_RESIDENT_LIBRARY;
        }
        if (err)
            CloseWindow(fh->window);

    } /* if (fh->window) */
    else
    {
        D(bug("[contask] Failed to open a window\n"));
        err = ERROR_NO_FREE_STORE;
    }

    return err;
}

static BOOL MakeSureWinIsOpen(struct filehandle *fh)
{
    if (fh->window)
        return TRUE;
    return MakeConWindow(fh) == 0;
}

static void close_con(struct filehandle *fh)
{
    /* Clean up */

    D(bug("[CON] Deleting timer request 0x%p\n", fh->timerreq));
    if (fh->timerreq)
    {
        CloseDevice((struct IORequest *) fh->timerreq);
        DeleteIORequest((struct IORequest *) fh->timerreq);
    }

    D(bug("[CON] Deleting timer port 0x%p\n", fh->timermp));
    DeleteMsgPort(fh->timermp);

    if (fh->flags & FHFLG_CONSOLEDEVICEOPEN)
    {
        D(bug("[CON] Closing console.device...\n"));
        CloseDevice((struct IORequest *) fh->conreadio);
    }

    D(bug("[CON] Closing window 0x%p\n", fh->window));
    if (fh->window)
        CloseWindow(fh->window);

    D(bug("[CON] Delete console.device IORequest 0x%p\n", fh->conreadio));
    DeleteIORequest(ioReq(fh->conreadio));

    D(bug("[CON] Delete console.device MsgPort 0x%p\n", fh->conreadmp));
    FreeVec(fh->conreadmp);

    if (fh->screenname)
        FreeVec(fh->screenname);
    if (fh->wintitle)
        FreeVec(fh->wintitle);
    if (fh->pastebuffer)
        FreeMem(fh->pastebuffer, PASTEBUFSIZE);

    CloseLibrary((struct Library*) fh->intuibase);
    CloseLibrary((struct Library*) fh->dosbase);

    /* These libraries are opened only if completion was used */
    if (fh->gfxbase)
        CloseLibrary((struct Library*) fh->gfxbase);
    if (fh->gtbase)
        CloseLibrary(fh->gtbase);

    FreeVec(fh);
}

static struct filehandle *open_con(struct DosPacket *dp, LONG *perr)
{
    char *filename, *fn;
    struct filehandle *fh;
    struct DeviceNode *dn;
    LONG err, ok;
    LONG i;

    dn = BADDR(dp->dp_Arg3);
    *perr = ERROR_NO_FREE_STORE;
    fh = AllocVec(sizeof(struct filehandle), MEMF_PUBLIC | MEMF_CLEAR);
    if (!fh)
        return NULL;

    fh->intuibase = (APTR) OpenLibrary("intuition.library", 0);
    fh->dosbase = (APTR) OpenLibrary("dos.library", 0);
    fh->utilbase = (APTR) OpenLibrary("utility.library", 0);
    Forbid();
    fh->inputbase = (struct Device *) FindName(&SysBase->DeviceList, "input.device");
    Permit();

    if (!fh->intuibase || !fh->dosbase || !fh->utilbase || !fh->inputbase)
    {
        CloseLibrary((APTR) fh->utilbase);
        CloseLibrary((APTR) fh->dosbase);
        CloseLibrary((APTR) fh->intuibase);
        FreeVec(fh);
        return NULL;
    }

    fh->timermp = CreateMsgPort();
    fh->timerreq = (struct timerequest*) CreateIORequest(fh->timermp, sizeof(struct timerequest));
    OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *) fh->timerreq, 0);

    err = 0;
    filename = BSTR2C((BSTR) dp->dp_Arg1);
    fn = filename;
    i = isdosdevicec(fn);
    if (i >= 0)
        fn += i + 1;

    fh->contask = FindTask(0);

    NEWLIST(&fh->pendingReads);

    /* Create msgport for console.device communication */
    fh->conreadmp = AllocVec(sizeof(struct MsgPort) * 2, MEMF_PUBLIC | MEMF_CLEAR);
    if (fh->conreadmp)
    {

        fh->conreadmp->mp_Node.ln_Type = NT_MSGPORT;
        fh->conreadmp->mp_Flags = PA_SIGNAL;
        fh->conreadmp->mp_SigBit = AllocSignal(-1);
        fh->conreadmp->mp_SigTask = fh->contask;
        NEWLIST(&fh->conreadmp->mp_MsgList);

        fh->conwritemp = fh->conreadmp + 1;

        fh->conwritemp->mp_Node.ln_Type = NT_MSGPORT;
        fh->conwritemp->mp_Flags = PA_SIGNAL;
        fh->conwritemp->mp_SigBit = AllocSignal(-1);
        fh->conwritemp->mp_SigTask = fh->contask;
        NEWLIST(&fh->conwritemp->mp_MsgList);

        fh->conreadio = (struct IOStdReq *) CreateIORequest(fh->conreadmp, sizeof(struct IOStdReq));
        if (fh->conreadio)
        {
            D(bug("contask: conreadio created, parms '%s'\n", fn));

            fh->nw = default_nw;

            if (parse_filename(fh, fn, &fh->nw))
            {
                if (!(fh->flags & FHFLG_AUTO))
                {
                    err = MakeConWindow(fh);
                    if (!err)
                        ok = TRUE;
                }
                else
                {
                    ok = TRUE;
                }
            }
            else
                err = ERROR_BAD_STREAM_NAME;

            if (!ok)
            {
                DeleteIORequest(ioReq(fh->conreadio));
            }

        } /* if (fh->conreadio) */
        else
        {
            err = ERROR_NO_FREE_STORE;
        }

    } /* if (fh->conreadmp) */
    else
    {
        err = ERROR_NO_FREE_STORE;
    }

    if (dn->dn_Startup)
        fh->flags |= FHFLG_RAW;

    if (!ok)
        close_con(fh);

    *perr = err;
    FreeVec(filename);
    return fh;
}

static void startread(struct filehandle *fh)
{
    if (fh->flags & FHFLG_ASYNCCONSOLEREAD)
        return;
    fh->conreadio->io_Command = CMD_READ;
    fh->conreadio->io_Data = fh->consolebuffer;
    fh->conreadio->io_Length = CONSOLEBUFFER_SIZE;
    SendIO((struct IORequest*) fh->conreadio);
    fh->flags |= FHFLG_ASYNCCONSOLEREAD;
}

static void stopwait(struct filehandle *fh, struct DosPacket *waitingdp, ULONG result)
{
    if (waitingdp)
    {
        AbortIO((struct IORequest *) fh->timerreq);
        WaitIO((struct IORequest *) fh->timerreq);
        replypkt(waitingdp, result);
    }
}

static void stopread(struct filehandle *fh, struct DosPacket *waitingdp)
{
    struct Message *msg, *next_msg;

    stopwait(fh, waitingdp, DOSFALSE);

    ForeachNodeSafe(&fh->pendingReads, msg, next_msg)
    {
        struct DosPacket *dpr;

        Remove((struct Node *) msg);
        dpr = (struct DosPacket*) msg->mn_Node.ln_Name;
        replypkt(dpr, DOSFALSE);
    }
}

LONG CONMain(struct ExecBase *SysBase)
{
    struct MsgPort *mp;
    struct DosPacket *dp;
    struct Message *mn;
    struct FileHandle *dosfh;
    LONG error;
    struct filehandle *fh;
    struct FileLock *fl;
    struct DosPacket *waitingdp = NULL;

    D(bug("[CON] started\n"));
    mp = &((struct Process*) FindTask(NULL))->pr_MsgPort;
    WaitPort(mp);
    dp = (struct DosPacket*) GetMsg(mp)->mn_Node.ln_Name;
    D(bug("[CON] startup message received. port=0x%p path='%b'\n", mp, dp->dp_Arg1));

    fh = open_con(dp, &error);
    if (!fh)
    {
        D(bug("[CON] init failed\n"));
        goto end;
    }
    D(bug("[CON] 0x%p open\n", fh));
    replypkt(dp, DOSTRUE);

    for (;;)
    {
        ULONG conreadmask = 1L << fh->conreadmp->mp_SigBit;
        ULONG timermask = 1L << fh->timermp->mp_SigBit;
        ULONG packetmask = 1L << mp->mp_SigBit;
        ULONG sigs;

        sigs = Wait(packetmask | conreadmask | timermask);

        if (sigs & timermask)
        {
            if (waitingdp)
            {
                replypkt(waitingdp, DOSFALSE);
                waitingdp = NULL;
            }
        }

        if (sigs & conreadmask)
        {
            GetMsg(fh->conreadmp);
            fh->flags &= ~FHFLG_ASYNCCONSOLEREAD;
            if (waitingdp)
            {
                stopwait(fh, waitingdp, DOSTRUE);
                waitingdp = NULL;
            }
            D(bug("IO_READ %d\n", fh->conreadio->io_Actual));
            fh->conbuffersize = fh->conreadio->io_Actual;
            fh->conbufferpos = 0;
            /* terminate with 0 char */
            fh->consolebuffer[fh->conbuffersize] = '\0';
            if (fh->flags & FHFLG_RAW)
            {
                LONG inp;
                /* raw mode */
                for (inp = 0; (inp < fh->conbuffersize) && (fh->inputpos < INPUTBUFFER_SIZE);)
                {
                    fh->inputbuffer[fh->inputpos++] = fh->consolebuffer[inp++];
                }
                fh->inputsize = fh->inputstart = fh->inputpos;
                HandlePendingReads(fh);
            } /* if (fh->flags & FHFLG_RAW) */
            else
            {
                /* Cooked mode */
                if (process_input(fh))
                {
                    /*
                     * process_input() returns TRUE when EOF was received after the WAIT console
                     * has been closed by the owner.
                     */
                    dp = NULL;
                    goto end;
                }
            } /* if (fh->flags & FHFLG_RAW) else ... */
            startread(fh);
        }

        while ((mn = GetMsg(mp)))
        {
            dp = (struct DosPacket*) mn->mn_Node.ln_Name;
            dp->dp_Res2 = 0;
            D(
                    bug("[CON 0x%p] packet 0x%p:%d 0x%p,0x%p,0x%p\n", fh, dp, dp->dp_Type, dp->dp_Arg1, dp->dp_Arg2,
                            dp->dp_Arg3));
            error = 0;
            switch (dp->dp_Type)
            {
            case ACTION_FH_FROM_LOCK:
                fl = BADDR(dp->dp_Arg2);
                if (fl->fl_Task != mp || fl->fl_Key != (IPTR) fh)
                {
                    replypkt2(dp, DOSFALSE, ERROR_OBJECT_NOT_FOUND);
                    break;
                }
                fh->usecount--;
                FreeMem(fl, sizeof(*fl));
                /* Fallthrough */
            case ACTION_FINDINPUT:
            case ACTION_FINDOUTPUT:
            case ACTION_FINDUPDATE:
                dosfh = BADDR(dp->dp_Arg1);
                dosfh->fh_Interactive = DOSTRUE;
                dosfh->fh_Arg1 = (SIPTR) fh;
                fh->usecount++;
                fh->breaktask = dp->dp_Port->mp_SigTask;
                D(bug("[CON] Find fh=%x. Usecount=%d\n", dosfh, fh->usecount));
                replypkt(dp, DOSTRUE);
                break;
            case ACTION_COPY_DIR_FH:
                fl = AllocMem(sizeof(*fl), MEMF_CLEAR | MEMF_PUBLIC);
                if (fl == BNULL)
                {
                    replypkt2(dp, (SIPTR) BNULL, ERROR_NO_FREE_STORE);
                }
                else
                {
                    fh->usecount++;
                    fl->fl_Task = mp;
                    fl->fl_Key = (IPTR) fh;
                    replypkt(dp, (SIPTR) MKBADDR(fl));
                }
                break;
            case ACTION_END:
                fh->usecount--;
                D(bug("[CON] usecount=%d\n", fh->usecount));
                if (fh->usecount <= 0)
                {
                    if (fh->flags & FHFLG_WAIT)
                    {
                        D(bug("[CON] Delayed close, waiting...\n"));

                        /*
                         * Bounce all pending read and waits (the same as we do when exiting).
                         * However the process is still around, waiting for EOF input.
                         * Our user has just closed his struct FileHandle and dropped us.
                         */
                        stopread(fh, waitingdp);
                        waitingdp = NULL;
                        fh->flags = (fh->flags & ~FHFLG_READPENDING) | FHFLG_WAITFORCLOSE;
                    }
                    else
                        goto end;
                }
                replypkt(dp, DOSTRUE);
                break;
            case ACTION_READ:
                if (!MakeSureWinIsOpen(fh))
                {
                    replypkt2(dp, DOSFALSE, ERROR_NO_FREE_STORE);
                    break;
                }
                fh->breaktask = dp->dp_Port->mp_SigTask;
                startread(fh);
                con_read(fh, dp);
                break;
            case ACTION_WRITE:
                if (!MakeSureWinIsOpen(fh))
                {
                    replypkt2(dp, DOSFALSE, ERROR_NO_FREE_STORE);
                    break;
                }
                fh->breaktask = dp->dp_Port->mp_SigTask;
                startread(fh);
                answer_write_request(fh, dp);
                break;
            case ACTION_SCREEN_MODE:
            {
                D(bug("ACTION_SCREEN_MODE %s\n", dp->dp_Arg1 ? "RAW" : "CON"));
                if (dp->dp_Arg1 && !(fh->flags & FHFLG_RAW))
                {
                    /* Switching from CON: mode to RAW: mode */
                    fh->flags |= FHFLG_RAW;
                    fh->inputstart = fh->inputsize;
                    fh->inputpos = fh->inputsize;
                    HandlePendingReads(fh);
                }
                else
                {
                    /* otherwise just copy the flags */
                    if (dp->dp_Arg1)
                        fh->flags |= FHFLG_RAW;
                    else
                        fh->flags &= ~FHFLG_RAW;
                }
                replypkt(dp, DOSTRUE);
            }
                break;
            case ACTION_CHANGE_SIGNAL:
            {
                struct Task *old = fh->breaktask;
                if (dp->dp_Arg2)
                    fh->breaktask = (struct Task*) dp->dp_Arg2;
                replypkt2(dp, DOSTRUE, (SIPTR) old);
            }
                break;
            case ACTION_WAIT_CHAR:
            {
                if (!MakeSureWinIsOpen(fh))
                {
                    replypkt2(dp, DOSFALSE, ERROR_NO_FREE_STORE);
                    break;
                }
                if (fh->inputsize > 0)
                {
                    replypkt(dp, DOSTRUE);
                }
                else
                {
                    LONG timeout = dp->dp_Arg1;
                    LONG sec = timeout / 1000000;
                    LONG usec = timeout % 1000000;

                    fh->timerreq->tr_node.io_Command = TR_ADDREQUEST;
                    fh->timerreq->tr_time.tv_secs = sec;
                    fh->timerreq->tr_time.tv_micro = usec;
                    SendIO((struct IORequest *) fh->timerreq);
                    waitingdp = dp;
                }
                startread(fh);
            }
                break;
            case ACTION_IS_FILESYSTEM:
                replypkt(dp, DOSFALSE);
                break;
            case ACTION_DISK_INFO:
            {
                /* strange console handler features */
                struct InfoData *id = BADDR(dp->dp_Arg1);
                memset(id, 0, sizeof(struct InfoData));
                id->id_DiskType =
                        (fh->flags & FHFLG_RAW) ? AROS_MAKE_ID('R', 'A', 'W', 0) : AROS_MAKE_ID('C', 'O', 'N', 0);
                id->id_VolumeNode = (BPTR) fh->window;
                id->id_InUse = (IPTR) fh->conreadio;
                replypkt(dp, DOSTRUE);
            }
                break;
            case ACTION_SEEK:
                /* Yes, DOSTRUE. Check Guru Book for details. */
                replypkt2(dp, DOSTRUE, ERROR_ACTION_NOT_KNOWN);
                break;
            default:
                bug("[CON] unknown action %d\n", dp->dp_Type);
                replypkt2(dp, DOSFALSE, ERROR_ACTION_NOT_KNOWN);
                break;
            }
        }
    }
end:
    D(bug("[CON] 0x%p closing\n", fh));
    if (fh)
    {
        D(bug("[CON] Cancelling read requests...\n"));
        stopread(fh, waitingdp);

        if (fh->flags & FHFLG_ASYNCCONSOLEREAD)
        {
            D(bug("[CON] Aborting console ioReq 0x%p\n", fh->conreadio));

            AbortIO(ioReq(fh->conreadio));
            WaitIO(ioReq(fh->conreadio));
        }

        D(bug("[CON] Closing handle...\n"));
        close_con(fh);
    }

    if (dp)
    {
        D(bug("[CON] Replying packet 0x%p\n", dp));
        replypkt(dp, DOSFALSE);
    }

    D(bug("[CON] 0x%p closed\n", fh));
    return 0;
}
