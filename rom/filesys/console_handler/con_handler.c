/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define CONSOLE_SHOW_MENU

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/console.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/workbench.h>

#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/alerts.h>
#include <utility/tagitem.h>
#include <dos/bptr.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <intuition/intuition.h>
#include <workbench/startup.h>
#include <devices/conunit.h>

#include <stddef.h>
#include <string.h>

#include "con_libdefs.h"

#if defined(__AROSPLATFORM_SMP__)
#include <aros/types/spinlock_s.h>
#include <proto/execlock.h>
#include <resources/execlock.h>
#endif

#define DREAD(x)
#define DACTION(x)

#if defined(CONSOLE_SHOW_MENU)
extern const char GM_UNIQUENAME(LibName)[];
__section(".text.romtag") const char GM_UNIQUENAME(CopyDateStr)[] = "1995-2025";
__section(".text.romtag") const char GM_UNIQUENAME(AROSTeamStr)[] = "AROS Development Team";
__section(".text.romtag") const char GM_UNIQUENAME(AboutTemplateStr)[] = "%s V%ld.%ld\n%s V%ld.%ld\n\nCopyright \xa9 %s by %s";

static void MakeConsAbout(struct Window *win, struct IntuitionBase *IntuitionBase)
{
    struct {
        const char *conhnamestr;
        ULONG conhversnm;
        ULONG conhrevnm;
        const char *condnamestr;
        ULONG condversnm;
        ULONG condrevnm;
        const char *conhdatestr;
        const char *conhauthstr;
    } erArgs = {
        &GM_UNIQUENAME(LibName)[0],
        VERSION_NUMBER,
        REVISION_NUMBER,
        NULL,
        0,
        0,
        &GM_UNIQUENAME(CopyDateStr)[0],
        &GM_UNIQUENAME(AROSTeamStr)[0]
    };
    struct EasyStruct   es;
    es.es_StructSize   = sizeof(es);
    es.es_Flags        = 0;
    es.es_Title        = &GM_UNIQUENAME(LibName)[0];
    es.es_TextFormat   = &GM_UNIQUENAME(AboutTemplateStr)[0];
    es.es_GadgetFormat = "Continue";
    struct MsgPort *querymp = CreateMsgPort();
    RAWARG eraPtr = (RAWARG)&erArgs;
    if (querymp) {
        struct IOStdReq *queryio = (struct IOStdReq *) CreateIORequest(querymp, sizeof(struct IOStdReq));
        if (queryio) {
            if (0 == OpenDevice("console.device", -1, (struct IORequest *)queryio, 0)) {
                struct Library *ConsoleDevice = (struct Library *)queryio->io_Device;
                erArgs.condnamestr = ConsoleDevice->lib_Node.ln_Name;
                erArgs.condversnm = ConsoleDevice->lib_Version;
                erArgs.condrevnm = ConsoleDevice->lib_Revision;
                CloseDevice((struct IORequest *)queryio);
            }
            DeleteIORequest((struct IORequest *)queryio);
        }
        DeleteMsgPort(querymp);
    }
    if (erArgs.condnamestr == NULL) {
        es.es_TextFormat   = &GM_UNIQUENAME(AboutTemplateStr)[13];
        erArgs.condnamestr = erArgs.conhnamestr;
        erArgs.condversnm = erArgs.conhversnm;
        erArgs.condrevnm = erArgs.conhrevnm;
        eraPtr = (RAWARG)&erArgs.condnamestr;
    }

    EasyRequestArgs(win, &es, NULL, eraPtr);
}
#endif

#include <devices/serial.h>

#include "con_handler_intern.h"
#include "support.h"

/*
 * Set in the startup BPTR to ask for a device instead of a console window,
 * with the rest of the BPTR pointing at a FileSysStartupMsg naming it. A BPTR
 * is a longword address shifted right by two, so the top bits are spare.
 *
 * This is the established convention between an AUX: handler stub and the
 * console handler it hands over to, so stubs and console handlers from
 * different sources interoperate.
 */
#define AUXF_DEVICE     0x40000000

#if defined(CONSOLE_SHOW_MENU)
enum
{
    MEN_CONSOLE_ABOUT = 1,
    MEN_CONSOLE_CLOSE,
    MEN_CONSOLE_CLIP0,
    MEN_CONSOLE_CLIP1,
    MEN_CONSOLE_CLIP2,
    MEN_CONSOLE_CLIP3,
    MEN_CONSOLE_COPY,
    MEN_CONSOLE_PASTE,
};
#endif

static char *BSTR2C(BSTR srcs)
{
    int len = AROS_BSTR_strlen(srcs);
    UBYTE *src = AROS_BSTR_ADDR(srcs);
    char *dst;

    dst = AllocVec(len + 1, MEMF_ANY);
    if (!dst)
        return NULL;
    memcpy(dst, src, len);
    dst[len] = 0;

    return dst;
}
static WORD isdosdevicec(CONST_STRPTR s)
{
    UBYTE b = 0;
    while (s[b]) {
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

    D(bug("[con:handler] %s(0x%p)\n", __func__, fh));

    if (fh->otherwindow == NULL) {
        struct TagItem win_tags[] =
        {
            { WA_PubScreen,         0 },
            { WA_AutoAdjust,        TRUE },
            { WA_PubScreenName,     0 },
            { WA_PubScreenFallBack, TRUE },
            { TAG_DONE }
        };

        win_tags[2].ti_Data = (IPTR) fh->screenname;
        D(bug("[con:handler] %s: Using screen '%s', IntuitionBase = 0x%p\n", __func__, fh->screenname, IntuitionBase));

        /*  Autoadjust doesn't enforce the window's width and height to be larger than
         minwidth and minheight, so we set it here to avoid crashes in devs/console
         if a user does e.g. dir >con:0/0/0/0
         */
        fh->nw.Width = fh->nw.Width > fh->nw.MinWidth ? fh->nw.Width : -1;
        fh->nw.Height = (fh->flags & FHFLG_BOOTCON && fh->nw.Height == -1) ||
                            fh->nw.Height > fh->nw.MinHeight ? fh->nw.Height : fh->nw.MinHeight;

        fh->window = OpenWindowTagList(&fh->nw, (struct TagItem *) win_tags);
    }
    else
        fh->window = fh->otherwindow;

    if (fh->window)
    {
        D(bug("[con:handler] %s: Using window  @ 0x%p\n", __func__, fh->window));

#if defined(CONSOLE_SHOW_MENU)
        struct TagItem menu_tags[] = {
            {GTMN_NewLookMenus, TRUE},
            {TAG_DONE               }
        };
        struct NewMenu newconmenu[] = {
            { NM_TITLE, "Console",      NULL,   0                               },
             { NM_ITEM, NM_BARLABEL                                             },
             { NM_ITEM,  "About...",    "?",    0, 0, (APTR)MEN_CONSOLE_ABOUT   },
             { NM_ITEM, NM_BARLABEL                                             },
             { NM_ITEM,  "Close",       "Q",    !(fh->nw.Flags & WFLG_CLOSEGADGET) ? NM_ITEMDISABLED : 0,
                                                   0, (APTR)MEN_CONSOLE_CLOSE   },
            { NM_TITLE, "Edit",         NULL,   0                               },
             { NM_ITEM,  "Clipboard Unit", NULL, 0                              },
              { NM_SUB,  "0",           "0",    0, 0, (APTR)MEN_CONSOLE_CLIP0   },
              { NM_SUB,  "1",           "1",    0, 0, (APTR)MEN_CONSOLE_CLIP1   },
              { NM_SUB,  "2",           "2",    0, 0, (APTR)MEN_CONSOLE_CLIP2   },
              { NM_SUB,  "3",           "3",    0, 0, (APTR)MEN_CONSOLE_CLIP3   },
             { NM_ITEM, NM_BARLABEL                                             },
             { NM_ITEM,  "Copy",        "c",    0, 0, (APTR)MEN_CONSOLE_COPY    },
             { NM_ITEM,  "Paste",       "v",    0, 0, (APTR)MEN_CONSOLE_PASTE   },
            { NM_END                                                            }
        };
        if ((fh->winmenu = CreateMenusA(newconmenu, menu_tags))) {
            APTR vi = GetVisualInfoA(fh->window->WScreen, NULL);
            if (vi) {
                if (LayoutMenusA(fh->winmenu, vi, menu_tags)) {
                    SetMenuStrip(fh->window, fh->winmenu);
                    D(bug("[con:handler] %s: menu attached\n", __func__));
                }
                FreeVisualInfo(vi);
            }
        }
#endif
        fh->conreadio->io_Data = (APTR) fh->window;
        fh->conreadio->io_Length = sizeof(struct Window);

        if (0 == OpenDevice("console.device", CONU_SNIPMAP, ioReq(fh->conreadio), 0)) {
            const UBYTE lf_on[] =
            { 0x9B, 0x32, 0x30, 0x68 }; /* Set linefeed mode    */

            D(bug("[con:handler] %s: console.device open\n", __func__));

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

            if (fh->workbenchbase && (fh->appmsgport = CreateMsgPort())) {
                if ((fh->appwindow = AddAppWindow(0, 0, fh->window, fh->appmsgport, NULL)))
                    D(bug("[con:handler] %s: promoted to AppWindow\n", __func__));
            }

        } /* if (0 == OpenDevice("console.device", CONU_STANDARD, ioReq(fh->conreadio), 0)) */
        else
            err = ERROR_INVALID_RESIDENT_LIBRARY;
        if (err)
        {
            /* The pointer must not survive the close: MakeSureWinIsOpen
             * treats a non-NULL window as an open one, so a stale pointer
             * both bypasses the FHFLG_NOWINDOW latch and hands console
             * I/O a freed window.
             */
            CloseWindow(fh->window);
            fh->window = NULL;
        }

    } /* if (fh->window) */
    else {
        D(bug("[con:handler] Failed to open a window\n"));
        err = ERROR_NO_FREE_STORE;
    }

    return err;
}

/*
 * Device mode: everything above this layer only ever issues CMD_READ and
 * CMD_WRITE, so any device answering those can stand in for console.device.
 * console.device is the odd one out in taking a window through the IORequest
 * at open time; a plain device takes a unit number and flags instead.
 */
static LONG MakeConDevice(struct filehandle *fh)
{
    LONG err = 0;

    D(bug("[con:handler] %s: opening '%s' unit %d\n", __func__, fh->devname,
          fh->devunit));

    fh->conreadio->io_Data = NULL;
    fh->conreadio->io_Length = 0;

    if (0 == OpenDevice(fh->devname, fh->devunit, ioReq(fh->conreadio),
                        fh->devflags)) {
        fh->flags |= FHFLG_CONSOLEDEVICEOPEN;

        fh->conwriteio = *fh->conreadio;
        fh->conwriteio.io_Message.mn_ReplyPort = fh->conwritemp;
        /*
         * The read request is an IOExtSer in device mode, but this one is a
         * plain IOStdReq embedded in the filehandle. Copying its length over
         * would advertise storage that is not there, right in the middle of
         * the structure.
         */
        fh->conwriteio.io_Message.mn_Length = sizeof(fh->conwriteio);

        /* No linefeed-mode escape here: that is a console.device control
           sequence, and a serial line would simply print it. */
    } else {
        D(bug("[con:handler] %s: cannot open '%s'\n", __func__, fh->devname));
        err = ERROR_DEVICE_NOT_MOUNTED;
    }

    return err;
}

static BOOL MakeSureWinIsOpen(struct filehandle *fh)
{
    /* In device mode there is no window and never will be. */
    if (fh->flags & FHFLG_DEVICEMODE)
        return TRUE;
    if (fh->window)
        return TRUE;
    if (fh->flags & FHFLG_NOWINDOW)
        return FALSE;
    if (MakeConWindow(fh) == 0)
        return TRUE;
    if (fh->flags & FHFLG_BOOTCON) {
        /* The boot console could not get its window - typically the
         * screen's bitmap allocation failing on a chip-RAM-only machine
         * whose memory the booted program already owns. Latch it:
         * without this every packet re-attempts the whole screen open
         * (40 KB of chip for a Workbench screen) forever. The callers
         * turn a latched boot console into a sink: writes are
         * swallowed, reads see end-of-file.
         */
        fh->flags |= FHFLG_NOWINDOW;
    }
    return FALSE;
}

static void close_con(struct filehandle *fh)
{
    /* Clean up */

    D(bug("[con:handler] Deleting timer request 0x%p\n", fh->timerreq));
    if (fh->timerreq) {
        CloseDevice((struct IORequest *) fh->timerreq);
        DeleteIORequest((struct IORequest *) fh->timerreq);
    }

    D(bug("[con:handler] Deleting timer port 0x%p\n", fh->timermp));
    DeleteMsgPort(fh->timermp);

    if (fh->flags & FHFLG_CONSOLEDEVICEOPEN) {
        D(bug("[con:handler] Closing console.device...\n"));
        CloseDevice((struct IORequest *) fh->conreadio);
    }

    if (fh->appwindow) {
        D(bug("[con:handler] Unpromote console window from being an AppWindow\n"));
        RemoveAppWindow(fh->appwindow);
    }
    if (fh->appmsgport) {
        struct AppMessage  *appmsg;
        while ((appmsg = (struct AppMessage *) GetMsg(fh->appmsgport)))
            ReplyMsg ((struct Message *) appmsg);
        D(bug("[con:handler] Delete MsgPort for AppWindow\n"));
        DeleteMsgPort(fh->appmsgport);
    }

    D(bug("[con:handler] Closing window 0x%p\n", fh->window));
    if (fh->window)
        CloseWindow(fh->window);

    D(bug("[con:handler] Delete console.device IORequest 0x%p\n", fh->conreadio));
    DeleteIORequest(ioReq(fh->conreadio));

    D(bug("[con:handler] Delete console.device MsgPort 0x%p\n", fh->conreadmp));
    FreeVec(fh->conreadmp);

    if (fh->screenname)
        FreeVec(fh->screenname);
    if (fh->wintitle)
        FreeVec(fh->wintitle);
    if (fh->pastebuffer)
        FreeMem(fh->pastebuffer, PASTEBUFSIZE);

    CloseLibrary((struct Library*) fh->intuibase);
    CloseLibrary((struct Library*) fh->dosbase);
    CloseLibrary((struct Library*) fh->workbenchbase);

    /* These libraries are opened only if completion was used */
    if (fh->gfxbase)
        CloseLibrary((struct Library*) fh->gfxbase);
    if (fh->gtbase)
        CloseLibrary(fh->gtbase);

    FreeVec(fh);
}

static struct filehandle *open_con(struct DosPacket *dp, LONG *perr)
{
#if defined(__AROSPLATFORM_SMP__)
    void *ExecLockBase = OpenResource("execlock.resource");
#endif
    char *filename, *fn;
    struct filehandle *fh;
    struct DeviceNode *dn;
    LONG err, ok = FALSE;
    LONG i;

    dn = BADDR(dp->dp_Arg3);
    *perr = ERROR_NO_FREE_STORE;
    fh = AllocVec(sizeof(struct filehandle), MEMF_PUBLIC | MEMF_CLEAR);
    if (!fh)
        return NULL;

    fh->dn = dn;

    /*
     * The startup is either a small number selecting CON: or RAW:, or, with
     * AUXF_DEVICE set, a FileSysStartupMsg naming a device to run over. Only
     * the flag can tell those apart: a startup given as a string is a valid
     * pointer too, so no test on the value alone is enough.
     */
    if ((IPTR)dp->dp_Arg2 & AUXF_DEVICE) {
        struct FileSysStartupMsg *fssm =
            BADDR((BPTR)((IPTR)dp->dp_Arg2 & ~AUXF_DEVICE));

        if (fssm && fssm->fssm_Device != BNULL) {
            ULONG len = AROS_BSTR_strlen(fssm->fssm_Device);

            /* The name may be on the caller's stack, so take a copy. */
            if (len > 0 && len < sizeof(fh->devname)) {
                CopyMem(AROS_BSTR_ADDR(fssm->fssm_Device), fh->devname, len);
                fh->devname[len] = '\0';
                fh->devunit = fssm->fssm_Unit;
                fh->devflags = fssm->fssm_Flags;
                fh->flags |= FHFLG_DEVICEMODE;
            }
        }

        if (!(fh->flags & FHFLG_DEVICEMODE)) {
            D(bug("[con:handler] bad device startup 0x%p\n", dp->dp_Arg2));
            FreeVec(fh);
            *perr = ERROR_BAD_STREAM_NAME;
            return NULL;
        }
    }

    fh->intuibase = (APTR) TaggedOpenLibrary(TAGGEDOPEN_INTUITION);
    fh->gtbase = (APTR) TaggedOpenLibrary(TAGGEDOPEN_GADTOOLS);
    fh->dosbase = (APTR) TaggedOpenLibrary(TAGGEDOPEN_DOS);
    fh->utilbase = (APTR) TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    fh->workbenchbase = (APTR) TaggedOpenLibrary(TAGGEDOPEN_WORKBENCH);

#if defined(__AROSPLATFORM_SMP__)
    if (ExecLockBase)
        ObtainSystemLock(&SysBase->DeviceList, SPINLOCK_MODE_READ, LOCKF_FORBID);
    else
        Forbid();
#else
    Forbid();
#endif

    fh->inputbase = (struct Device *) FindName(&SysBase->DeviceList, "input.device");

#if defined(__AROSPLATFORM_SMP__)
    if (ExecLockBase)
        ReleaseSystemLock(&SysBase->DeviceList, LOCKF_FORBID);
    else
        Permit();
#else
    Permit();
#endif

    /* A device-backed console has no window and no keyboard of its own, so it
       needs neither intuition.library nor input.device. */
    if (!fh->dosbase || !fh->utilbase ||
        (!(fh->flags & FHFLG_DEVICEMODE) && (!fh->intuibase || !fh->inputbase))) {
        CloseLibrary((APTR) fh->utilbase);
        CloseLibrary((APTR) fh->dosbase);
        CloseLibrary((APTR) fh->intuibase);
        CloseLibrary((APTR) fh->workbenchbase);
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
    if (fh->conreadmp) {
        SetMem( fh->conreadmp, 0, sizeof( *fh->conreadmp ) );
        fh->conreadmp->mp_Node.ln_Type = NT_MSGPORT;
        fh->conreadmp->mp_Flags = PA_SIGNAL;
        fh->conreadmp->mp_SigBit = AllocSignal(-1);
        fh->conreadmp->mp_SigTask = fh->contask;
        NEWLIST(&fh->conreadmp->mp_MsgList);

        fh->conwritemp = fh->conreadmp + 1;

        SetMem( fh->conwritemp, 0, sizeof( *fh->conwritemp ) );
        fh->conwritemp->mp_Node.ln_Type = NT_MSGPORT;
        fh->conwritemp->mp_Flags = PA_SIGNAL;
        fh->conwritemp->mp_SigBit = AllocSignal(-1);
        fh->conwritemp->mp_SigTask = fh->contask;
        NEWLIST(&fh->conwritemp->mp_MsgList);

        /*
         * A device may expect a larger request than console.device does and
         * refuse to open with a short one: serial.device wants a whole
         * IOExtSer. The extra fields go unused here, they just have to exist.
         */
        fh->conreadio = (struct IOStdReq *) CreateIORequest(fh->conreadmp,
            (fh->flags & FHFLG_DEVICEMODE) ? sizeof(struct IOExtSer)
                                           : sizeof(struct IOStdReq));
        if (fh->conreadio) {
            D(bug("[con:handler] conreadio created, parms '%s'\n", fn));

            fh->nw = default_nw;
#if defined(CONSOLE_SHOW_MENU)
            fh->nw.IDCMPFlags |= IDCMP_MENUPICK;
#endif
            if (fh->flags & FHFLG_DEVICEMODE) {
                err = MakeConDevice(fh);
                if (!err)
                    ok = TRUE;
            }
            else if (parse_filename(fh, fn, &fh->nw)) {
                if (!(fh->flags & FHFLG_AUTO)) {
                    err = MakeConWindow(fh);
                    if (!err)
                        ok = TRUE;
                }
                else
                    ok = TRUE;
            }
            else
                err = ERROR_BAD_STREAM_NAME;

            /* No cleanup here: close_con() below frees the IORequest, and
               doing it twice corrupts the allocator. */

        } /* if (fh->conreadio) */
        else {
            err = ERROR_NO_FREE_STORE;
        }

    } /* if (fh->conreadmp) */
    else {
        err = ERROR_NO_FREE_STORE;
    }

    /* dn_Startup selects RAW: only when it really is the small CON/RAW
       selector; in device mode it is the FileSysStartupMsg pointer. AUX: is a
       cooked console, so it must not be read as RAW. */
    if (!(fh->flags & FHFLG_DEVICEMODE) && dn->dn_Startup)
        fh->flags |= FHFLG_RAW;

    if (!ok) {
        close_con(fh);
        fh = NULL;
    }

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
    /*
     * console.device completes a read as soon as it has something, so asking
     * for a bufferful costs nothing. A plain device instead waits until the
     * whole count has arrived, which for a console would mean no input at all
     * until the user typed a full buffer, so ask for one character.
     */
    fh->conreadio->io_Length = (fh->flags & FHFLG_DEVICEMODE)
                             ? 1 : CONSOLEBUFFER_SIZE;
    SendIO((struct IORequest*) fh->conreadio);
    fh->flags |= FHFLG_ASYNCCONSOLEREAD;
}

static void pump_raw_input(struct filehandle *fh)
{
    while (fh->conbufferpos < fh->conbuffersize)
    {
        LONG available = fh->conbuffersize - fh->conbufferpos;
        LONG space = INPUTBUFFER_SIZE - fh->inputsize;
        LONG copylen;

        if (space <= 0)
            break;

        copylen = available < space ? available : space;
        CopyMem(fh->consolebuffer + fh->conbufferpos,
            fh->inputbuffer + fh->inputsize, copylen);
        fh->conbufferpos += copylen;
        fh->inputsize += copylen;
        fh->inputpos = fh->inputstart = fh->inputsize;

        HandlePendingReads(fh);

        /* Without a pending application read, keep the remaining device
         * bytes in consolebuffer until space is made by ACTION_READ. */
        if (fh->inputsize != 0)
            break;
    }
}

static void stopwait(struct filehandle *fh, struct DosPacket *waitingdp, ULONG result)
{
    if (waitingdp) {
        AbortIO((struct IORequest *) fh->timerreq);
        WaitIO((struct IORequest *) fh->timerreq);
        replypkt(waitingdp, result);
    }
}

static void stopread(struct filehandle *fh, struct DosPacket *waitingdp)
{
    struct Message *msg, *next_msg;

    stopwait(fh, waitingdp, DOSFALSE);

    ForeachNodeSafe(&fh->pendingReads, msg, next_msg) {
        struct DosPacket *dpr;

        Remove((struct Node *) msg);
        dpr = (struct DosPacket*) msg->mn_Node.ln_Name;
        replypkt(dpr, DOSFALSE);
    }
}

LONG CONMain(struct ExecBase *SysBase)
{
    struct MsgPort *mp = NULL;
    struct MsgPort *procmp;
    struct DosPacket *dp;
    struct Message *mn;
    struct FileHandle *dosfh;
    LONG error;
    struct filehandle *fh;
    struct FileLock *fl;
    struct DosPacket *waitingdp = NULL;

    D(bug("[con:handler] started\n"));
    procmp = &((struct Process*) FindTask(NULL))->pr_MsgPort;
    WaitPort(procmp);
    dp = (struct DosPacket*) GetMsg(procmp)->mn_Node.ln_Name;
    D(bug("[con:handler] startup message received. port=0x%p path='%b'\n", procmp, dp->dp_Arg1));

    /* Clients get a port of their own. The process port cannot serve both,
     * because DoPkt() replies land there too: TAB completion looks up
     * directories, and a client packet arriving during one of those lookups
     * would be taken for the reply and alerted as AN_AsyncPkt.
     */
    mp = CreateMsgPort();
    if (!mp) {
        error = ERROR_NO_FREE_STORE;
        D(bug("[con:handler] no packet port\n"));
        goto end;
    }

    fh = open_con(dp, &error);
    if (!fh) {
        D(bug("[con:handler] init failed\n"));
        goto end;
    }
    D(bug("[con:handler] 0x%p open\n", fh));

    /*
     * A device-backed console is a single stream, so publish the port and let
     * everyone share this handler. CON: deliberately does not do this: leaving
     * dn_Task clear is what gets each open its own window.
     */
    if (fh->flags & FHFLG_DEVICEMODE)
        fh->dn->dn_Task = mp;

    replypkt(dp, DOSTRUE);
    {
#if defined(CONSOLE_SHOW_MENU)
        ULONG clipunit = 0;
#endif
        ULONG conreadmask = 1L << fh->conreadmp->mp_SigBit;
        ULONG timermask = 1L << fh->timermp->mp_SigBit;
        ULONG packetmask = (1L << mp->mp_SigBit) | (1L << procmp->mp_SigBit);
        ULONG winmask = fh->window ? 1L << fh->window->UserPort->mp_SigBit : 0L;
        ULONG appwindowmask = fh->appmsgport ? 1L << fh->appmsgport->mp_SigBit : 0L;
        ULONG i, insertedlen;
        ULONG sigs;
        UBYTE iconpath[INPUTBUFFER_SIZE];
        WORD currentpos, currentrest;

        for (;;) {
            i = 0;
            insertedlen = 0;
            sigs = Wait(packetmask | conreadmask | timermask | winmask | appwindowmask);

            if ((appwindowmask) && (sigs & appwindowmask)) {
                D(bug("[con:handler] %s: appwindow msg signal\n", __func__));
                while ((fh->appmsg = (struct AppMessage *)GetMsg(fh->appmsgport))) {
                    if (fh->appmsg->am_Type == AMTYPE_APPWINDOW) {
                        if (fh->appmsg->am_NumArgs >= 1) {
                            do {
                                if (fh->appmsg->am_ArgList[i].wa_Lock) {
                                    NameFromLock(fh->appmsg->am_ArgList[i].wa_Lock,
                                                 iconpath, INPUTBUFFER_SIZE - 1);
                                    AddPart(iconpath, fh->appmsg->am_ArgList[i].wa_Name,
                                            INPUTBUFFER_SIZE - 1);
                                    D(bug("[con:handler] D&D iconpath: %s\n", iconpath));

                                    if ((insertedlen = strlen(iconpath))) {
                                        /*
                                         * Get rid of trailing slashes of drawers?
                                        if ((iconpath[insertedlen - 1] == '/'))
                                            insertedlen--;
                                         */
                                        if (strchr(iconpath, ' ')
                                             && insertedlen <= (INPUTBUFFER_SIZE - 2)) {
                                            memmove(iconpath + 1, iconpath, ++insertedlen);
                                            iconpath[0] = iconpath[insertedlen++] = '"';
                                        }
                                        if (insertedlen <= (INPUTBUFFER_SIZE - 1))
                                            iconpath[insertedlen++] = ' ';

                                        currentpos = fh->inputpos;
                                        currentrest = fh->inputsize - fh->inputpos;
                                        memmove(&fh->inputbuffer[currentpos + insertedlen],
                                                &fh->inputbuffer[currentpos],
                                                currentrest);
                                        CopyMem(iconpath, &fh->inputbuffer[currentpos],
                                                insertedlen);
                                        fh->inputsize += insertedlen;
                                        fh->inputpos += insertedlen;

                                        do_write(fh, &fh->inputbuffer[currentpos],
                                                 insertedlen + currentrest);
                                        do_movecursor(fh, CUR_LEFT, currentrest);
                                    }
                                }
                            } while (++i < fh->appmsg->am_NumArgs);
                        }
                    }
                    ReplyMsg((struct Message *)fh->appmsg);
                }
                ActivateWindow(fh->window);
            }

    #if defined(CONSOLE_SHOW_MENU)
            if ((winmask) && (sigs & winmask)) {
                struct IntuiMessage *msg;
                D(bug("[con:handler] %s: window signal\n", __func__));
                while ((msg = (struct IntuiMessage *)GetMsg(fh->window->UserPort))) {
                    ULONG   msgclass = msg->Class;
                    UWORD   msgcode  = msg->Code;

                    switch (msgclass) {
                    case IDCMP_MENUPICK:
                        {
                            struct MenuItem *item;
                            UWORD           men  = msgcode;

                            D(bug("[con:handler] %s: IDCMP_MENUPICK\n", __func__));

                            /* Applications using a console window may replace
                             * the handler's menu strip and request menu picks
                             * as console.device raw events.  Do not interpret
                             * their menu codes against our stale menu strip. */
                            if (fh->window->MenuStrip != fh->winmenu)
                            {
                                struct InputEvent ie = {0};
                                struct Library *ConsoleDevice =
                                    (struct Library *)fh->conreadio->io_Device;

                                ie.ie_Class = IECLASS_MENULIST;
                                ie.ie_Code = msg->Code;
                                ie.ie_Qualifier = msg->Qualifier;
                                ie.ie_X = msg->MouseX;
                                ie.ie_Y = msg->MouseY;
                                ie.ie_TimeStamp.tv_secs = msg->Seconds;
                                ie.ie_TimeStamp.tv_micro = msg->Micros;
                                CDInputHandler(&ie, ConsoleDevice);
                                break;
                            }

                            while(men != MENUNULL)
                            {
                                if ((item = ItemAddress(fh->winmenu, men)))
                                {
                                    IPTR menu_selected = (IPTR)GTMENUITEM_USERDATA(item);
                                    switch(menu_selected)
                                    {
                                        case MEN_CONSOLE_ABOUT:
                                            {
                                                D(bug("[con:handler] %s: Menu: Show About ...\n", __func__));
                                                MakeConsAbout(fh->window, IntuitionBase);
                                            }
                                            break;
                                        case MEN_CONSOLE_CLOSE:
                                            {
                                                D(bug("[con:handler] %s: Menu: Close\n", __func__));
                                                struct Library *ConsoleDevice = (struct Library *)fh->conreadio->io_Device;
                                                struct InputEvent ie;
                                                ie.ie_NextEvent = NULL;
                                                ie.ie_Class     = IECLASS_CLOSEWINDOW;
                                                CDInputHandler(&ie, ConsoleDevice);
                                                D(bug("[con:handler] %s: Menu: Close window sent\n", __func__));
                                            }
                                            break;
                                        case MEN_CONSOLE_COPY:
                                            {
                                                D(bug("[con:handler] %s: Menu: Copy\n", __func__));
                                                struct Library *ConsoleDevice = (struct Library *)fh->conreadio->io_Device;
                                                struct InputEvent ie;
                                                ie.ie_NextEvent = NULL;
                                                ie.ie_Class     = IECLASS_RAWKEY;
                                                ie.ie_Code      = 0x33;
                                                ie.ie_Qualifier = IEQUALIFIER_RCOMMAND;
                                                CDInputHandler(&ie, ConsoleDevice);
                                                D(bug("[con:handler] %s: Menu: Copy sent\n", __func__));
                                            }
                                            break;
                                        case MEN_CONSOLE_PASTE:
                                            {
                                                D(bug("[con:handler] %s: Menu: Paste\n", __func__));
                                                do_paste(fh);
                                                process_input(fh);
                                            }
                                            break;
                                        case MEN_CONSOLE_CLIP0:
                                            clipunit = 0;
                                            break;
                                        case MEN_CONSOLE_CLIP1:
                                            clipunit = 1;
                                            break;
                                        case MEN_CONSOLE_CLIP2:
                                            clipunit = 2;
                                            break;
                                        case MEN_CONSOLE_CLIP3:
                                            clipunit = 3;
                                            break;
                                    }
                                    men = item->NextSelect;
                                } else {
                                    men = MENUNULL;
                                }
                            }
                        }
                        break;
                    default:
                        break;
                    }
                    ReplyMsg((struct Message *)msg);
                }
            }
    #endif

            if (sigs & timermask) {
                D(bug("[con:handler] %s: timer signal\n", __func__));
                if (waitingdp) {
                    replypkt(waitingdp, DOSFALSE);
                    waitingdp = NULL;
                }
            }

            if (sigs & conreadmask) {
                DREAD(bug("[con:handler] %s: console read signal\n", __func__));
                GetMsg(fh->conreadmp);
                fh->flags &= ~FHFLG_ASYNCCONSOLEREAD;
                if (waitingdp) {
                    stopwait(fh, waitingdp, DOSTRUE);
                    waitingdp = NULL;
                }
                DREAD(bug("[con:handler] %s: IO_READ %d\n", __func__, fh->conreadio->io_Actual));
                fh->conbuffersize = fh->conreadio->io_Actual;
                fh->conbufferpos = 0;
                /* terminate with 0 char */
                fh->consolebuffer[fh->conbuffersize] = '\0';
                if (fh->flags & FHFLG_RAW) {
                    /* raw mode */
                    DREAD(bug("[con:handler] %s: FHFLG_RAW\n", __func__));
                    pump_raw_input(fh);
                } /* if (fh->flags & FHFLG_RAW) */
                else {
                    DREAD(bug("[con:handler] %s: COOKED\n", __func__));
                    /* Cooked mode */
                    if (process_input(fh)) {
                        /*
                         * process_input() returns TRUE when EOF was received after the WAIT console
                         * has been closed by the owner.
                         */
                        dp = NULL;
                        goto end;
                    }
                } /* if (fh->flags & FHFLG_RAW) else ... */

                if ((fh->flags & FHFLG_CONSOLEDEVICEOPEN) &&
                    (!(fh->flags & FHFLG_RAW) ||
                        fh->conbufferpos >= fh->conbuffersize))
                    startread(fh);
            }

            /* DOS addresses a handler by the port it was started on until
             * it has a file handle to go by, so the first open still turns
             * up here. Move those over and serve everything from one place.
             */
            while ((mn = GetMsg(procmp)))
                PutMsg(mp, mn);

            while ((mn = GetMsg(mp))) {
                dp = (struct DosPacket*) mn->mn_Node.ln_Name;
                dp->dp_Res2 = 0;
                DACTION(
                        bug("[con:handler] (con @ 0x%p) packet 0x%p:%d 0x%p,0x%p,0x%p\n", fh, dp, dp->dp_Type, dp->dp_Arg1, dp->dp_Arg2,
                                dp->dp_Arg3));
                error = 0;
                switch (dp->dp_Type) {
                case ACTION_FH_FROM_LOCK:
                    DACTION(bug("[con:handler] ACTION_FH_FROM_LOCK\n"));
                    fl = BADDR(dp->dp_Arg2);
                    if (fl->fl_Task != mp || fl->fl_Key != (IPTR) fh) {
                        replypkt2(dp, DOSFALSE, ERROR_OBJECT_NOT_FOUND);
                        break;
                    }
                    fh->usecount--;
                    FreeMem(fl, sizeof(*fl));
                    /* Fallthrough */
                case ACTION_FINDINPUT:
                case ACTION_FINDOUTPUT:
                case ACTION_FINDUPDATE:
                    DACTION(bug("[con:handler] ACTION_FINDxxxx\n"));
                    dosfh = BADDR(dp->dp_Arg1);
                    dosfh->fh_Interactive = DOSTRUE;
                    dosfh->fh_Arg1 = (SIPTR) fh;
                    /* Talk to us here from now on, not on the process port. */
                    dosfh->fh_Type = mp;
                    fh->usecount++;
                    fh->breaktask = dp->dp_Port->mp_SigTask;
                    DACTION(bug("[con:handler] Find fh=%x. Usecount=%d\n", dosfh, fh->usecount));
                    replypkt(dp, DOSTRUE);
                    break;
                case ACTION_COPY_DIR_FH:
                    DACTION(bug("[con:handler] ACTION_COPY_DIR_FH\n"));
                    fl = AllocMem(sizeof(*fl), MEMF_CLEAR | MEMF_PUBLIC);
                    if (fl == BNULL) {
                        replypkt2(dp, (SIPTR) BNULL, ERROR_NO_FREE_STORE);
                    }
                    else {
                        fh->usecount++;
                        fl->fl_Task = mp;
                        fl->fl_Access = ACCESS_READ;
                        fl->fl_Key = (IPTR) fh;
                        replypkt(dp, (SIPTR) MKBADDR(fl));
                    }
                    break;
                case ACTION_FREE_LOCK:
                    DACTION(bug("[con:handler] ACTION_FREE_LOCK\n"));
                    fl = BADDR(dp->dp_Arg1);
                    fh = (struct filehandle *)fl->fl_Key;

                    FreeMem(fl, sizeof(*fl));
                    fh->usecount--;

                    replypkt(dp, DOSTRUE);
                    break;
                case ACTION_END:
                    fh->usecount--;
                    D(bug("[con:handler] ACTION_END (usecount %d)\n", fh->usecount));
                    if (fh->usecount <= 0) {
                        if (fh->flags & FHFLG_WAIT) {
                            D(bug("[con:handler] Delayed close, waiting...\n"));

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
                    DACTION(bug("[con:handler] ACTION_READ\n"));
                    if (!MakeSureWinIsOpen(fh)) {
                        if (fh->flags & FHFLG_NOWINDOW)
                            replypkt(dp, 0); /* sink: end-of-file */
                        else
                            replypkt2(dp, DOSFALSE, ERROR_NO_FREE_STORE);
                        break;
                    }
                    fh->breaktask = dp->dp_Port->mp_SigTask;
                    if (fh->flags & FHFLG_RAW) {
                        con_read(fh, dp);
                        pump_raw_input(fh);
                        if (fh->conbufferpos >= fh->conbuffersize)
                            startread(fh);
                    } else {
                        startread(fh);
                        con_read(fh, dp);
                    }
                    break;
                case ACTION_WRITE:
                    DACTION(bug("[con:handler] ACTION_WRITE\n"));
                    if (!MakeSureWinIsOpen(fh)) {
                        if (fh->flags & FHFLG_NOWINDOW)
                            replypkt(dp, dp->dp_Arg3); /* sink: swallowed */
                        else
                            replypkt2(dp, DOSFALSE, ERROR_NO_FREE_STORE);
                        break;
                    }
                    fh->breaktask = dp->dp_Port->mp_SigTask;
                    startread(fh);
                    answer_write_request(fh, dp);
                    break;
                case ACTION_SCREEN_MODE:
                    {
                        DACTION(bug("[con:handler] ACTION_SCREEN_MODE %s\n", dp->dp_Arg1 ? "RAW" : "CON"));
                        if (dp->dp_Arg1 && !(fh->flags & FHFLG_RAW)) {
                            /* Switching from CON: mode to RAW: mode */
                            fh->flags |= FHFLG_RAW;
                            fh->inputstart = fh->inputsize;
                            fh->inputpos = fh->inputsize;
                            HandlePendingReads(fh);
                        } else {
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
                        DACTION(bug("[con:handler] ACTION_CHANGE_SIGNAL\n"));
                        if (dp->dp_Arg2)
                            fh->breaktask = (struct Task*) dp->dp_Arg2;
                        replypkt2(dp, DOSTRUE, (SIPTR) old);
                    }
                    break;
                case ACTION_WAIT_CHAR:
                    {
                        DACTION(bug("[con:handler] ACTION_WAIT_CHAR\n"));
                        if (!MakeSureWinIsOpen(fh)) {
                            if (fh->flags & FHFLG_NOWINDOW)
                                replypkt(dp, DOSFALSE); /* sink: never a char */
                            else
                                replypkt2(dp, DOSFALSE, ERROR_NO_FREE_STORE);
                            break;
                        }
                        if (fh->inputsize > 0) {
                            replypkt(dp, DOSTRUE);
                        }
                        else if (dp->dp_Arg1 == 0) {
                            replypkt(dp, DOSFALSE);
                        } else {
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
                    DACTION(bug("[con:handler] ACTION_IS_FILESYSTEM\n"));
                    replypkt(dp, DOSFALSE);
                    break;
                case ACTION_DISK_INFO:
                    {
                        /* strange console handler features */
                        struct InfoData *id = BADDR(dp->dp_Arg1);
                        DACTION(bug("[con:handler] ACTION_DISK_INFO\n"));
                        SetMem(id, 0, sizeof(struct InfoData));
                        id->id_DiskType =
                                (fh->flags & FHFLG_RAW) ? AROS_MAKE_ID('R', 'A', 'W', 0) : AROS_MAKE_ID('C', 'O', 'N', 0);
                        /* The window we hang off, for whoever wants to put
                           something over it. Empty means we have none just
                           now: a console opened AUTO gets its window when
                           something first writes to it. Driving a device
                           there will never be one, which is -1 rather than
                           empty so that the two can be told apart. */
                        id->id_VolumeNode = (fh->flags & FHFLG_DEVICEMODE)
                                                ? (BPTR)(SIPTR)-1
                                                : (BPTR)fh->window;
                        /* Anyone still holding a stream on us. Reporting the
                           IORequest here made us look busy for as long as we
                           were alive, so DISMOUNT could never proceed. */
                        id->id_InUse = fh->usecount;
                        replypkt(dp, DOSTRUE);
                    }
                    break;
                case ACTION_INHIBIT:
                    /* Nothing is buffered on our side, so there is nothing to
                       stop doing. */
                    DACTION(bug("[con:handler] ACTION_INHIBIT %ld\n", (LONG)dp->dp_Arg1));
                    replypkt(dp, DOSTRUE);
                    break;
                case ACTION_DIE:
                    D(bug("[con:handler] ACTION_DIE (usecount %d)\n", fh->usecount));
                    if (fh->usecount > 0) {
                        replypkt2(dp, DOSFALSE, ERROR_OBJECT_IN_USE);
                        break;
                    }
                    replypkt(dp, DOSTRUE);
                    dp = NULL;
                    goto end;
                case ACTION_SEEK:
                    /* Yes, DOSTRUE. Check Guru Book for details. */
                    DACTION(bug("[con:handler] ACTION_SEEK\n"));
                    replypkt2(dp, DOSTRUE, ERROR_ACTION_NOT_KNOWN);
                    break;
                default:
                    /* Saying no is a normal answer here: capability probes
                     * ask for actions a console has no use for. */
                    DACTION(bug("[con:handler] unknown action %d from '%s'\n",
                            dp->dp_Type, dp->dp_Port && dp->dp_Port->mp_SigTask
                                ? dp->dp_Port->mp_SigTask->tc_Node.ln_Name
                                : "?"));
                    replypkt2(dp, DOSFALSE, ERROR_ACTION_NOT_KNOWN);
                    break;
                }
            }
        }
    }
end:
    D(bug("[con:handler] 0x%p closing\n", fh));
    if (fh) {
        /* Stop DOS handing out our port before it goes away, so that the next
           access starts a fresh handler. The node may have been dismounted
           while we ran, so only touch it while it is still in the list. */
        if (fh->flags & FHFLG_DEVICEMODE) {
            struct DosList *dl = LockDosList(LDF_DEVICES | LDF_WRITE);

            while ((dl = NextDosEntry(dl, LDF_DEVICES))) {
                if (dl == (struct DosList *)fh->dn) {
                    if (fh->dn->dn_Task == mp)
                        fh->dn->dn_Task = NULL;
                    break;
                }
            }
            UnLockDosList(LDF_DEVICES | LDF_WRITE);
        }

        D(bug("[con:handler] Cancelling read requests...\n"));
        stopread(fh, waitingdp);

        if (fh->flags & FHFLG_ASYNCCONSOLEREAD) {
            D(bug("[con:handler] Aborting console ioReq 0x%p\n", fh->conreadio));

            AbortIO(ioReq(fh->conreadio));
            WaitIO(ioReq(fh->conreadio));
        }

        D(bug("[con:handler] Closing handle...\n"));
        close_con(fh);
    }

    if (dp) {
        D(bug("[con:handler] Replying packet 0x%p, error %ld\n", dp, error));
        replypkt2(dp, DOSFALSE, error);
    }

    if (mp)
        DeleteMsgPort(mp);

    D(bug("[con:handler] 0x%p closed\n", fh));
    return 0;
}
