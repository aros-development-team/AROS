/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

/*****************************************************************************

    NAME */
	AROS_LH1(BOOL, MUI_DeleteCustomClass,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_CustomClass *, mcc, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 19, MUIMaster)

/*  FUNCTION
	Delete private or public custom classes.

    INPUTS
	mcc - pointer from MUI_CreateCustomClass()

    RESULT
	TRUE  : success
	FALSE : some objects or sub classes were still in use.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MUI_CreateCustomClass()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (mcc)
    {
        Class *super = mcc->mcc_Super;
	
	if (FreeClass(mcc->mcc_Class))
        {
	    mui_free(mcc);

            MUI_FreeClass(super);

  	    return TRUE;
        }
    }

    return FALSE;

    AROS_LIBFUNC_EXIT

} /* MUIA_DeleteCustomClass */
