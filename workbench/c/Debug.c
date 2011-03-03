/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Beep
    Lang: English
*/

/*****************************************************************************

    NAME
	Debug

    FORMAT
	Debug

    SYNOPSIS

    LOCATION
	C:

    FUNCTION
	Activates built-in AROS debugger (SAD)

    EXAMPLE

    SEE ALSO

******************************************************************************/

#include <proto/exec.h>

const TEXT version[] = "$VER: Debug 1.0 (16.08.2010)";

AROS_ENTRY(__startup static ULONG, Start,
	   AROS_UFHA(char *, argstr, A0),
	   AROS_UFHA(ULONG, argsize, D0),
	   struct ExecBase *, SysBase)
{
    AROS_USERFUNC_INIT

    Debug(0);
    return 0;
    
    AROS_USERFUNC_EXIT
}
