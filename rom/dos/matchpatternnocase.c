/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, MatchPatternNoCase,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, pat, D1),
	AROS_LHA(STRPTR, str, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 162, Dos)

/*  FUNCTION

    Similar to MatchPattern(), only case insensitive (see there for
    more information). For use with ParsePatternNoCase().

    INPUTS

    pat  --  Pattern as returned by ParsePatternNoCase()
    str  --  String to match against the pattern 'pat'

    RESULT

    Boolean telling whether the match was successful or not.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    MatchPattern(), ParsePatternNoCase().

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    STRPTR s;
    LONG match=0;
    struct markerarray ma, *macur=&ma, *cur2;
    LONG macnt=0, cnt2;
    ULONG level;
    UBYTE a, b, c, t;
    LONG error;
#undef ERROR
#define ERROR(a) { error=(a); goto end; }

    ma.next=ma.prev=NULL;
    for(;;)
        switch(*pat)
        {
            case P_REPBEG: /* _#(_a), _#a_ or _#[_a] */
                PUSH(0,++pat,str);
                level=1;
                for(;;)
                {
                    c=*pat++;
                    if(c==P_REPBEG)
                        level++;
                    else if(c==P_REPEND)
                        if(!--level)
                            break;
                }
                break;
            case P_REPEND: /* #(a_)_ */
                level=1;
                for(;;)
                {
                    c=*--pat;
                    if(c==P_REPEND)
                        level++;
                    else if(c==P_REPBEG)
                        if(!--level)
                            break;
                }
                break;
            case P_NOT: /* _~(_a) */
                s=++pat;
                level=1;
                for(;;)
                {
                    c=*s++;
                    if(c==P_NOT)
                        level++;
                    else if(c==P_NOTEND)
                        if(!--level)
                            break;
                }
                PUSH(1,s,str);
                break;
            case P_NOTEND: /* ~(a_)_ */
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
            case P_ORSTART: /* ( */
                s=++pat;
                level=1;
                for(;;)
                {
                    c=*s++;
                    if(c==P_ORSTART)
                        level++;
                    else if(c==P_ORNEXT)
                    {
                        if(level==1)
                        { PUSH(0,s,str); }
                    }else if(c==P_OREND)
                        if(!--level)
                            break;
                }
                break;
            case P_ORNEXT: /* | */
                pat++;
                level=1;
                for(;;)
                {
                    c=*pat++;
                    if(c==P_ORSTART)
                        level++;
                    else if(c==P_OREND)
                        if(!--level)
                            break;
                }
                break;
            case P_OREND: /* ) */
                pat++;
                break;
            case P_SINGLE: /* ? */
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
            case P_CLASS: /* [ */
                pat++;
                for(;;)
                {
                    a=b=*pat++;
                    if(a==P_CLASS)
                    {
                        POP(t,pat,str);
                        if(t&&*str)
                        { PUSH(1,pat,str+1); }
                        break;
                    }
                    if(*pat=='-')
                    {
                        b=*++pat;
			if(b==P_CLASS)
			    b=255;
                    }
		    c=ToUpper(*str);
                    if(c>=a&&c<=b)
                    {
                        str++;
                        while(*pat++!=P_CLASS)
                            ;
                        break;
                    }
                }
                break;
            case P_NOTCLASS: /* [~ */
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
                    if(a==P_CLASS)
                    {
                        str++;
                        break;
                    }
                    if(*pat=='-')
                    {
			b=*++pat;
			if(b==P_CLASS)
                            b=255;
                    }
		    c=ToUpper(*str);
                    if(c>=a&&c<=b)
                    {
                        POP(t,pat,str);
                        if(t&&*str)
                        { PUSH(1,pat,str+1); }
                        break;
                    }
                }
                break;
            case P_ANY: /* #? */
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
            default:
                if(*pat++==ToUpper(*str))
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
} /* MatchPatternNoCase */
