/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/13 13:52:52  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced __AROS_LA by __AROS_LHA

    Revision 1.2  1996/08/01 17:40:58  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include "dos_intern.h"

#ifndef EOF
#define EOF -1
#endif

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH2(LONG, UnGetC,

/*  SYNOPSIS */
	__AROS_LHA(BPTR, file,      D1),
	__AROS_LHA(LONG, character, D2),

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

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

    LONG *result=&((struct Process *)FindTask(NULL))->pr_Result2;

    /* Get pointer to filehandle */
    struct FileHandle *fh=(struct FileHandle *)BADDR(file);

    /* If the file is in write mode there was nothing read recently */
    if(fh->fh_Flags&FHF_WRITE)
    {
	*result=ERROR_SEEK_ERROR;
	return 0;
    }

    /* Unget EOF character if the last character read was an EOF */
    if(character==EOF&&fh->fh_End==fh->fh_Buf)
    {
	fh->fh_Pos++;
	return EOF;
    }

    /* Test if I may unget a character on this file */
    if(fh->fh_Pos==fh->fh_Buf)
    {
	*result=ERROR_SEEK_ERROR;
	return 0;
    }

    /* OK. Unget character and return. */
    fh->fh_Pos--;
    if(character!=EOF)
	*fh->fh_Pos=character;
    return character?character:1;
    __AROS_FUNC_EXIT
} /* UnGetC */
