/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>

#include <proto/exec.h>
#include "dos_intern.h"

#ifndef EOF
#define EOF -1
#endif

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(LONG, UnGetC,

/*  SYNOPSIS */
	AROS_LHA(BPTR, file,      D1),
	AROS_LHA(LONG, character, D2),

/*  LOCATION */

	struct DosLibrary *, DOSBase, 53, Dos)

/*  FUNCTION
	Push a character back into a read filehandle. If you've read
	a character from that file you may always push at least 1 character
	back. UnGetC(file,-1) ungets the last character read. This also
	works for EOF.

    INPUTS
	file	  - Filehandle you've read from.
	character - Character to push back or EOF.

    RESULT
	!=0 if all went well, 0 if the character couldn't be pushed back.
	IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FGetC(), IoErr()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    SIPTR *result;

    ASSERT_VALID_PROCESS(me);

    result=&me->pr_Result2;

    /* Get pointer to filehandle */
    struct FileHandle *fh=(struct FileHandle *)BADDR(file);

    /* If the file is in write mode there was nothing read recently */
    if(fh->fh_Flags&FHF_WRITE)
    {
	*result=ERROR_SEEK_ERROR;
	return 0;
    }

    /* Unget EOF character if the last character read was an EOF */
    if(character==EOF&&fh->fh_End==0)
    {
	fh->fh_Pos++;
	return EOF;
    }

    /* Test if I may unget a character on this file */
    if(fh->fh_Pos==0)
    {
	*result=ERROR_SEEK_ERROR;
	return 0;
    }

    /* OK. Unget character and return. */
    fh->fh_Pos--;
    if(character!=EOF)
	((UBYTE *)BADDR(fh->fh_Buf))[fh->fh_Pos]=character;
    return character?character:1;
    AROS_LIBFUNC_EXIT
} /* UnGetC */
