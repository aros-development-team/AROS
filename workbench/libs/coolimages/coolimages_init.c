/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <proto/intuition.h>
#include <proto/cybergraphics.h>

#include "coolimages_intern.h"
#include LC_LIBDEFS_FILE

#include <aros/debug.h>

/****************************************************************************************/

AROS_SET_LIBFUNC(Init, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT;
    
    D(bug("Inside Init func of coolimages.library\n"));

    if (!cool_buttonclass)
    {
    	if (!InitCoolButtonClass(CyberGfxBase)) return FALSE;
    	
	cool_buttonclass->cl_ID = COOLBUTTONGCLASS;
	AddClass(cool_buttonclass);
    }
    
    if (!cool_imageclass)
    {
    	if (!InitCoolImageClass(CyberGfxBase)) return FALSE;
	
	cool_imageclass->cl_ID = COOLIMAGECLASS;
	AddClass(cool_imageclass);
    }
    
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT;
}

/****************************************************************************************/

AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT;
    
    D(bug("Inside Expunge func of coolimages.library\n"));

    CleanupCoolImageClass();
    CleanupCoolButtonClass();
    
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
