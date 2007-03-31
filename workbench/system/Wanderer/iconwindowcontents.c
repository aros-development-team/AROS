/*
    Copyright  2004-2006, The AROS Development Team. All rights reserved.
    $Id: iconwindowcontents.c 25432 2007-03-14 18:05:52Z NicJA $
*/

#define DEBUG 0

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>

#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>
#include <proto/exec.h>
#include <proto/datatypes.h>

#include <dos/dos.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

#include <intuition/screens.h>
#include <datatypes/pictureclass.h>
#include <clib/macros.h>

#include "wanderer.h"
#include "wandererprefs.h"
#include "iconwindow.h"
#include "iconwindowcontents.h"

#include <aros/debug.h>

/*** Instance Data **********************************************************/

struct IconWindowContents_DATA
{
	struct  MUI_EventHandlerNode iwcd_EventHandlerNode;
	Object 									    *iwcd_IconList;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct IconWindowContents_DATA *data = INST_DATA(CLASS, self)

/*** Hook functions *********************************************************/

/*** Methods ****************************************************************/

Object *IconWindowContents__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object                          *iconList = NULL;

D(bug("[iconwindowcontents] IconWindowContents__OM_NEW()\n"));
	
    iconList = (Object *)GetTagData(MUIA_IconListview_IconList, 0, message->ops_AttrList);

D(bug("[iconwindowcontents] IconWindowContents__OM_NEW:  IconList @ %x\n", iconList));
	
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
		Child, (IPTR) IconListviewObject,
			MUIA_IconListview_UseWinBorder,        TRUE,
			MUIA_IconListview_IconList,     (IPTR) iconList,
		End,
        
        TAG_MORE, (IPTR) message->ops_AttrList
    );
    
    if (self != NULL)
    {
        SETUP_INST_DATA;
		data->iwcd_IconList = iconList;
    }
        
    return self;
}

IPTR IconWindowContents__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;
    struct TagItem *tstate = message->ops_AttrList, *tag;

    return DoSuperMethodA(CLASS, self, (Msg) message);
}


IPTR IconWindowContents__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_INST_DATA;
    IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;
    
    switch (message->opg_AttrID)
    {
        default:
            rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }
    
    return rv;
}

IPTR IconWindowContents__MUIM_Setup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    if (!DoSuperMethodA(CLASS, self, message)) return FALSE;

	if ((BOOL)XGET(_win(self), MUIA_IconWindow_IsRoot))
	{
		if (muiRenderInfo(self))
		{
D(bug("[iconwindowcontents] IconWindowContents__MUIM_Window_Setup: Setting up EventHandler for (IDCMP_DISKINSERTED | IDCMP_DISKREMOVED)\n"));
		
			data->iwcd_EventHandlerNode.ehn_Priority = 1;
			data->iwcd_EventHandlerNode.ehn_Flags    = MUI_EHF_GUIMODE;
			data->iwcd_EventHandlerNode.ehn_Object   = self;
			data->iwcd_EventHandlerNode.ehn_Class    = CLASS;
			data->iwcd_EventHandlerNode.ehn_Events   = IDCMP_DISKINSERTED | IDCMP_DISKREMOVED;

			DoMethod(_win(self), MUIM_Window_AddEventHandler, &data->iwcd_EventHandlerNode);
		}
		else
		{
D(bug("[iconwindowcontents] IconWindowContents__MUIM_Window_Setup: Couldnt add IDCMP EventHandler!\n"));
		}
	}

D(bug("[iconwindowcontents] IconWindowContents__MUIM_Window_Setup: Setup complete!\n"));
	
    return TRUE;
}

IPTR IconWindowContents__MUIM_Cleanup
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

	if ((BOOL)XGET(_win(self), MUIA_IconWindow_IsRoot))
	{
	  DoMethod(_win(self), MUIM_Window_RemEventHandler, &data->iwcd_EventHandlerNode);
	}

    return DoSuperMethodA(CLASS, self, message);
}

IPTR IconWindowContents__MUIM_HandleEvent
(
    Class *CLASS, Object *self, struct MUIP_HandleEvent *message
)
{
    SETUP_INST_DATA;

D(bug("[IconWindowContents] IconWindowContents__MUIM_HandleEvent()\n"));
    struct IntuiMessage *imsg = message->imsg;
		
    if(imsg->Class == IDCMP_DISKINSERTED) 
	{
D(bug("[IconWindowContents] IconWindowContents__MUIM_HandleEvent: IDCMP_DISKINSERTED\n"));
		DoMethod(data->iwcd_IconList, MUIM_IconList_Update);
		return(MUI_EventHandlerRC_Eat);
	}
	else if (imsg->Class == IDCMP_DISKREMOVED) 
	{
D(bug("[IconWindowContents] IconWindowContents__MUIM_HandleEvent: IDCMP_DISKREMOVED\n"));
		DoMethod(data->iwcd_IconList, MUIM_IconList_Update);
		return(MUI_EventHandlerRC_Eat);
	}
    return 0;
}

/*** Setup ******************************************************************/
ICONWINDOWCONTENTS_CUSTOMCLASS
(
    IconWindowContents, NULL, MUIC_Virtgroup, NULL,
    OM_NEW,                        struct opSet *,
    OM_SET,                        struct opSet *,
    OM_GET,                        struct opGet *,
    MUIM_Setup,                          Msg,
    MUIM_Cleanup,                      Msg,
	MUIM_HandleEvent,                Msg
);
