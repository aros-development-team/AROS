#include <exec/types.h>
#include <intuition/classusr.h>
#include <libraries/desktop.h>
#include <libraries/mui.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "desktop_intern.h"
#include "support.h"

#include "iconcontainerclass.h"
#include "iconcontainerobserver.h"

#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */

        #include <proto/desktop.h>

        AROS_LH2(Object*, CreateDesktopObjectA,

/*  SYNOPSIS */
        AROS_LHA(ULONG,             kind, D0),
        AROS_LHA(struct TagItem *,  tags, A0),

/*  LOCATION */
        struct DesktopBase *, DesktopBase, 8, Desktop)

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

	Object *newObject=NULL;
	Object *semanticObject;

	switch(kind)
	{
		case CDO_IconContainer:
			newObject=NewObject(IconContainer->mcc_Class, NULL, TAG_END);

			semanticObject=NewObject(IconContainerObserver->mcc_Class, NULL,
						ICOA_Presentation, newObject,
						TAG_MORE, tags);


			break;
	}

	return newObject;

    AROS_LIBFUNC_EXIT
} /* CreateWorkbenchObjectA */


