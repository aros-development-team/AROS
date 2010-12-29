/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: examine.c 32949 2010-04-08 23:57:16Z neil $

    Desc: dos.library function ExamineFH().
    Lang: English
*/
#define DEBUG 0
#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, ExamineFH,

/*  SYNOPSIS */
	AROS_LHA(BPTR,                   lock, D1),
	AROS_LHA(struct FileInfoBlock *, fib,  D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 65, Dos)
/*
    FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
*/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle */
    struct FileHandle *fh = BADDR(lock);
    D(bug("[ExamineFH] fh=%x fib=%x\n", fh, fib));
    return dopacket2(DOSBase, NULL, fh->fh_Type, ACTION_EXAMINE_FH, fh->fh_Arg1, MKBADDR(fib));

    AROS_LIBFUNC_EXIT
} /* ExamineFH */
