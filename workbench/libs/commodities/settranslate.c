/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include <libraries/commodities.h>
#include <exec/libraries.h>
#include <proto/exec.h>

    AROS_LH2(VOID, SetTranslate,

/*  SYNOPSIS */

	AROS_LHA(CxObj *            , translator, A0),
	AROS_LHA(struct InputEvent *, events    , A1),

/*  LOCATION */

	struct Library *, CxBase, 19, Commodities)

/*  FUNCTION

    Set translation (a list of input events) for a commodity translator
    object.

    INPUTS

    translator  --  the commodity translator the translation result of which
                    to set (may be NULL)
    events      --  the new input event list

    RESULT

    NOTES

    If events is set to NULL, all commodity messages passed to the object
    are swallowed. Neither commodities.library nor any other commodities
    user will change your list of InputEvents; however, it will be used
    asynchronously to the application program which means you shouldn't
    in any way corrupt the chain.
   
    EXAMPLE

    BUGS

    SEE ALSO

    cx_lib/CxTranslate(), <devices/inputevent.h>

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if (translator == NULL)
    {
	return;
    }

    ObtainSemaphore(&GPB(CxBase)->cx_SignalSemaphore);
    
    if (CXOBJType(translator) != CX_TRANSLATE)
    {
	translator->co_Error |= COERR_BADTYPE;
    }
    else
    {
	translator->co_Ext.co_IE = events;
    }

    ReleaseSemaphore(&GPB(CxBase)->cx_SignalSemaphore);
    
    AROS_LIBFUNC_EXIT
} /* SetTranslate */
