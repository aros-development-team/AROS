/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, ParsePattern,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, Source,     D1),
	AROS_LHA(STRPTR, Dest,       D2),
	AROS_LHA(LONG,   DestLength, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 140, Dos)

/*  FUNCTION
	Takes a pattern containing wildcards and transforms it into some
	intermediate representation for use with the MatchPattern() function.
	The intermediate representation is longer but generally a buffer
	size of 2*(strlen(Source)+1) is enough. Nevertheless you should check
	the returncode to be sure that everything went fine.

    INPUTS
	Source     - Pattern describing the kind of strings that match.
		     Possible tokens are:
		     #x     - The following character or item is repeaded 0 or
		              more times.
		     ?      - Item matching a single non-NUL character.
		     a|b|c  - Matches one of multiple strings.
		     ~x     - This item matches if the item x doesn't match.
		     (a)    - Parens
		     [a-z]  - Matches a single character out of the set.
		     [~a-z] - Matches a single non-NUL character not in the set.
		     'c     - Escapes the following character.
		     *      - Same as #?, but optional.
	Dest       - Buffer for the destination.
	DestLength - Size of the buffer.

    RESULT
	 1 - There are wildcards in the pattern (it might match more than
	     one string).
	 0 - No wildcards in it, all went fine.
	-1 - An error happened. IoErr() gives additional information in
	     that case.

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    STRPTR stack, end;
    UBYTE a;
    LONG iswild=0;

    LONG *result=&((struct Process *)FindTask(NULL))->pr_Result2;
#undef ERROR
#define ERROR(a) { *result=a; return -1; }
    stack=end=Dest+DestLength;
#define PUT(a) { if(Dest>=stack) ERROR(ERROR_BUFFER_OVERFLOW); *Dest++=(a); }
    
    if(!*Source)
    {
        PUT(0);
        return 0;
    }

    while(*Source)
    {
        switch(*Source++)
        {
            case '#':
                iswild=1;
		switch(*Source)
		{
		    case '?':
			Source++;
			PUT(P_ANY);
			break;
		    case ')':
		    case '\0':
			ERROR(ERROR_BAD_TEMPLATE);
			break;
		    default:
			PUT(P_REPBEG);
			*--stack=P_REPEND;
			continue;
                }
                break;
            case '~':
		switch(*Source)
		{
		    case '\0':
			a=Source[-1];
			PUT(a);
			break;
		    case ')':
			ERROR(ERROR_BAD_TEMPLATE);
			break;
		    default:
			iswild=1;
			PUT(P_NOT);
			*--stack=P_NOTEND;
			continue;
		}
		break;
            case '?':
                iswild=1;
                PUT(P_SINGLE);
                continue;
            case '(':
                PUT(P_ORSTART);
                *--stack=P_OREND;
                continue;
            case '|':
                iswild=1;
		if(stack==end)
		    ERROR(ERROR_BAD_TEMPLATE);
		while(!(*stack==P_OREND||stack==end))
		   PUT(*stack++);
                PUT(P_ORNEXT);
                continue;
            case ')':
                while(!(stack==end||*stack==P_OREND))
		    PUT(*stack++);
		if(stack==end)
		    ERROR(ERROR_BAD_TEMPLATE)
		else
		    PUT(*stack++);
                break;
            case '[':
		iswild=1;
                if(*Source=='~')
		{
		    Source++;
                    PUT(P_NOTCLASS);
                }else
                    PUT(P_CLASS);
		a=*Source++;
		if(!a)
		    ERROR(ERROR_BAD_TEMPLATE);
		do
		{
		    if(a=='\'')
			a=*Source++;
		    PUT(a);
		    a=*Source++;
		    if(!a)
			ERROR(ERROR_BAD_TEMPLATE);
		}while(a!=']');
		PUT(P_CLASS);
                break;
            case '*':
                if(DOSBase->dl_Flags&RNF_WILDSTAR)
                {
		    iswild=1;
                    PUT(P_ANY);
                }else
                    PUT('*');
                break;
	    case '%':
		continue;
            case '\'':
		switch(*Source)
		{
		    case '*':
		    case '?':
		    case '(':
		    case '|':
		    case ')':
		    case '~':
		    case '[':
		    case ']':
		    case '%':
		    case '\'':
			Source++;
		    default:
			break;
		}
                /* Fall through */
            default:
                a=Source[-1];
                PUT(a);
                break;
        }
	while(stack!=end&&*stack!=P_OREND)
	    PUT(*stack++);
    }
    if(stack!=end)
	ERROR(ERROR_BAD_TEMPLATE);
    PUT(0);
    return iswild;
    AROS_LIBFUNC_EXIT
} /* ParsePattern */
