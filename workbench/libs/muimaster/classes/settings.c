/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_SettingsData
{
    int dummy;
};

static IPTR ListDisplayFunc(struct Hook *hook, char **array, char *entry)
{
    *array++ = "";
    *array++ = "";
    *array = entry;

    return 0;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Settings_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_Settings *data;
    struct TagItem  	    *tag, *tags;
    Object *listobj;
    static const struct Hook list_display_hook = { {NULL, NULL}, HookEntry,
						   ListDisplayFunc, NULL };
    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
			         Child, HGroup,
			           Child, VGroup,
			             Child, ListviewObject,
			               MUIA_Listview_List, listobj = ListObject,
			                 InputListFrame,
			                 MUIA_List_AdjustWidth, TRUE,
			                 MUIA_List_Format, "DELTA=2,DELTA=5,",
			                 MUIA_List_DisplayHook, &list_display_hook,
			                 End, /* ListObject */
			               End, /* ListviewObject */
			             Child, HGroup,
			               Child, MUI_NewObject(MUIC_Popimage,
							    MUIA_FixHeight, 20,
						    MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_All,
						    TAG_DONE), /* Popframe really */
			               Child, MUI_NewObject(MUIC_Popimage,
							    MUIA_FixHeight, 20,
						    MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_All,
						    TAG_DONE),
			               End, /* HGroup */
			             End, /* VGroup */
			       Child, MUI_NewObject(MUIC_Settingsgroup,
						    TAG_DONE),
			           End, /* HGroup */
			       
			TAG_MORE, msg->ops_AttrList);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
    	}
    }

    return (IPTR)obj;
}

BOOPSI_DISPATCHER(IPTR, Settings_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:
	    return Settings_New(cl, obj, (struct opSet *)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Settings_desc = { 
    MUIC_Settings,
    MUIC_Group, 
    sizeof(struct MUI_SettingsData), 
    (void*)Settings_Dispatcher 
};

