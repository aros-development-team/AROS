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

/* Linklib to provide a 'SysBase' symbol. Also verifies that
 * the symbol is set appropriately.
 */
struct ExecBase *SysBase;
extern const LONG const __aros_libreq_SysBase __attribute__((weak));

static AROS_UFH2(IPTR, RawPutc,
        AROS_UFHA(UBYTE, c, D0),
        AROS_UFHA(struct ExecBase *, SysBase, A3))
{
    AROS_USERFUNC_INIT
    if (c)
        RawPutChar(c);
    return c;
    AROS_USERFUNC_EXIT
}

static int SysBase_check_init(void)
{
    if (SysBase == NULL) {
        IPTR arr[] = {
                 (IPTR)(FindTask(NULL)->tc_Node.ln_Name),
                 __aros_libreq_SysBase,
        };
        RawDoFmt("%s: Requires exec.library v%ld, none provided\n",arr,(VOID_FUNC)RawPutc,SysBase);
        return FALSE;
    }
    if (__aros_libreq_SysBase > SysBase->LibNode.lib_Version) {
        IPTR arr[] = {
                 (IPTR)(FindTask(NULL)->tc_Node.ln_Name),
                 __aros_libreq_SysBase,
                 SysBase->LibNode.lib_Version
        };
        RawDoFmt("%s: Requires exec.library v%ld, found v%ld\n",arr,(VOID_FUNC)RawPutc,SysBase);
        return FALSE;
    }

    return TRUE;
}

ADD2INIT(SysBase_check_init,127)
