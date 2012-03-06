/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <aros/debug.h>

#include <exec/errors.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>

#include LC_LIBDEFS_FILE

#include "printtotool_intern.h"

static BPTR OpenTool(struct PrintToToolBase *PrintToToolBase, CONST_STRPTR tool)
{
    BPTR pipe_r, pipe_w;

    if ((pipe_w = Open("PIPE:*", MODE_NEWFILE))) {
        TEXT buff[64];
        if (NameFromFH(pipe_w, buff, sizeof buff)) {
            if ((pipe_r = Open(buff, MODE_OLDFILE))) {
                BPTR con = Open("CON://///AUTO/CLOSE/WAIT", MODE_NEWFILE);
                if (SystemTags(tool, SYS_Input, pipe_r,
                                     SYS_Output, con,
                                     SYS_Asynch, TRUE,
                                     TAG_END) >= 0) {
                    return pipe_w;
                }
                if (con != BNULL)
                    Close(con);
                Close(pipe_r);
            } else {
                D(bug("%s: Can't open read side of %s\n", __func__, buff));
            }
        } else {
            D(bug("%s: Can't get name of autopipe\n", __func__));
        }
        Close(pipe_w);
    } else {
        D(bug("%s: Can't open autopipe\n", __func__));
    }

    return BNULL;
}

BOOL pu_OpenFile(struct PrintToToolBase *PrintToToolBase, 
                 struct PrintToToolUnit *pu)
{
    struct FileRequester *fr;
    BOOL ok;

    ObtainSemaphore(&pu->pu_Lock);
    if (!pu->pu_FileReq) {
        fr = AllocAslRequestTags(ASL_FileRequest,
                ASLFR_TitleText,"Tool to print to...",
                ASLFR_InitialDrawer,"C:",
                ASLFR_RejectIcons, TRUE,
                TAG_END);
        if (fr == NULL) {
            D(bug("%s: Can't create requestor\n"));
            ReleaseSemaphore(&pu->pu_Lock);
            return IOERR_OPENFAIL;
        }
        pu->pu_FileReq = fr;
    } else {
        fr = pu->pu_FileReq;
    }

    D(bug("fr = %p\n", fr));
    ok = AslRequestTags(fr, TAG_END);
    D(bug("ok = %d (%s %s)\n", ok, fr-fr_Drawer, fr->fr_File));

    if (ok) {
        BPTR here;
      
        if (fr->fr_Drawer) {
            BPTR lock = Lock(fr->fr_Drawer, SHARED_LOCK);
            if (!lock)
                return IOERR_OPENFAIL;
            here = CurrentDir(lock);
        } else {
            here = CurrentDir(BNULL);
            CurrentDir(here);
        }

        if (fr->fr_File) {
            pu->pu_File = OpenTool(PrintToToolBase, fr->fr_File);
        } else {
            pu->pu_File = BNULL;
        }
        D(bug("opened = %d\n", opened));

        CurrentDir(here);
    }

    if (pu->pu_File == BNULL) {
        ReleaseSemaphore(&pu->pu_Lock);
        return IOERR_OPENFAIL;
    }

    return 0;
}
 
AROS_LH1(void, BeginIO,
 AROS_LHA(struct IOStdReq *, io, A1),
          struct PrintToToolBase *, PrintToToolBase, 5, PrintToTool)
{
    AROS_LIBFUNC_INIT

    struct PrintToToolUnit *pu = (struct PrintToToolUnit *)io->io_Unit;
    LONG err;

    D(bug("BeginIO: io_Command = %d\n", io->io_Command));
    D(bug("pu_File=%p\n", BADDR(pu->pu_File)));
    if (pu->pu_File == BNULL) {
        err = pu_OpenFile(PrintToToolBase, pu);
        if (err != 0) {
            D(bug("err = %d\n", err));
            io->io_Error = err;
            goto end;
        }
    }

    switch (io->io_Command) {
    case CMD_FLUSH:
    case CMD_UPDATE:
        io->io_Error = 0;
        break;
    case CMD_WRITE:
        err = Write(pu->pu_File, io->io_Data, io->io_Length);
        if (err < 0) {
            io->io_Error = IoErr();
            io->io_Actual = 0;
        } else {
            io->io_Error = 0;
            io->io_Actual = err;
        }
        break;
    default:
        io->io_Error = IOERR_NOCMD;
        break;
    }

end:
    if (!(io->io_Flags & IOF_QUICK))
        ReplyMsg(&io->io_Message);
    return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
 AROS_LHA(struct IORequest *, io, A1), 
          struct PrintToToolBase *, PrintToToolBase, 6, PrintToTool)
{
    AROS_LIBFUNC_INIT
    return IOERR_NOCMD;
    AROS_LIBFUNC_EXIT
}

static int GM_UNIQUENAME(Init)(struct PrintToToolBase *PrintToToolBase)
{
    int i;

    for (i = 0; i < PRINTTOTOOL_UNITS; i++) {
        InitSemaphore(&PrintToToolBase->pt_Unit[i].pu_Lock);
    }

    if ((PrintToToolBase->pt_DOSBase = OpenLibrary("dos.library", 35))) {
        if ((PrintToToolBase->pt_AslBase = OpenLibrary("asl.library", 36))) {
            D(bug("initted\n"));
            return TRUE;
        }
        CloseLibrary(PrintToToolBase->pt_DOSBase);
    }

    return FALSE;
}

static int GM_UNIQUENAME(Expunge)(struct PrintToToolBase *PrintToToolBase)
{
    int i;

    for (i = 0; i < PRINTTOTOOL_UNITS; i++) {
        FreeFileRequest(PrintToToolBase->pt_Unit[i].pu_FileReq);
    }

    CloseLibrary(PrintToToolBase->pt_AslBase);
    CloseLibrary(PrintToToolBase->pt_DOSBase);
    D(bug("expunged\n"));

    return TRUE;
}

static int GM_UNIQUENAME(Open)(struct PrintToToolBase *PrintToToolBase, struct IOStdReq* io, ULONG unitnum, ULONG flags)
{
    D(bug("open unit %d\n", unitnum));
    if (unitnum < 0 || unitnum >= PRINTTOTOOL_UNITS)
        return FALSE;

    io->io_Unit = (struct Unit *)&PrintToToolBase->pt_Unit[unitnum];
    D(bug("io unit %p\n", io->io_Unit));
    return TRUE;
}

static int GM_UNIQUENAME(Close)(struct PrintToToolBase *PrintToToolBase, struct IOStdReq *io)
{
    struct PrintToToolUnit *pu = (struct PrintToToolUnit *)io->io_Unit;

    D(bug("close: %p , %p\n", pu, BADDR(pu->pu_File)));
    io->io_Unit = NULL;
    if (pu->pu_File == BNULL)
        return TRUE;

    Close(pu->pu_File);
    pu->pu_File = BNULL;
    ReleaseSemaphore(&pu->pu_Lock);

    return TRUE;
}
 
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
