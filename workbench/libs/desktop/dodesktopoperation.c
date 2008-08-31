/*
   Copyright © 1995-2003, The AROS Development Team. All rights reserved.
   $Id$ 
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/classusr.h>
#include <libraries/desktop.h>
#include <libraries/mui.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "desktop_intern.h"
#include "support.h"

#include "operationclass.h"

#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */

        #include <proto/desktop.h>

        AROS_LH2(ULONG, DoDesktopOperationA,

/*  SYNOPSIS */

	AROS_LHA(ULONG, operationCode, D0),
	AROS_LHA(struct TagItem *, tags, D2),

/*  LOCATION */

        struct DesktopBase *, DesktopBase, 9, Desktop)
/*
   FUNCTION

   INPUTS

   RESULT

   NOTES

   EXAMPLE

   BUGS

   SEE ALSO

   INTERNALS

   HISTORY

   *****************************************************************************
 */
{
    AROS_LIBFUNC_INIT 
    struct DesktopOperation *dop,
                   *subdop;
    BOOL            found = FALSE;
    ULONG           result = 0;
    Object         *newObject;
    Object         *target;

    dop = (struct DesktopOperation *) DesktopBase->db_OperationList.lh_Head;
    while (dop->do_Node.ln_Succ && !found)
    {
        if (operationCode == dop->do_Code)
        {
            newObject = NewObjectA(dop->do_Impl->mcc_Class, NULL, NULL);
            if (newObject)
            {
                target = (Object *) GetTagData(DDO_Target, NULL, tags);
                if (target)
                {
                    result =
                        DoMethod(newObject, OPM_Execute, (IPTR) target,
                                 operationCode);
                }
                DisposeObject(newObject);
            }

            found = TRUE;
        }

        if (!IsListEmpty(&dop->do_SubItems))
        {
            subdop = (struct DesktopOperation *) dop->do_SubItems.lh_Head;
            while (subdop->do_Node.ln_Succ && !found)
            {
                if (operationCode == subdop->do_Code)
                {
                    newObject =
                        NewObjectA(subdop->do_Impl->mcc_Class, NULL, NULL);
                    if (newObject)
                    {
                        target = (Object *) GetTagData(DDO_Target, NULL, tags);
                        if (target)
                        {
                            result =
                                DoMethod(newObject, OPM_Execute, (IPTR) target,
                                         operationCode);
                        }
                        DisposeObject(newObject);
                    }

                    found = TRUE;
                }

                subdop = (struct DesktopOperation *) subdop->do_Node.ln_Succ;
            }

        }

        dop = (struct DesktopOperation *) dop->do_Node.ln_Succ;
    }

    return result;

    AROS_LIBFUNC_EXIT
} /* DoDesktopOperationA */
