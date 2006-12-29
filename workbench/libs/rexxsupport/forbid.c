/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Disable task switching
    Lang: English
*/

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <rexx/storage.h>
#include <rexx/errors.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "rexxsupport_intern.h"
#include "rxfunctions.h"

/* If not in forbid state keep the Permit() count in a RexxVar from the script,
 * when in forbid state use local forbid_nest variable to count the Forbid() nesting
 * this is to avoid calling SetRexxVar when in forbid state
 */
static BOOL inforbid = FALSE;
static int forbid_nest = 0;

LONG rxsupp_forbid(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    char val[5];
    
    if (!inforbid)
    {
	char *s;
	int permit_nest;
	
	if (GetRexxVar(msg, NEST_VAR, &s) == RC_OK)
	    permit_nest = *(int *)s;
	else
	    permit_nest = -1;
    
	permit_nest++;
	snprintf(val, 5, "%d", permit_nest);

	if (permit_nest == 0)
	{
	    Forbid();
	    inforbid = TRUE;
	    forbid_nest = 0;
	}
	else
	    if (SetRexxVar(msg, NEST_VAR, (char *)&permit_nest, sizeof(int)) != RC_OK)
	    {
		*argstring = NULL;
		return ERR10_012;
	    }
    }
    else /* inforbid == TRUE */
    {
	forbid_nest++;
	snprintf(val, 5, "%d", forbid_nest);
    }

    *argstring = CreateArgstring(val, strlen(val));
    return RC_OK;
}

LONG rxsupp_permit(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    char val[5];

    if (!inforbid)
    {
	char *s;
	int permit_nest;
	
	if (GetRexxVar(msg, NEST_VAR, &s) == RC_OK)
	    permit_nest = *(int *)s;
	else
	    permit_nest = -1;
    
	permit_nest--;
	snprintf(val, 5, "%d", permit_nest);

	if (SetRexxVar(msg, NEST_VAR, (char *)&permit_nest, sizeof(int)) != RC_OK)
	{
	    *argstring = NULL;
	    return ERR10_012;
	}
    }
    else /* inforbid == TRUE */
    {
	forbid_nest--;
	snprintf(val, 5, "%d", forbid_nest);
	
	if (forbid_nest < 0)
	{
	    Permit();
	    inforbid = FALSE;
	}
    }
    
    *argstring = CreateArgstring(val, strlen(val));
    return RC_OK;
}
    
