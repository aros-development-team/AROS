/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Support functions for MatchFirst/MatchNext/MatchEnd
    Lang: english
*/

/****************************************************************************************/

#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "dos_intern.h"
#include <aros/debug.h>

#include <string.h>

/****************************************************************************************/

#define COMPTYPE_NORMAL  1 
#define COMPTYPE_PATTERN 2
#define COMPTYPE_UNKNOWN 3

/****************************************************************************************/

struct AChain *Match_AllocAChain(LONG extrasize, struct DosLibrary *DOSBase)
{
    return AllocVec(sizeof(struct AChain) + extrasize, MEMF_PUBLIC | MEMF_CLEAR);
}

/****************************************************************************************/

void Match_FreeAChain(struct AChain *ac, struct DosLibrary *DOSBase)
{
    FreeVec(ac);
}

/****************************************************************************************/

static void RemoveTrailingSlash(STRPTR s)
{
    LONG len = strlen(s);

    if (len >= 2)
    {
	if ((s[len - 1] == '/') &&
	    ((s[len - 2] != '/') && (s[len - 2] != ':')))
	{
	    s[len - 1] = '\0';
	}
    }
}

/**************************************************************************

The job of Match_BuildAChainList is to split the pattern string passed to
MatchFirst into path components. Most imporant rules (as found out after
hours of testing on Amiga):

  - Each path component containing a pattern string is put into a single
    AChain
  - If there are several successive path components *without* pattern then
    this are merged into one single AChain.
  - No matter what: the last path component always gets into its own single
    AChain.

Examples: [<???>] is one AChain

  pictures                          [pictures]
  pictures/#?                       [pictures} [#?]
  work:                             [work:] []
  work:pictures                     [work:} [pictures]
  work:pictures/#?                  [work:pictures] [#?]
  work:pictures/aros                [work:pictures] [aros]
  work:pictures/aros/games          [work:pictures/aros] [games]
  work:#?/aros/games                [work:] [#?] [aros] [games}
  work:#?/#?/aros/games/quake       [work:} [#?] [#?] [aros/games] [quake]

**************************************************************************/
  
LONG Match_BuildAChainList(STRPTR pattern, struct AnchorPath *ap,
			   struct AChain **retac, struct DosLibrary *DOSBase)
{
    struct AChain 	*baseac = 0, *prevac = 0, *ac;
    STRPTR 		patterncopy = 0;
    STRPTR		patternstart, patternend, patternpos;
    LONG 		len, error = 0;
    WORD		comptype = COMPTYPE_UNKNOWN;
    WORD		compcount = 0;
    WORD		i;
    UBYTE		c;

    *retac = 0;

    len = strlen(pattern);

    patterncopy = AllocVec(len + 1, MEMF_PUBLIC);
    if (!patterncopy)
    {
        error = ERROR_NO_FREE_STORE;
	goto done;
    }
    
    strcpy(patterncopy, pattern);

    RemoveTrailingSlash(patterncopy);

    patternstart = patterncopy;

    patternpos = strchr(patterncopy, ':');
    if (!patternpos)
    {
	comptype = COMPTYPE_UNKNOWN;
	patternpos = patternstart;
	patternend = patternstart;
    }
    else
    {
	comptype = COMPTYPE_NORMAL;
	patternend = patternpos++;
	compcount = 1;
    }

    do
    {
	for(;;)
	{
	    c = *patternpos;
	    if (c == '/')
	    {
		if (comptype == COMPTYPE_UNKNOWN)
		{
		    comptype = COMPTYPE_NORMAL;
		    patternend = patternpos;
		}
		else if (comptype == COMPTYPE_NORMAL)
		{
		    patternend = patternpos;
		    compcount++;
		}
		if (comptype == COMPTYPE_PATTERN)
		{
		    patternend = patternpos;
		    break;		
		}
	    }
	    else if (c == '\0')
	    {
		if (comptype == COMPTYPE_UNKNOWN)
		{
		    comptype = COMPTYPE_NORMAL;
		    patternend = patternpos;
		    break;
		}
		if (comptype == COMPTYPE_NORMAL)
		{
		    compcount++;
		    break;
		}
		patternend = patternpos;
		break;
	    }
	    else if ((c == '#') ||
		     (c == '~') ||
		     (c == '[') ||
		     (c == ']') ||
		     (c == '?') ||
		     (c == '*') ||
		     (c == '(') ||
		     (c == ')') ||
		     (c == '|') ||
		     (c == '%'))
	    {
		if (comptype == COMPTYPE_NORMAL)
		{
		    break;
		}
		comptype = COMPTYPE_PATTERN;
	    }
	    
	    patternpos++;

	} /* for(;;) */

	len = (LONG)(patternend - patternstart + 2);
	if (comptype == COMPTYPE_PATTERN) len = len * 2 + 2;

	ac = Match_AllocAChain(len, DOSBase);
	if (!ac)
	{
	    error = ERROR_NO_FREE_STORE;
	    goto done;
	}

	if (comptype == COMPTYPE_NORMAL)
	{
	    if (*patternend == '\0')
	    {
		strcpy(ac->an_String, patternstart);
	    } else {
		c = patternend[1];
		patternend[1] = '\0';
		strcpy(ac->an_String, patternstart);
		patternend[1] = c;
	    }
	    
	} /* if (comptype == COMPTYPE_NORMAL) */
	else
	{
	    if (*patternend == '\0')
	    {
		i = ParsePatternNoCase(patternstart, ac->an_String, len);
		if (i == 0) 
		{
		    /* It is not a pattern, although we guessed it was one.
		       Do the strcpy, otherwise we have uppercase stuff in
		       ac->an_String because of ParsePatternNOCASE() */
		    strcpy(ac->an_String, patternstart);
		}
	    }
	    else
	    {
		c = patternend[1];
		patternend[1] = '\0';
		i = ParsePatternNoCase(patternstart, ac->an_String, len);
		if (i == 0) 
		{
		    /* It is not a pattern, although we guessed it was one.
		       Do the strcpy, otherwise we have uppercase stuff in
		       ac->an_String because of ParsePatternNOCASE() */
		    strcpy(ac->an_String, patternstart);
		}
		patternend[1] = c;
	    }
	    
	    if (i == -1)
	    {
		error = ERROR_BAD_TEMPLATE;
		Match_FreeAChain(ac, DOSBase);ac = 0;
		goto done;
	    }
	    
	    if (i)
	    {
	        ac->an_Flags |= DDF_PatternBit;
		ap->ap_Flags |= APF_ITSWILD;
	    }
	    
	} /* if (comptype == COMPTYPE_NORMAL) else ... */

	RemoveTrailingSlash(ac->an_String);

	if (!prevac)
	{
	    baseac = ac;
	}
	else
	{
	    prevac->an_Child = ac;
	    ac->an_Parent = prevac;
	}

	prevac = ac;

	patternpos = patternend;
	comptype = COMPTYPE_UNKNOWN;
	patternstart = patternend = patternpos + 1;
	compcount = 0;

    } while (*patternpos++ != '\0');

done:
    if (patterncopy) FreeVec(patterncopy);

    if (!error)
    {
#if MATCHFUNCS_NO_DUPLOCK
        /*
	* No DupLock() here, because then we would have to UnLock it in
	* MatchEnd and we would not know any valid lock to which we could
	* CurrentDir after, because we must make sure there is a valid
	* CurrentDir after MatchEnd.
	*/
	
        baseac->an_Lock = CurrentDir(0);
	CurrentDir(baseac->an_Lock);
#endif
	
	*retac = baseac;
    }
    else
    {
        ap->ap_Flags |= APF_NOMEMERR;
	
     	if (baseac)
	{
	    #define nextac prevac /* to not have to add another variable */

	    ac = baseac;
	    while(ac)
	    {
		nextac = ac->an_Child;
		Match_FreeAChain(ac, DOSBase);
		ac = nextac;
	    }
	}
    }

    return error;}

/******************************************************************************/

LONG Match_MakeResult(struct AnchorPath *ap, struct DosLibrary *DOSBase)
{
    LONG error = 0;

    ap->ap_Info = ap->ap_Current->an_Info;
    if (ap->ap_Strlen)
    {
	ap->ap_Buf[0] = 0;
	if (NameFromLock(ap->ap_Current->an_Lock, ap->ap_Buf, ap->ap_Strlen))
	{
	    if (!AddPart(ap->ap_Buf, ap->ap_Current->an_Info.fib_FileName, ap->ap_Strlen))
	    {
		error = IoErr();
	    }
	} else {
	    error = IoErr();
	}
    }

    return error;
}

/******************************************************************************/
