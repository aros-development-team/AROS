/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <aros/symbolsets.h>
#include <aros/asmcall.h>

#include <exec/libraries.h>

#include <proto/exec.h>
#include <proto/dos.h>

/* From libautoinit.a */
void __showerror(char *format, const IPTR *);

/* Linklib to provide a 'SysBase' symbol. Also verifies that
 * the symbol is set appropriately.
 */
struct ExecBase *SysBase;
extern const LONG const __aros_libreq_SysBase __attribute__((weak));

static int SysBase_check_init(void)
{
    if (SysBase == NULL)
        return FALSE;

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

ADD2INIT(SysBase_check_init,127)
