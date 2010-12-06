/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id:$

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

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
AROS_UFH2(__startup static int, Start,
	  AROS_UFHA(char *, argstr, A0),
	  AROS_UFHA(ULONG, argsize, D0))
{
    AROS_USERFUNC_INIT

    struct ExecBase *SysBase = *((APTR *)4);
#else
AROS_UFH3(__startup static int, Start,
	  AROS_UFHA(char *, argstr, A0),
	  AROS_UFHA(ULONG, argsize, D0),
	  AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
#endif

    Debug(0);
    return 0;
    
    AROS_USERFUNC_EXIT
}
