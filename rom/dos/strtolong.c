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

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH2I(LONG, StrToLong,

/*  SYNOPSIS */
	__AROS_LHA(STRPTR, string, D1),
	__AROS_LHA(LONG *, value,  D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 136, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

    LONG sign=0, v=0;
    STRPTR s=string;
    
    /* Skip leading whitespace characters */
    if(*s==' '||*s=='\t')
        s++;

    /* Swallow sign */
    if(*s=='+'||*s=='-')
        sign=*s++;
    
    /* If there is no number return an error. */
    if(*s<'0'||*s>'9')
    {
        *value=0;
        return -1;
    }

    /* Calculate result */    
    do
        v=v*10+*s++-'0';
    while(*s>='0'&&*s<='9');

    /* Negative? */
    if(sign=='-')
        v=-v;

    /* All done. */    
    *value=v;
    return s-string;
    __AROS_FUNC_EXIT
} /* StrToLong */
