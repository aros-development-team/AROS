/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/09/11 12:54:46  digulla
    A couple of new DOS functions from M. Fleischer

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH3(LONG, ParsePattern,

/*  SYNOPSIS */
	__AROS_LHA(STRPTR, Source,      D1),
	__AROS_LHA(STRPTR, Dest,        D2),
	__AROS_LHA(LONG,   DestLength,  D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 140, Dos)

/*  FUNCTION
	Takes a pattern containing wildcards and transforms it into some
	intermediate representation for use with the MathPattern() function.
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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)
    STRPTR stack, end;
    UBYTE a, b, t;
    LONG iswild=0;

    LONG *result=&((struct Process *)FindTask(NULL))->pr_Result2;
#define ERROR(a) { *result=a; return -1; }
    stack=end=Dest+DestLength;
#define PUT(a) { if(Dest>=stack) ERROR(ERROR_BUFFER_OVERFLOW); *Dest++=(a); }
    
    if(!*Source)
    {
        PUT(0);
        return 0;
    }else
        PUT(MP_OR);

    while(*Source)
    {
        switch(*Source++)
        {
            case '#':
                iswild=1;
                if(*Source=='?')
                {
                    Source++;
                    PUT(MP_ALL);
                }else
                {
                    PUT(MP_MULT);
                    *--stack=MP_MULT_END;
                    continue;
                }
                break;
            case '~':
                iswild=1;
                PUT(MP_NOT);
                *--stack=MP_NOT_END;
                continue;
            case '?':
                iswild=1;
                PUT(MP_SINGLE);
                break;
            case '(':
                PUT(MP_OR);
                *--stack=MP_OR_END;
                continue;
            case '|':
                iswild=1;
                if(stack!=end&&*stack!=MP_OR_END)
                    ERROR(ERROR_BAD_TEMPLATE);
                PUT(MP_OR_NEXT);
                break;
            case ')':
                if(stack==end||*stack!=MP_OR_END)
                    ERROR(ERROR_BAD_TEMPLATE);
                PUT(*stack++);
                break;
            case '[':
                if(*Source=='~')
                {
                    Source++;
                    PUT(MP_NOT_SET);
                }else
                    PUT(MP_SET);
                a=*Source++;
                if(!a)
                    ERROR(ERROR_BAD_TEMPLATE);
                do
                {
                    if(Source[0]=='-'&&Source[1]!=']')
                    {
                        Source++;
                        b=*Source++;
                        if(!b)
                            ERROR(ERROR_BAD_TEMPLATE);
                        if(b>a)
                            t=a, a=b, b=t;
                        PUT(MP_DASH);
                        if(b>=0x81&&b<=0x8e)
                        {
                            PUT(MP_ESCAPE);
                            b-=0x80;
                        }
                        PUT(b);
                    }
                    if(a>=0x81&&a<=0x8e)
                    {
                        PUT(MP_ESCAPE);
                        a-=0x80;
                    }
                    PUT(a);
                    a=*Source++;
                    if(!a)
                        ERROR(ERROR_BAD_TEMPLATE);
                }while(a!=']');
                PUT(MP_SET_END);
                break;
            case '*':
                if(DOSBase->dl_Flags&RNF_WILDSTAR)
                {
                    PUT(MP_ALL);
                }else
                    PUT('*');
                break;
            case '\'':
                if(!*Source++)
                    ERROR(ERROR_BAD_TEMPLATE);
                /* Fall through */
            default:
                a=Source[-1];
                if(a>=0x81&&a<=0x8e)
                {
                    PUT(MP_ESCAPE);
                    a-=0x80;
                }
                PUT(a);
                break;
        }
        while(stack!=end&&*stack!=MP_OR_END)
            PUT(*stack++);
    }
    if(stack!=end)
        ERROR(ERROR_BAD_TEMPLATE);
    PUT(MP_OR_END);
    PUT(0);
    return iswild;
    __AROS_FUNC_EXIT
} /* ParsePattern */
