/*
    Copyright © 2009-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: arosc library - support code for entering and leaving a program
    Lang: english
*/
#include "__arosc_privdata.h"
#include "__exitfunc.h"

#define DEBUG 0
#include <aros/debug.h>

#include <dos/stdio.h>
#include <proto/dos.h>

void __arosc_program_startup(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);

    D(bug("[__arosc_program_startup] aroscbase 0x%p\n", __GM_GetBase()));

    /* Function is just a placeholder for the future */

    /* A some C error IO routines evidently rely on this, and
     * should be fixed!
     */
    if (me->pr_Task.tc_Node.ln_Type == NT_PROCESS &&
        me->pr_CES != BNULL)
    {
        SetVBuf(me->pr_CES, NULL, BUF_NONE, -1);
    }
}

void __arosc_program_end(void)
{
    struct aroscbase *aroscbase = __GM_GetBase();
    D(bug("[__arosc_program_end]\n"));

    if (!(aroscbase->acb_flags & ABNORMAL_EXIT))
        __callexitfuncs();
}
