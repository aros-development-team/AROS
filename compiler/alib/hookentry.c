/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: amiga.lib function HookEntry()
    Lang: english
*/

#include <aros/system.h>
#include <stdarg.h>
#include "alib_intern.h"

/******************************************************************************

    NAME */
#include <aros/asmcall.h>
#include <proto/alib.h>

	AROS_UFH3(IPTR, HookEntry,

/*  SYNOPSIS */
	AROS_UFHA(struct Hook *, hook,   A0),
	AROS_UFHA(APTR,          object, A2),
	AROS_UFHA(APTR,          param,  A1))

/*  FUNCTION
	Some high level languages (HLL) don't allow to pass arguments in
	registers. For these HLLs, it's not possible to call a hook
	directly. To use hooks with these HLLs, you must put HookEntry into
	hook->h_Entry and the real callback function into hook->h_SubEntry.
	HookEntry will push the registers on the stack and then call
	hook->h_SubEntry.

    INPUTS
	hook - Call this hook.
	object - This is the object which is passed to the hook. The valid
	    values for this parameter depends on the definition of the called
	    hook.
	param - Pass these parameters to the specified object

    RESULT
	The return value depends on the definition of the hook.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CallHookA(), CallHook()

    HISTORY
	28.11.96 digulla created

******************************************************************************/
{
    AROS_USERFUNC_INIT
    return ((IPTR (*)(struct Hook *, APTR, APTR))(hook->h_SubEntry))
			(hook, object, param);
    AROS_USERFUNC_EXIT
} /* HookEntry */

