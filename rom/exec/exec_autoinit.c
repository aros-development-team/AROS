/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <aros/symbolsets.h>
#include <aros/asmcall.h>
#include <aros/autoinit.h>

#include <exec/libraries.h>

#include <proto/exec.h>
#include <proto/dos.h>

/* Linklib to provide a 'SysBase' symbol. Also verifies that
 * the symbol is set appropriately.
 */
struct ExecBase *SysBase;
extern const LONG const __aros_libreq_SysBase __attribute__((weak));

static int SysBase_autoinit(struct ExecBase *sysBase)
{
    if (sysBase == NULL)
        return FALSE;

    SysBase = sysBase;

    if (__aros_libreq_SysBase > SysBase->LibNode.lib_Version) {
        IPTR arr[] = {
                 (IPTR)(FindTask(NULL)->tc_Node.ln_Name),
                 __aros_libreq_SysBase,
                 SysBase->LibNode.lib_Version
        };
        __showerror("%s: Requires exec.library v%ld, found v%ld\n",arr);
        return FALSE;
    }

    return TRUE;
}

ADD2INIT(SysBase_autoinit,-128)
