/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm BOOL MUI_DeleteCustomClass(register __a0 struct MUI_CustomClass *mcc)
#else
	AROS_LH1(BOOL, MUI_DeleteCustomClass,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_CustomClass *, mcc, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 19, MUIMaster)
#endif
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
	CloseLibrary(mcc->mcc_Module);
	mui_free(mcc);
	return TRUE;
    }

    return FALSE;

    AROS_LIBFUNC_EXIT

} /* MUIA_DeleteCustomClass */
