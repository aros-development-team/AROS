/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <aros/debug.h>
#include <aros/asmcall.h>

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <dos/dosextens.h>
#include <libraries/expansion.h>

#include <proto/exec.h>
#include <proto/expansion.h>

#define _STR(A) #A
#define STR(A) _STR(A)

#define HANDLER_NAME "emul.handler"
#define VERSION 1
#define REVISION 9

#define rom_tag Emul_ROMTag

void EmulHandler_work(void);

static AROS_UFP3 (APTR, EmulHandler_Init,
		  AROS_UFPA(struct Library *, lh, D0),
		  AROS_UFPA(BPTR, segList, A0),
		  AROS_UFPA(struct ExecBase *, sysBase, A6));

static const TEXT handler_name[] = HANDLER_NAME;
static const TEXT version_string[] =
   HANDLER_NAME " " STR(VERSION) "." STR(REVISION) " (" ADATE ")\n";

const struct Resident rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&rom_tag,
   (APTR)(&rom_tag + 1),
   RTF_AFTERDOS,
   VERSION,
   NT_PROCESS,
   -1,
   (STRPTR)handler_name,
   (STRPTR)version_string,
   (APTR)EmulHandler_Init
};

static AROS_UFH3 (APTR, EmulHandler_Init,
		  AROS_UFHA(struct Library *, lh, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct Library *ExpansionBase;
    struct DeviceNode *dlv = NULL;

    ExpansionBase = OpenLibrary("expansion.library",0);
    if (ExpansionBase) {
        IPTR pp[4 + sizeof(struct DosEnvec)/sizeof(IPTR)] = {};

        D(bug("[Emulhandler] startup: got ExpansionBase\n"));

        pp[0] = (IPTR)"System";
        pp[1] = (IPTR)NULL;
        pp[2] = 0;
        pp[DE_TABLESIZE + 4]  = DE_BOOTBLOCKS;
        pp[DE_SIZEBLOCK + 4]  = 128;
        /* .... */
        pp[DE_BUFMEMTYPE + 4] = MEMF_PUBLIC;
        pp[DE_MASK + 4]       = 0x7FFFFFFE;
        pp[DE_BOOTPRI + 4]    = 0;
        pp[DE_DOSTYPE + 4]    = AROS_MAKE_ID('E','M','U','L');

        dlv = MakeDosNode(pp);

        if (dlv) {
            dlv->dn_SegList = CreateSegList(EmulHandler_work);
            D(bug("[Emulhandler] startup allocated dlv %p, handler %p\n", dlv, dlv->dn_SegList));
            AddBootNode(dlv->dn_Priority, ADNF_STARTPROC, dlv, NULL);
        }
    }

    CloseLibrary(ExpansionBase);

    return dlv ? lh : NULL;

    AROS_USERFUNC_EXIT
}
