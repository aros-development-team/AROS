/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/12/09 13:53:37  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.2  1996/10/24 15:50:34  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/11 12:54:46  digulla
    A couple of new DOS functions from M. Fleischer

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <clib/utility_protos.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <clib/dos_protos.h>

	AROS_LH3(LONG, ParsePatternNoCase,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, Source,      D1),
	AROS_LHA(STRPTR, Dest,        D2),
	AROS_LHA(LONG,   DestLength,  D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 161, Dos)

/*  FUNCTION
	Similar to ParsePattern(), only case insensitive (see there
	for more information). For use with MatchPatternNoCase().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ParsePattern(), MatchPatternNoCase().

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
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
                        PUT(ToLower(b));
                    }
                    if(a>=0x81&&a<=0x8e)
                    {
                        PUT(MP_ESCAPE);
                        a-=0x80;
                    }
                    PUT(ToLower(a));
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
                PUT(ToLower(a));
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
    AROS_LIBFUNC_EXIT
} /* ParsePatternNoCase */
