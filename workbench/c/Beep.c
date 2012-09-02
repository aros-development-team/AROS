/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Beep
    Lang: English
*/

/*****************************************************************************

    NAME
	Beep

    FORMAT
	Beep

    SYNOPSIS

    LOCATION
	C:

    FUNCTION
	BEEP produces a beep via Intuition DisplayBeep(NULL).

    EXAMPLE

    SEE ALSO

******************************************************************************/

#include <proto/exec.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <proto/intuition.h>
#include <intuition/screens.h>

const TEXT version[] = "$VER: Beep 41.2 (03.03.2011)";

__startup AROS_PROCH(Start, argstr, argsize, SysBase)
{
    AROS_PROCFUNC_INIT
    
    struct IntuitionBase *IntuitionBase;

    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 0);
    if (!IntuitionBase)
    	return RETURN_FAIL;

    DisplayBeep( NULL );
    
    CloseLibrary(&IntuitionBase->LibNode);
    return RETURN_OK;

    AROS_PROCFUNC_EXIT
}

