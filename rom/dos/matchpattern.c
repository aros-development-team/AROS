/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/10/24 15:50:32  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/11 12:54:46  digulla
    A couple of new DOS functions from M. Fleischer

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	AROS_LH2(BOOL, MatchPattern,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, pat, D1),
	AROS_LHA(STRPTR, str, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 141, Dos)

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /*
        A simple method for pattern matching with multiple wildcards:
        I use markers that consist of both a pointer into the string
        and one into the pattern. The marker simply follows the string
        and everytime it hits a wildcard it's split into two new markers
        (one to assume that the wildcard match has ended and one to assume
         that it continues). If a marker doesn't fit any longer it's
        removed and if all of them are gone the pattern mismatches.
        OTOH if any of the markers reaches the end of both the string
        and the pattern simultaneously the pattern matches the string.
    */
    STRPTR s;
    LONG match=0;
    struct markerarray ma, *macur=&ma, *cur2;
    LONG macnt=0, cnt2;
    ULONG level;
    UBYTE a, b, c, t;
    LONG error;
#define ERROR(a) { error=(a); goto end; }

    ma.next=ma.prev=NULL;
    for(;;)
        switch(*pat)
        {
            case MP_MULT: /* _#(_a) */
                /* Split the marker */
                PUSH(0,++pat,str);
                level=1;
                for(;;)
                {
                    c=*pat++;
                    if(c==MP_MULT)
                        level++;
                    else if(c==MP_MULT_END)
                        if(!--level)
                            break;
                }
                break;
            case MP_MULT_END: /* #(a_)_ */
                /* Go back to the start of the block */
                level=1;
                for(;;)
                {
                    c=*--pat;
                    if(c==MP_MULT_END)
                        level++;
                    else if(c==MP_MULT)
                        if(!--level)
                            break;
                }
                break;
            case MP_NOT: /* _~(_a) */
                s=++pat;
                level=1;
                for(;;)
                {
                    c=*s++;
                    if(c==MP_NOT)
                        level++;
                    else if(c==MP_NOT_END)
                        if(!--level)
                            break;
                }
                PUSH(1,s,str);
                break;
            case MP_NOT_END: /* ~(a_)_ */
                cnt2=macnt;
                cur2=macur;
                do
                {
                    cnt2--;
                    if(cnt2<0)
                    {
                        cnt2=127;
                        cur2=cur2->prev;
                    }
                }while(!cur2->marker[cnt2].type);
                if(!*str++)
                {
                    macnt=cnt2;
                    macur=cur2;
                }else
                    if(str>cur2->marker[cnt2].str)
                        cur2->marker[cnt2].str=str;
                POP(t,pat,str);
                if(t&&*str)
                { PUSH(1,pat,str+1); }
                break;
            case MP_OR: /* ( */
                s=++pat;
                level=1;
                for(;;)
                {
                    c=*s++;
                    if(c==MP_OR)
                        level++;
                    else if(c==MP_OR_NEXT)
                    {
                        if(level==1)
                        { PUSH(0,s,str); }
                    }else if(c==MP_OR_END)
                        if(!--level)
                            break;
                }
                break;
            case MP_OR_NEXT: /* | */
                pat++;
                level=1;
                for(;;)
                {
                    c=*pat++;
                    if(c==MP_OR)
                        level++;
                    else if(c==MP_OR_END)
                        if(!--level)
                            break;
                }
                break;
            case MP_OR_END: /* ) */
                pat++;
                break;
            case MP_SINGLE: /* ? */
                pat++;
                if(*str)
                    str++;
                else
                {
                    POP(t,pat,str);
                    if(t&&*str)
                    { PUSH(1,pat,str+1); }
                }
                break;
            case MP_SET: /* [ */
                pat++;
                for(;;)
                {
                    a=b=*pat++;
                    if(a==MP_SET_END)
                    {
                        POP(t,pat,str);
                        if(t&&*str)
                        { PUSH(1,pat,str+1); }
                        break;
                    }
                    if(a==MP_ESCAPE)
                        a=b=*pat++ +0x80;
                    else if(a==MP_DASH)
                    {
                        a=*pat++;
                        if(a==MP_ESCAPE)
                            a=*pat++ +0x80;
                        b=*pat++;
                        if(b==MP_ESCAPE)
                            b=*pat++ +0x80;
                    }
                    if(*str>=a&&*str<=b)
                    {
                        str++;
                        while(*pat++!=MP_SET_END)
                            ;
                        break;
                    }
                }
                break;
            case MP_NOT_SET: /* [~ */
                if(!*str)
                {
                    POP(t,pat,str);
                    if(t&&*str)
                    { PUSH(1,pat,str+1); }
                    break;
                }
                pat++;
                for(;;)
                {
                    a=b=*pat++;
                    if(a==MP_SET_END)
                    {
                        str++;
                        break;
                    }
                    if(a==MP_ESCAPE)
                        a=b=*pat++ +0x80;
                    else if(a==MP_DASH)
                    {
                        a=*pat++;
                        if(a==MP_ESCAPE)
                            a=*pat++ +0x80;
                        b=*pat++;
                        if(b==MP_ESCAPE)
                            b=*pat++ +0x80;
                    }
                    if(*str>=a&&*str<=b)
                    {
                        POP(t,pat,str);
                        if(t&&*str)
                        { PUSH(1,pat,str+1); }
                        break;
                    }
                }
                break;
            case MP_ALL: /* #? */
                /* This often used pattern has extra treatment to be faster */
                if(*str)
                { PUSH(0,pat,str+1); }
                pat++;
                break;
            case 0:
                if(!*str)
                {
                    match=1;
                    ERROR(0);
                }else
                {
                    POP(t,pat,str);
                    if(t&&*str)
                    { PUSH(1,pat,str+1); }
                }
                break;
            case MP_ESCAPE:
                pat++;
                if(0x80+*pat++==*str)
                    str++;
                else
                {
                    POP(t,pat,str);
                    if(t&&*str)
                    { PUSH(1,pat,str+1); }
                }
                break;
            default:
                if(*pat++==*str)
                    str++;
                else
                {
                    POP(t,pat,str);
                    if(t&&*str)
                    { PUSH(1,pat,str+1); }
                }
                break;
        }
end:
    macur=ma.next;
    while(macur!=NULL)
    {
        struct markerarray *next=macur->next;
        FreeMem(macur,sizeof(struct markerarray));
        macur=next;
    }
    ((struct Process *)FindTask(NULL))->pr_Result2=error;
    return match;
    AROS_LIBFUNC_EXIT
} /* MatchPattern */
