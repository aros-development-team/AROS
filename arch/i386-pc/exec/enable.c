/*
    Copyright (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: i386unix version of Enable()
    Lang: english
*/

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#define get_cs() \
	({  short __value; \
	__asm__ __volatile__ ("mov %%ds,%%ax":"=a"(__value)); \
	(__value & 0x03);	})

AROS_LH0(void, Enable,
    struct ExecBase *, SysBase, 21, Exec)
{
    AROS_LIBFUNC_INIT

    if(--SysBase->IDNestCnt < 0)
    {
		/* We can do sti only in user mode!! */
		if (get_cs())
		{
        	__asm__ __volatile__ ("sti");
		}
    }

    AROS_LIBFUNC_EXIT
}
