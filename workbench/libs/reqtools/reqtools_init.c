/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    ReqTools initialization code.
*/

/****************************************************************************************/


#include "reqtools_intern.h"
#include <exec/types.h>
#include <exec/resident.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <devices/conunit.h>
#include <utility/utility.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/classes.h>
#include <libraries/reqtools.h>
#include <aros/symbolsets.h>
#include <aros/macros.h>

#include <string.h>

#define DEBUG 1
#include <aros/debug.h>

#include <exec/libraries.h>
#include <exec/alerts.h>
#include LC_LIBDEFS_FILE

#include "general.h"
#include "boopsigads.h"
#include "rtfuncs.h"

/****************************************************************************************/

/* Global variables */

#define __RT_DEFINEVARS
#include "globalvars.h"
#undef __RT_DEFINEVARS

/****************************************************************************************/


AROS_SET_LIBFUNC(Init, LIBBASETYPE, RTBase)
{
    AROS_SET_LIBFUNC_INIT;
    
    ReqToolsBase = (struct ReqToolsBase *)RTBase;
        
    D(bug("reqtools.library: Inside libinit func\n"));
    
    return RTFuncs_Init((struct ReqToolsBase *) RTBase, NULL) != NULL;
    
    AROS_SET_LIBFUNC_EXIT;
}

/****************************************************************************************/

AROS_SET_LIBFUNC(OpenLib, LIBBASETYPE, RTBase)
{
    AROS_SET_LIBFUNC_INIT;
    
    D(bug("reqtools.library: Inside libopen func\n"));
 
    /*
      This function is single-threaded by exec by calling Forbid.
      If you break the Forbid() another task may enter this function
      at the same time. Take care.
    */
    
    D(bug("reqtools.library: Inside libopen func\n"));
    
    return RTFuncs_Open((struct ReqToolsBase *) RTBase, 0) != NULL;
    
    AROS_SET_LIBFUNC_EXIT;
}

/****************************************************************************************/

AROS_SET_LIBFUNC(CloseLib, LIBBASETYPE, RTBase)
{
    AROS_SET_LIBFUNC_INIT;
    
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    D(bug("reqtools.library: Inside libclose func.\n"));

    RTFuncs_Close((struct ReqToolsBase *) RTBase);
    
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT;
}

/****************************************************************************************/

AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, RTBase)
{
    AROS_SET_LIBFUNC_INIT;
    
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
 
    D(bug("reqtools.library: Inside libexpunge func.\n"));

    RTFuncs_Expunge((struct ReqToolsBase *) RTBase);
    
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT;
}

/****************************************************************************************/

ADD2INITLIB(Init, 0);
ADD2OPENLIB(OpenLib, 0);
ADD2CLOSELIB(CloseLib, 0);
ADD2EXPUNGELIB(Expunge, 0);
