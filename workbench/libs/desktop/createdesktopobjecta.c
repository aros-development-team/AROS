#include <exec/types.h>
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

#include "iconcontainerclass.h"
#include "iconcontainerobserver.h"
#include "observer.h"

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
		{
			UBYTE *dir;
			struct TagItem *tag;

			tag=FindTagItem(ICOA_Directory, tags);
			if(tag)
			{
				dir=tag->ti_Data;
				tag->ti_Tag=TAG_IGNORE;
			}

			newObject=NewObjectA(IconContainer->mcc_Class, NULL, tags);

			semanticObject=NewObject(IconContainerObserver->mcc_Class, NULL,
						OA_Presentation, newObject,
						ICOA_Directory, dir, TAG_END);

			break;
		}

		case CDO_DiskIcon:
			newObject=NewObjectA(DiskIcon->mcc_Class, NULL, tags);

			semanticObject=NewObject(DiskIconObserver->mcc_Class, NULL,
						OA_Presentation, newObject,
						TAG_END);
			break;

		case CDO_DrawerIcon:
			newObject=NewObjectA(DrawerIcon->mcc_Class, NULL, tags);

			semanticObject=NewObject(DiskIconObserver->mcc_Class, NULL,
						OA_Presentation, newObject,
						TAG_END);
			break;

		case CDO_ToolIcon:
			newObject=NewObjectA(ToolIcon->mcc_Class, NULL, tags);

			semanticObject=NewObject(ToolIconObserver->mcc_Class, NULL,
						OA_Presentation, newObject,
						TAG_END);
			break;

		case CDO_ProjectIcon:
			newObject=NewObjectA(ProjectIcon->mcc_Class, NULL, tags);

			semanticObject=NewObject(ProjectIconObserver->mcc_Class, NULL,
						OA_Presentation, newObject,
						TAG_END);
			break;

		case CDO_TrashcanIcon:
			newObject=NewObjectA(TrashcanIcon->mcc_Class, NULL, tags);

			semanticObject=NewObject(TrashcanIconObserver->mcc_Class, NULL,
						OA_Presentation, newObject,
						TAG_END);
			break;
	}

	return newObject;

    AROS_LIBFUNC_EXIT
} /* CreateWorkbenchObjectA */


