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

#include <string.h>

#include "debug.h"

#include "mui.h"
#include "imspec.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_CycleData
{
    const char **entries;
    int entries_active;
    int entries_num;
    int entries_width;
    int entries_height;

    int cycle_width;
    int cycle_height;

    Object *pageobj;

    struct MUI_EventHandlerNode ehn;
    struct Hook     	    	pressedhook;

};

void PressedHookFunc(struct Hook *hook, Object *obj, APTR msg)
{
    struct MUI_CycleData    *data;
    Class   	    	    *cl = (Class *)hook->h_Data;
    LONG    	    	    act;

    data = INST_DATA(cl, obj);
    
    act = ++data->entries_active;
    if (act >= data->entries_num) act = 0;
    
    set(obj, MUIA_Cycle_Active, act);
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Cycle_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_CycleData    *data;
    struct TagItem  	    *tag, *tags;
    Object  	    	    *pageobj, *imgobj;
    int i;

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
        
        ButtonFrame,
        MUIA_Background,  MUII_ButtonBack,
        MUIA_InputMode,   MUIV_InputMode_RelVerify,
        MUIA_InnerTop,    1,
        MUIA_InnerBottom, 1,
        MUIA_Group_Horiz, TRUE,
        
        Child, (IPTR) imgobj = ImageObject,
            MUIA_InnerLeft,             2,
            MUIA_Image_Spec,     (IPTR) "6:17",
            MUIA_Image_FreeVert,        TRUE,
        End,
        Child, (IPTR) pageobj = PageGroup,
        End,
        
        TAG_MORE, (IPTR) msg->ops_AttrList
    );

    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);

    data->pageobj = pageobj;
    data->pressedhook.h_Entry = HookEntry;
    data->pressedhook.h_SubEntry = (HOOKFUNC)PressedHookFunc;
    data->pressedhook.h_Data = cl;
    
    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Cycle_Entries:
		    data->entries = (const char**)tag->ti_Data;
		    break;
		    
	    case    MUIA_Cycle_Active:
	    	    data->entries_active = tag->ti_Data;
		    break;
	}
    }

    if (!data->entries)
    {
	D(bug("Cycle_New: No Entries specified!\n"));
	CoerceMethod(cl,obj,OM_DISPOSE);
	return NULL;
    }

    /* Count the number of entries */
    for (i=0;data->entries[i];i++)
    {
    	Object *page;
	
	page = TextObject,
	    	    MUIA_Text_Contents, (IPTR)data->entries[i],
		    MUIA_Text_PreParse, (IPTR)"\033c",
		    End;
		    
    	if (!page)
	{
	    D(bug("Cycle_New: Could not create page object specified!\n"));
	    CoerceMethod(cl,obj,OM_DISPOSE);
	    return NULL;
	}
	
	DoMethod(pageobj, OM_ADDMEMBER, (IPTR)page);
    }
    data->entries_num = i;

    if ((data->entries_active >= 0) && (data->entries_active < data->entries_num))
    {
    	set(pageobj, MUIA_Group_ActivePage, data->entries_active);
    }
    else
    {
    	data->entries_active = 0;
    }
    
#if 1
    DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE,
    	     (IPTR)obj, 2, MUIM_CallHook, (IPTR)&data->pressedhook);
#else
    DoMethod(imgobj, MUIM_Notify, MUIA_Pressed, FALSE,
    	     (IPTR)obj, 3, MUIM_Set, MUIA_Cycle_Active, MUIV_Cycle_Active_Next);
#endif
         
    return (IPTR)obj;
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Cycle_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_CycleData    *data;
    struct TagItem  	    *tag, *tags;
    LONG    	    	    l;
    BOOL    	    	    noforward = TRUE;
    
    data = INST_DATA(cl, obj);
    
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Cycle_Active:
	    	l = (LONG)tag->ti_Data;

	    	if (l == MUIV_Cycle_Active_Next)
		{
		    l = data->entries_active + 1;
		    if (l >= data->entries_num) l = 0;
		}
		else if (l == MUIV_Cycle_Active_Prev)
		{
		    l = data->entries_active - 1;
		    if (l < 0) l = data->entries_num - 1;
		}

		if (l >= 0 && l < data->entries_num)
		{
		    data->entries_active = l;
		    set(data->pageobj, MUIA_Group_ActivePage, data->entries_active);
		}
		break;
		    
	    default:
	    	noforward = FALSE;
	    	break;
	}
    }
    
    if (noforward)
    {
    	struct opSet ops = *msg;
	struct TagItem tags[] =
	{
	    {MUIA_Group_Forward , FALSE },
	    {TAG_MORE	    	, 0       }	    
	};

	/* Zune must also be compilable with SAS C on Amiga */
	tags[1].ti_Data = (IPTR)msg->ops_AttrList;
	
	ops.ops_AttrList =  tags;
	
	return DoSuperMethodA(cl,obj,(Msg)&ops);
    }
    else
    {
    	return DoSuperMethodA(cl,obj,(Msg)msg);
    }
}

/**************************************************************************
 OM_GET
**************************************************************************/
static IPTR Cycle_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_CycleData *data = INST_DATA(cl, obj);
#define STORE *(msg->opg_Storage)

    switch(msg->opg_AttrID)
    {
	case	MUIA_Cycle_Active:
		STORE = data->entries_active;
		return 1;
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
STATIC IPTR Cycle_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_CycleData   *data;

    if (!(DoSuperMethodA(cl, obj, (Msg)msg)))
	return 0;

    data = INST_DATA(cl, obj);

    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 1;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
STATIC IPTR Cycle_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_CycleData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

    return DoSuperMethodA(cl,obj,(Msg)msg);
}


/**************************************************************************
 ...
**************************************************************************/
static ULONG Cycle_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    return DoSuperMethodA(cl, obj, (Msg)msg);
}


BOOPSI_DISPATCHER(IPTR, Cycle_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Cycle_New(cl, obj, (struct opSet *)msg);
	case OM_SET: return Cycle_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Cycle_Get(cl, obj, (struct opGet *)msg);
	case MUIM_Setup: return Cycle_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup: return Cycle_Cleanup(cl, obj, (APTR)msg);
	case MUIM_HandleEvent: return Cycle_HandleEvent(cl,obj,(APTR)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Cycle_desc = {
    MUIC_Cycle,
    MUIC_Group, 
    sizeof(struct MUI_CycleData), 
    (void*)Cycle_Dispatcher 
};
