/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id: freemem.c 15089 2002-08-04 19:48:12Z verhaegs $

    Desc: Return the type of a RexxMsg
    Lang: English
*/

#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <exec/types.h>
#include <rexx/storage.h>
#include <rexx/errors.h>

#include <ctype.h>
#include <stdlib.h>

#include "rexxsupport_intern.h"
#include "rxfunctions.h"

LONG rxsupp_typepkt(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    struct RexxMsg *msg2 = *(struct RexxMsg **)ARG1(msg);
    
    if ((msg->rm_Action & RXARGMASK) == 1)
    {
	*argstring = CreateArgstring((UBYTE *)&msg2->rm_Action, sizeof(msg2->rm_Action));
    }
    else /* 2 Args */
    {
	char c;
	
	switch (toupper(*(char *)ARG2(msg)))
	{
	case 'A':
	    c = '0' + (char)(msg2->rm_Action & RXARGMASK);
	    break;
	    
	case 'C':
	    c = ((msg2->rm_Action & RXCODEMASK) == RXCOMM) ? '1' : '0';
	    break;
	    
	case 'F':
	    c = ((msg2->rm_Action & RXCODEMASK) == RXFUNC) ? '1' : '0';
	    break;
	}
	
	*argstring = CreateArgstring(&c, 1);
    }
    
    return RC_OK;
}
