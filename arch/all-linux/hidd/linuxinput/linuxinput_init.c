/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: LinuxInput hidd initialization code.
    Lang: English.
*/

#define DEBUG 0

#define __OOP_NOATTRBASES__

#include <exec/rawfmt.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <utility/utility.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/mouse.h>
#include <hidd/keyboard.h>
#include <hidd/unixio.h>

#include LC_LIBDEFS_FILE

#include "linuxinput_intern.h"

#include <linux/input.h>

#define O_RDONLY    0

/* 
 * Some attrbases needed as global vars.
 * These are write-once read-many.
 */
OOP_AttrBase HiddKbdAB;
OOP_AttrBase HiddMouseAB;

static const struct OOP_ABDescr abd[] =
{
    { IID_Hidd_Kbd      , &HiddKbdAB    },
    { IID_Hidd_Mouse    , &HiddMouseAB  },
    { NULL              , NULL          }
};

VOID Update_EventHandlers(struct LinuxInput_staticdata *lsd)
{
    struct EventHandler * eh;
    ForeachNode(&lsd->eventhandlers, eh)
    {
        eh->mousehidd   = lsd->mousehidd;
        eh->kbdhidd     = lsd->kbdhidd;
        eh->unixio      = lsd->unixio;
    }
}

static inline void _sprintf(UBYTE *buffer, UBYTE *format, ...)
{
    va_list args;

    va_start(args, format);
    VNewRawDoFmt(format, RAWFMTFUNC_STRING, buffer, args);
    va_end(args);
}

static VOID Enumerate_InputDevices(struct LinuxInput_staticdata *lsd)
{
    int fd, ioerr, read;
    LONG i = 0;
    TEXT eventdev[64];


    while(TRUE)
    {
        LONG param = 0;
        struct EventHandler * eh = NULL;

        _sprintf(eventdev, "/dev/input/event%d", i);

        D(bug("[LinuxInput] Testing %s\n", eventdev));

        fd = Hidd_UnixIO_OpenFile(lsd->unixio, eventdev, O_RDONLY, 0, &ioerr);
        if (fd< 0)
            break;

        read = Hidd_UnixIO_IOControlFile(lsd->unixio, fd, EVIOCGBIT(0, EV_MAX), &param,&ioerr);

        if (read < 0)
            continue;

        eh = AllocVec(sizeof(struct EventHandler), MEMF_PUBLIC | MEMF_CLEAR);
        eh->eventdev = fd;
        eh->capabilities = CAP_NONE;

        if ((param & (1 << EV_REP)) && (param& (1 << EV_KEY)))
        {
            /* Mouse */
            eh->capabilities |= CAP_KEYBOARD;
            D(bug("[LinuxInput] Found Keyboard\n"));
        }

        if ((param & (1 << EV_REL)) && (param& (1 << EV_KEY)))
        {
            /* Mouse */
            eh->capabilities |= CAP_MOUSE;
            D(bug("[LinuxInput] Found Mouse\n"));
        }


        if (eh->capabilities != CAP_NONE)
            ADDHEAD(&lsd->eventhandlers, eh);
        else
            FreeVec(eh);

        i++;
    }
}

VOID Init_LinuxInput_inputtask(struct EventHandler * eh);
VOID Kill_LinuxInput_inputtask(struct EventHandler * eh);

static int Init_Hidd(LIBBASETYPEPTR LIBBASE)
{
    struct EventHandler * eh;

    InitSemaphore(&LIBBASE->lsd.sema);
    NEWLIST(&LIBBASE->lsd.eventhandlers);

    /*
     * We cannot cooperate with any other driver at the moment.
     * Attempting to do so results in duplicating events (one from
     * X11 and another from us).
     */
    if (OOP_FindClass("hidd.gfx.x11"))
        return FALSE;

    D(bug("[LinuxInput] Opening UnixIO... "));
    LIBBASE->lsd.unixio = OOP_NewObject(NULL, CLID_Hidd_UnixIO, NULL);
    if (!LIBBASE->lsd.unixio)
    {
        D(bug("Failed\n"));
        return FALSE;
    }
    D(bug("OK\n"));

    Enumerate_InputDevices(&LIBBASE->lsd);
    if (IsListEmpty(&LIBBASE->lsd.eventhandlers))
        return FALSE;

    if (!OOP_ObtainAttrBases(abd))
        return FALSE;

    Update_EventHandlers(&LIBBASE->lsd);

    ForeachNode(&LIBBASE->lsd.eventhandlers, eh)
    {
        Init_LinuxInput_inputtask(eh);
    }

    D(bug("[LinuxInput] Finished Init_Hidd"));

    return TRUE;
}

static int Expunge_Hidd(LIBBASETYPEPTR LIBBASE)
{
    int ioerr;
    struct EventHandler * eh;

    ForeachNode(&LIBBASE->lsd.eventhandlers, eh)
    {
        Kill_LinuxInput_inputtask(eh);
        Hidd_UnixIO_CloseFile(LIBBASE->lsd.unixio, eh->eventdev, &ioerr);
    }

    OOP_ReleaseAttrBases(abd);

    if (LIBBASE->lsd.unixio)
        OOP_DisposeObject(LIBBASE->lsd.unixio);

    return TRUE;
}

ADD2INITLIB(Init_Hidd, 1)
ADD2EXPUNGELIB(Expunge_Hidd, 1)
