/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

#define MENUF_CHECKED        (1<<0)
#define MENUF_CHECKIT        (1<<1)
#define MENUF_COMMANDSTRING  (1<<2)
#define MENUF_ENABLED        (1<<3)
#define MENUF_TOGGLE         (1<<4)

struct MUI_MenuitemData
{
    ULONG flags;
    ULONG exclude;

    char *shortcut;
    char *title;

    struct NewMenu *newmenu;

    struct MenuItem *trigger;
};

static int Menuitem_GetTotalChildren(Object *obj)
{
    Object                *cstate;
    Object                *child;
    struct MinList        *ChildList;
    int num = 0;

    get(obj, MUIA_Family_List, (ULONG *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
	num++;
	num += Menuitem_GetTotalChildren(child);
    }
    return num;
}

static int Menuitem_FillNewMenu(Object *obj, struct NewMenu *menu, int depth)
{
    Object                *cstate;
    Object                *child;
    struct MinList        *ChildList;
    int num = 0;

    if (depth > 2) return 0;

    get(obj, MUIA_Family_List, (ULONG *)&(ChildList));
    cstate = (Object *)ChildList->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
    	int entries;
    	ULONG checkit = 0, checked = 0, toggle = 0;

    	if (depth == 0) menu->nm_Type = NM_TITLE;
    	else if (depth == 1) menu->nm_Type = NM_ITEM;
    	else if (depth == 2) menu->nm_Type = NM_SUB;
    	get(child, MUIA_Menuitem_Title, &menu->nm_Label);
    	get(child, MUIA_Menuitem_Shortcut, &menu->nm_CommKey);
    	get(child, MUIA_Menuitem_Checkit, &checkit);
    	get(child, MUIA_Menuitem_Checked, &checked);
    	get(child, MUIA_Menuitem_Toggle, &toggle);
    	if (checkit) menu->nm_Flags |= CHECKIT;
    	if (checked) menu->nm_Flags |= CHECKED;
    	if (toggle) menu->nm_Flags |= MENUTOGGLE;
    	get(child, MUIA_Menuitem_Exclude, &menu->nm_MutualExclude);
	menu->nm_UserData = child;

	menu++;
	num++;
	entries = Menuitem_FillNewMenu(child,menu,depth+1);

	menu += entries;
	num += entries;
    }
    return num;
}

/**************************************************************************
 ...
**************************************************************************/
static struct NewMenu *Menuitem_BuildNewMenu(struct MUI_MenuitemData *data, Object *obj)
{
    int entries = Menuitem_GetTotalChildren(obj);
    if (data->newmenu) FreeVec(data->newmenu);
    data->newmenu = NULL;
    if (!entries) return NULL;

    if ((data->newmenu = (struct NewMenu*)AllocVec((entries+1)*sizeof(struct NewMenu),MEMF_CLEAR)))
    {
	Menuitem_FillNewMenu(obj,data->newmenu,0);
    }
    return data->newmenu;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Menuitem_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_MenuitemData *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg); /* We need no tags */ 
    if (!obj) return NULL;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem((struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case  MUIA_Menuitem_Checked:
		  _handle_bool_tag(data->flags, tag->ti_Data, MENUF_CHECKED);
		  break;

	    case  MUIA_Menuitem_Checkit:
		  _handle_bool_tag(data->flags, tag->ti_Data, MENUF_CHECKIT);
		  break;

	    case  MUIA_Menuitem_CommandString:
		  _handle_bool_tag(data->flags, tag->ti_Data, MENUF_COMMANDSTRING);
		  break;

	    case  MUIA_Menu_Enabled:
	    case  MUIA_Menuitem_Enabled:
		  _handle_bool_tag(data->flags, tag->ti_Data, MENUF_ENABLED);
		  break;

	    case  MUIA_Menuitem_Toggle:
		  _handle_bool_tag(data->flags, tag->ti_Data, MENUF_TOGGLE);
		  break;

	    case  MUIA_Menuitem_Exclude:
		  data->exclude = tag->ti_Data;
		  break;

	    case  MUIA_Menuitem_Shortcut:
		  data->shortcut = (char*)tag->ti_Data;
		  break;

	    case  MUIA_Menu_Title:
	    case  MUIA_Menuitem_Title:
		  data->title = (char*)tag->ti_Data;
		  break;
	}
    }

    return (ULONG)obj;
}

/**************************************************************************
 OM_SET
**************************************************************************/
STATIC ULONG Menuitem_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_MenuitemData *data;
    struct TagItem *tags,*tag;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem((struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case  MUIA_Menuitem_Checked:
		  _handle_bool_tag(data->flags, tag->ti_Data, MENUF_CHECKED);
		  break;

	    case  MUIA_Menuitem_Checkit:
		  _handle_bool_tag(data->flags, tag->ti_Data, MENUF_CHECKIT);
		  break;

	    case  MUIA_Menuitem_CommandString:
		  _handle_bool_tag(data->flags, tag->ti_Data, MENUF_COMMANDSTRING);
		  break;

	    case  MUIA_Menu_Enabled:
	    case  MUIA_Menuitem_Enabled:
		  _handle_bool_tag(data->flags, tag->ti_Data, MENUF_ENABLED);
		  tag->ti_Tag = TAG_IGNORE;
		  break;

	    case  MUIA_Menuitem_Toggle:
		  _handle_bool_tag(data->flags, tag->ti_Data, MENUF_TOGGLE);
		  break;

	    case  MUIA_Menuitem_Exclude:
		  data->exclude = tag->ti_Data;
		  break;

	    case  MUIA_Menuitem_Shortcut:
		  data->shortcut = (char*)tag->ti_Data;
		  break;

	    case  MUIA_Menu_Title:
	    case  MUIA_Menuitem_Title:
		  data->title = (char*)tag->ti_Data;
		  tag->ti_Tag = TAG_IGNORE;
		  break;

	    case  MUIA_Menuitem_Trigger:
	    	  data->trigger = (struct MenuItem*)tag->ti_Data;
		  break;
	}
    }

    return (ULONG)DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
#define STORE *(msg->opg_Storage)
STATIC ULONG Menuitem_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_MenuitemData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
	case MUIA_Menuitem_Checked:
	     STORE = ((data->flags & MENUF_CHECKED) != 0);
	     return 1;

	case MUIA_Menuitem_Checkit:
	     STORE = ((data->flags & MENUF_CHECKIT) != 0);
	     return 1;

	case MUIA_Menuitem_CommandString:
	     STORE = ((data->flags & MENUF_COMMANDSTRING) != 0);
	     return 1;

	case MUIA_Menu_Enabled:
	case MUIA_Menuitem_Enabled:
	     STORE = ((data->flags & MENUF_ENABLED) != 0);
	     return 1;

	case MUIA_Menuitem_Toggle:
	     STORE = ((data->flags & MENUF_TOGGLE) != 0);
	     return 1;

	case MUIA_Menuitem_Exclude:
	     STORE = data->exclude;
	     return 1;

	case MUIA_Menuitem_Shortcut:
	     STORE = (ULONG)data->shortcut;
	     return 1;

	case MUIA_Menu_Title:
	case MUIA_Menuitem_Title:
	     STORE = (ULONG)data->title;
	     return 1;

	case MUIA_Menuitem_NewMenu:
	     Menuitem_BuildNewMenu(data,obj);
	     STORE = (ULONG)data->newmenu;
	     return 1;

	case MUIA_Menuitem_Trigger:
	     STORE = (ULONG)data->trigger;
	     return 1;
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}
#undef STORE


#ifndef _AROS
__asm IPTR Menuitem_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Menuitem_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Menuitem_New(cl, obj, (struct opSet *) msg);
	case OM_SET: return Menuitem_Set(cl, obj, (struct opSet *) msg);
	case OM_GET: return Menuitem_Get(cl, obj, (struct opGet *) msg);

    }
    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Menuitem_desc = { 
    MUIC_Menuitem, 
    MUIC_Family, 
    sizeof(struct MUI_MenuitemData), 
    (void*)Menuitem_Dispatcher 
};

/*
 * Class descriptor.- this class is the same like menuitem
 */
const struct __MUIBuiltinClass _MUI_Menu_desc = { 
    MUIC_Menu, 
    MUIC_Family, 
    sizeof(struct MUI_MenuitemData), 
    (void*)Menuitem_Dispatcher 
};

/*
 * Class descriptor.- this class is the same like menuitem
 */
const struct __MUIBuiltinClass _MUI_Menustrip_desc = { 
    MUIC_Menustrip, 
    MUIC_Family, 
    sizeof(struct MUI_MenuitemData), 
    (void*)Menuitem_Dispatcher 
};
