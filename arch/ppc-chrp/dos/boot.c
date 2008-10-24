/*
    Copyright  1995-2001, The AROS Development Team. All rights reserved.
    $Id: boot.c 28503 2008-04-28 09:32:13Z schulz $

    Desc: Boot your operating system.
    Lang: english
*/

#include <exec/types.h>
#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <dos/dosextens.h>
#include <utility/tagitem.h>
#include <aros/arossupportbase.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

/* Require this for the stdout defn */
#include <stdio.h>

extern int submain(struct ExecBase *, struct DosLibrary *);

void boot(struct ExecBase *SysBase, BOOL hidds_ok)
{
    struct DosLibrary *DOSBase;

    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    if( DOSBase == NULL )
    {
        /* BootStrap couldn't open dos.library */
        Alert(AT_DeadEnd | AN_BootStrap | AG_OpenLib | AO_DOSLib );
    }

    submain(SysBase, DOSBase);

    /* No RemTask() here, otherwise the process cleanup routines
       are not called. And that would for example mean, that the
       Boot Process (having a CLI) is not removed from the rootnode.
       --> Dead stuff in there -> Crash
    */
    return;
}
