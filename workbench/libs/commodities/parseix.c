/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef  DEBUG
#define  DEBUG 0
#endif

#include "cxintern.h"

#include <aros/debug.h>

#include <libraries/commodities.h>
#include <proto/commodities.h>
#include <proto/utility.h>
#define __NOLIBBASE__ 1
#include <proto/keymap.h>
#undef __NOLIBBASE__
#include <devices/inputevent.h>
#include <devices/keymap.h>
#include <string.h>

#include "parse.h"

#define DEBUG_PARSEIX(x)	;
#define DEBUG_PMATCH(x)		;

BOOL pMatch(pix_S[], CONST_STRPTR, LONG *, BOOL *, struct Library *CxBase);
VOID GetNext(CONST_STRPTR *);
BOOL IsSeparator(char);

/*****************************************************************************

    NAME */

    AROS_LH2(LONG, ParseIX,

/*  SYNOPSIS */

	AROS_LHA(CONST_STRPTR, desc, A0),
	AROS_LHA(IX *  , ix  , A1),

/*  LOCATION */

	struct Library *, CxBase, 22, Commodities)

/*  FUNCTION

    Fill in an InputXpression 'ix' according to the specifications given
    in the string pointed to by 'desc'.

    The string should be formatted according to:

    [class] {[-] (qualifier|synonym)} [[-] upstroke] [HighMap|ANSICode]

    For more information on this, consult "xxx/CxParse.doc".

    INPUTS

    desc  --  pointer to the string specifying the conditions and codes of
              the InputXpression.
    ix    --  pointer to an (uninitizlized) InputXpression structure that
              will be filled according to 'desc'.

    RESULT

    0   --  Everything went OK.
    -1  --  Tokens after end
    -2  --  'desc' was NULL

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    MatchIX(), <libraries/commodities.h>

    INTERNALS

    HISTORY

    10.05.97   SDuvan  implemented

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    LONG val;
    BOOL dash, upstrdash = TRUE;
    BOOL upstroke = FALSE;

    struct InputEvent event;
    
    /* Set as standard if no class is specified in the description */
#if 0
    /* Caller is expected to set this! */
    ix->ix_Version = IX_VERSION;
#endif
    ix->ix_Class = IECLASS_RAWKEY;
    ix->ix_Code = 0;
    ix->ix_CodeMask = 0xFFFF;
    ix->ix_Qualifier = 0;
    ix->ix_QualMask = IX_NORMALQUALS & ~(IEQUALIFIER_INTERRUPT | IEQUALIFIER_MULTIBROADCAST);
    ix->ix_QualSame = 0;
    
    if (desc == NULL)
    {
	ix->ix_Code = 0xFFFF;

	return -2;
    }

    DEBUG_PARSEIX(dprintf("ParseIX: ix = 0x%lx, desc = \"%s\"\n", ix, desc));
    
    while (IsSeparator(*desc))
    {
	desc++;
    }
    
    dash = FALSE;
    
    if (pMatch(pix_Class, desc, &val, &dash, CxBase))
    {
	ix->ix_Class = val;
	GetNext(&desc);
    }
    
    while (TRUE)
    {
	dash = TRUE;
	
	if (pMatch(pix_IEvent, desc, &val, &dash, CxBase))
	{
	    if (dash)
	    {
		ix->ix_QualMask &= ~val;
	    }
	    else
	    {
		ix->ix_Qualifier |= val;
	    }
	    
	    GetNext(&desc);
	}
	else
	{
	    dash = TRUE;
	    
	    if (pMatch(pix_Synonyms, desc, &val, &dash, CxBase))
	    {
	        ix->ix_QualSame |= val;
		
		if (dash)
		{
		    switch (val)
		    {
		    case IXSYM_SHIFT:
			ix->ix_QualMask &= ~IXSYM_SHIFTMASK;
			break;
			
		    case IXSYM_ALT:
			ix->ix_QualMask &= ~IXSYM_ALTMASK;
			break;
			
		    case IXSYM_CAPS:
			ix->ix_QualMask &= ~IXSYM_CAPSMASK;
			break;
		    }
		}
		else
		{
		    switch (val)
		    {
		    case IXSYM_SHIFT:
			ix->ix_Qualifier |= IXSYM_SHIFTMASK;
			break;
			
		    case IXSYM_ALT:
			ix->ix_Qualifier |= IXSYM_ALTMASK;
			break;
			
		    case IXSYM_CAPS:
			ix->ix_Qualifier |= IXSYM_CAPSMASK;
			break;
		    }
		}
		
		GetNext(&desc);
	    }
	    else
	    {
		break;
	    }
	}
    }
    
    if (pMatch(pix_Upstroke, desc, &val, &upstrdash, CxBase))
    {
	upstroke = TRUE;
	GetNext(&desc);
    }
    
    dash = FALSE;
    
    if (pMatch(pix_Highmap, desc, &val, &dash, CxBase))
    {
	ix->ix_Code = val;
    }
    else
    {
	if (*desc != '\0')
	{
	    if (InvertKeyMap(*desc, &event, NULL))
	    {
		ix->ix_Code = event.ie_Code;
	    }
	}
    }
    
    if (upstroke)
    {
	if (upstrdash)
	{
	    ix->ix_CodeMask &= ~IECODE_UP_PREFIX;
	}
	else
	{
	    ix->ix_Code |= IECODE_UP_PREFIX;
	}
    }
    
    while (!(IsSeparator(*desc)))
    {
	desc++;
    }

    DEBUG_PARSEIX(dprintf("ParseIX: Class 0x%lx Code 0x%lx CodeMask 0x%lx\n"
			  "ParseIX: Qualifier 0x%lx QualMask 0x%lx QualSame 0x%lx\n",
			  ix->ix_Class,
			  ix->ix_Code,
			  ix->ix_CodeMask,
			  ix->ix_Qualifier,
			  ix->ix_QualMask,
			  ix->ix_QualSame));
    
    if (*desc == '\0')
    {
	return 0;
    }
    else
    {
	DEBUG_PARSEIX(dprintf("ParseIX: fail, desc 0x%lx *desc 0x%lx\n", desc, *desc));
	return -1;
    }
    
    AROS_LIBFUNC_EXIT
} /* ParseIX */


BOOL pMatch(pix_S words[], CONST_STRPTR string, LONG *v, BOOL *dash,
	    struct Library *CxBase)
{
    CONST_STRPTR nstr = string;
    int    i;

    DEBUG_PMATCH(dprintf("pMatch: words[0] = \"%s\" string \"%s\" dash %d\n",
			 words[0].name, string, *dash));
    
    if (*dash)
    {
	if (*nstr == '-')
	{
	    nstr++;
	    *dash = TRUE;
	}
	else
	{
	    *dash = FALSE;
	}
    }
    
    for (i = 0; words[i].name != NULL; i++)
    {
	if (Strnicmp(nstr, words[i].name, strlen(words[i].name)) == 0)
	{
	    *v = words[i].value;
	    DEBUG_PMATCH(dprintf("pMatch: value 0x%lx\n", *v));
	    return TRUE;
	}
    }

    DEBUG_PMATCH(dprintf("pMatch: not found\n"));
    return FALSE;
}


VOID GetNext(CONST_STRPTR *str)
{
    while (!(IsSeparator(**str)))
    {
	(*str)++;
    }
    
    while (IsSeparator(**str) && !(**str=='\0'))
    {
	(*str)++;
    }
}


BOOL IsSeparator(char a)
{
    if (a == ' ' || a == '\n' || a == '\t' || a == ',' || a == '\0')
    {
	return TRUE;
    }
    else
    {
	return FALSE;
    }
}
