/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Implementation of printf()
    Lang: english
*/

#include <exec/execbase.h>
#include <dos/dosextens.h>
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

__RA2(static void,_putc,UBYTE,chr,D0,struct Data *,hd,A3)
{
    __AROS_FUNC_INIT

    if (hd->count >= 0)
    {
	if (FPutC (hd->file, chr) == EOF)
	    hd->count = EOF;
	else
	    hd->count ++;
    }
    __AROS_FUNC_EXIT
}

int printf(const char * format, ...)
{
    struct Data hd;

    hd.count = 0;
    hd.file=((struct Process *)FindTask(NULL))->pr_COS;

    RawDoFmt((char *)format,&format+1,(VOID_FUNC)_putc,&hd);

    return hd.count;
}


