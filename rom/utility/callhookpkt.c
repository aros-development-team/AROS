/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CallHookPkt - Call an Amiga callback hook.
    Lang: english
*/
#include "intern.h"

/*****************************************************************************

    NAME */
#include <utility/hooks.h>
#include <aros/asmcall.h>
#include <proto/utility.h>

	AROS_LH3(IPTR, CallHookPkt,

/*  SYNOPSIS */
	AROS_LHA(struct Hook *, hook, A0),
	AROS_LHA(APTR         , object, A2),
	AROS_LHA(APTR         , paramPacket, A1),

/*  LOCATION */
	struct Library *, UtilityBase, 17, Utility)

/*  FUNCTION
	Call the callback hook defined by a Hook structure.
	This is effectively a long jump to the hook->h_Entry vector
	of the structure.

	The Hook will be called with the same arguments as this function.
	If your compiler cannot support correctly registered arguments
	(most can), you can use the HookEntry function defined in amiga.lib
	to push the arguments on the stack and call your function.

	See the include file utility/hooks.h for more information.

    INPUTS
	hook	    -	Pointer to an initialized Hook structure. See the
			include file <utility/hooks.h> for a definition.
	object	    -	The object that this Hook is to act upon.
	paramPacket -	The arguments to this callback. This will depend
			entirely on the type of the object.

    RESULT
	Depends upon the Hook itself.

    NOTES

    EXAMPLE

    BUGS
	If your callback function does not have the correct register
	definitions, the result of this function is entirely unreliable.

	You can get the correct register definitions by using the AROS_UFHA()
	macros (See <utility/hooks.h>).

    SEE ALSO
	amiga.lib/CallHook()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,UtilityBase)

    return AROS_UFC3(IPTR, hook->h_Entry,
	AROS_UFCA(struct Hook *, hook, A0),
	AROS_UFCA(APTR,          object, A2),
	AROS_UFCA(APTR,  paramPacket, A1)
    );

    AROS_LIBFUNC_EXIT
} /* CallHookPkt */
