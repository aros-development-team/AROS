/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

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

#include "iconclass.h"
#include "iconobserver.h"
#include "iconcontainerclass.h"
#include "iconcontainerobserver.h"
#include "observer.h"

#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */

        #include <proto/desktop.h>

        AROS_LH1(struct DesktopOperationItem*, GetMenuItemList,

/*  SYNOPSIS */
        AROS_LHA(ULONG,      operationType, D0),

/*  LOCATION */
        struct DesktopBase *, DesktopBase, 10, Desktop)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct DesktopOperationItem *doi=NULL;
	struct DesktopOperation *dop;
	LONG items=0, index=0;

	dop=DesktopBase->db_OperationList.lh_Head;
	while(dop->do_Node.ln_Succ)
	{
		if(dop->do_Code & operationType)
			items++;
		dop=(struct DesktopOperation*)dop->do_Node.ln_Succ;
	}

	if(items)
		doi=(struct DesktopOperationItem*)AllocVec(sizeof(struct DesktopOperationItem)*(items+1), MEMF_ANY);
	else
		return doi;

	dop=DesktopBase->db_OperationList.lh_Head;
	while(dop->do_Node.ln_Succ)
	{
		if(dop->do_Code & operationType)
		{
			doi[index].doi_Code=dop->do_Code;
			doi[index].doi_Name=dop->do_Name;

			index++;
		}

		dop=(struct DesktopOperation*)dop->do_Node.ln_Succ;
	}

	doi[index].doi_Code=0;
	doi[index].doi_Name=NULL;

	return doi;

    AROS_LIBFUNC_EXIT
} /* CreateWorkbenchObjectA */


