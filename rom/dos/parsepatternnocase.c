/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <proto/utility.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, ParsePatternNoCase,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, Source,     D1),
	AROS_LHA(STRPTR, Dest,       D2),
	AROS_LHA(LONG,   DestLength, D3),

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
			PUT(ToUpper(a));
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
		    PUT(ToUpper(a));
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
                PUT(ToUpper(a));
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
} /* ParsePatternNoCase */
