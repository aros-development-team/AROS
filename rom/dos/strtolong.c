/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:23  digulla
    Initial revision

    Desc:
    Lang: english
*/

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH2I(LONG, StrToLong,

/*  SYNOPSIS */
	__AROS_LA(STRPTR, string, D1),
	__AROS_LA(LONG *, value,  D2),

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
