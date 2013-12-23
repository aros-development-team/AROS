/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/config.h>
#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <inttypes.h>

#include "debug_intern.h"

/*
 * A horrible hack. It works only under UNIX.
 * TODO: rewrite all this crap and provide C subroutines with well
 * defined API to call them from gdb for debug info lookup. This would
 * remove a requirement to rewrite _gdbinit every time when something
 * changes in this library.
 */
#ifndef HOST_OS_unix
#undef AROS_MODULES_DEBUG
#endif

#if AROS_MODULES_DEBUG
#include "../../arch/all-unix/kernel/hostinterface.h"
#endif

static int Debug_Init(struct DebugBase *DebugBase)
{
    struct TagItem *bootMsg;
    struct ELF_ModuleInfo *kmod;
#if AROS_MODULES_DEBUG
    struct HostInterface *HostIFace;
#endif

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
            return FALSE;

    NEWLIST(&DebugBase->db_Modules);
    InitSemaphore(&DebugBase->db_ModSem);

    bootMsg = KrnGetBootInfo();
    kmod = (struct ELF_ModuleInfo *)LibGetTagData(KRN_DebugInfo, 0, bootMsg);


    for (; kmod; kmod = kmod->Next)
    {
        RegisterModule_ELF(kmod->Name, BNULL, kmod->eh, kmod->sh, (struct Library *)DebugBase);
    }

#if AROS_MODULES_DEBUG
    HostIFace = (APTR)LibGetTagData(KRN_HostInterface, 0, bootMsg);
    /*
     * Provide a pointer to our modules list to the bootstrap.
     * This is needed because gdb is actually debugging bootstrap
     * and it can read debug information only from there
     */
    if (HostIFace && HostIFace->ModListPtr)
        *HostIFace->ModListPtr = &DebugBase->db_Modules;
#endif

    D(bug("[Debug] Debug_Init() done\n"));
    return 1;
}

ADD2INITLIB(Debug_Init, 0)
