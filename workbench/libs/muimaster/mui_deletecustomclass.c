/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
	The function itself is a bug ;-) Remove it!

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct MUIMasterBase *,MUIMasterBase)

    if (mcc && FreeClass(mcc->mcc_Class))
    {
	mui_free(mcc);

        MUI_FreeClass(mcc->mcc_Super);

	return TRUE;
    }

    return FALSE;

    AROS_LIBFUNC_EXIT

} /* MUIA_DeleteCustomClass */
