/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: assert() function for the kernel.
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <assert.h>
#include <aros/arossupportbase.h>

extern struct ExecBase *SysBase;

void __assert (const char * expr, const char * file, unsigned int line)
{
    /* TODO: Make this call the kernel.resource KernelAssertFail() fn */

    /* Awkward, but I need a global SysBase variable */
    struct AROSSupportBase *AROSSupportBase;

    AROSSupportBase = (struct AROSSupportBase *)SysBase->DebugData;

    /* Basically, this is the body of an KASSERT() */
    AROSSupportBase->kprintf("\x07%s::%ld: assertion failed: %s\n",
		file, line, expr);

    Alert(AG_BadParm);
} /* assert */
