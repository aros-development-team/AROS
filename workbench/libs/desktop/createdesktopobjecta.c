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
#include "presentation.h"

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
        This function is sloppy - sort it out!

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
	struct TagItem *obsTags;

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

			SetAttrs(newObject, PA_Observer, semanticObject, TAG_END);

			break;
		}

		case CDO_DiskIcon:
		{
			struct TagItem *obsTags, *findTag;

			obsTags=AllocVec(sizeof(struct TagItem)*3, MEMF_ANY);
			findTag=FindTagItem(IA_Label, tags);

			newObject=NewObjectA(DiskIcon->mcc_Class, NULL, tags);

			obsTags[0].ti_Tag=IOA_Name;
			obsTags[0].ti_Data=findTag->ti_Data;
			obsTags[1].ti_Tag=OA_Presentation;
			obsTags[1].ti_Data=newObject;
			obsTags[2].ti_Tag=TAG_END;
			obsTags[2].ti_Data=0;

			semanticObject=NewObjectA(DiskIconObserver->mcc_Class, NULL, obsTags);
			break;
		}

		case CDO_DrawerIcon:
		{
			struct TagItem *obsTags, *findTag, *findTag2;

			obsTags=AllocVec(sizeof(struct TagItem)*4, MEMF_ANY);
			findTag=FindTagItem(IA_Label, tags);
			findTag2=FindTagItem(IOA_Directory, tags);

			newObject=NewObjectA(DrawerIcon->mcc_Class, NULL, tags);

			obsTags[0].ti_Tag=IOA_Name;
			obsTags[0].ti_Data=findTag->ti_Data;
			obsTags[1].ti_Tag=OA_Presentation;
			obsTags[1].ti_Data=newObject;
			obsTags[2].ti_Tag=IOA_Directory;
			obsTags[2].ti_Data=findTag2->ti_Data;
			obsTags[3].ti_Tag=TAG_END;
			obsTags[3].ti_Data=0;


			semanticObject=NewObjectA(DrawerIconObserver->mcc_Class, NULL, obsTags);
			break;
		}

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

		case CDO_Desktop:
			newObject=NewObjectA(IconContainer->mcc_Class, NULL, tags);

			semanticObject=NewObject(DesktopObserver->mcc_Class, NULL,
						OA_Presentation, newObject, TAG_END);

			break;

		case CDO_DirectoryWindow:
		{
			Class *windowClass;
			struct TagItem *windowArgs;
			Object *windowObject;

			if(DesktopBase->db_DefaultWindow)
				windowClass=DesktopBase->db_DefaultWindow;
			else
				windowClass=MUIC_Window;

//			if(DesktopBase->db_DefaultWindowArguments)
//				windowArgs=DesktopBase->db_DefaultWindowArguments;
//			else
//			{
				windowArgs=AllocVec(sizeof(struct TagItem)*4, MEMF_ANY);

				windowArgs[0].ti_Tag=MUIA_Window_UseBottomBorderScroller;
				windowArgs[0].ti_Data=TRUE;
				windowArgs[1].ti_Tag=MUIA_Window_UseRightBorderScroller;
				windowArgs[1].ti_Data=TRUE;
				windowArgs[2].ti_Tag=WindowContents;
				windowArgs[2].ti_Data=CreateDesktopObjectA(CDO_IconContainer, tags);
				windowArgs[3].ti_Tag=TAG_END;
				windowArgs[3].ti_Data=0;
//			}

			windowObject=NewObjectA(windowClass, NULL, windowArgs);

			break;
		}
	}

	return newObject;

    AROS_LIBFUNC_EXIT
} /* CreateWorkbenchObjectA */


