/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Implementation of printf()
    Lang: english
*/

#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <aros/asmcall.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include <stdio.h>

/* All I need is a global variable SysBase */
extern struct ExecBase *SysBase;

struct Data
{
    int count;
    BPTR file;
};

AROS_UFH2(static void, _putc,
    AROS_UFHA(UBYTE,         chr, D0),
    AROS_UFHA(struct Data *, hd,  A3)
)
{
    AROS_LIBFUNC_INIT

    if (hd->count >= 0)
    {
	if (FPutC (hd->file, chr) == EOF)
	    hd->count = EOF;
	else
	    hd->count ++;
    }
    AROS_LIBFUNC_EXIT
}

int printf(const char * format, ...)
{
    struct Data hd;

    hd.count = 0;
    hd.file=((struct Process *)FindTask(NULL))->pr_COS;

    RawDoFmt((char *)format,&format+1,(VOID_FUNC)_putc,&hd);

    return hd.count;
}


