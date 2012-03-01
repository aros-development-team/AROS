/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

/*  #define MYDEBUG 1 */
#include "debug.h"

#include "mui.h"
#include "muimaster_intern.h"
#include "prefs.h"
#include "support.h"
#include "support_classes.h"
#include "radio_private.h"

extern struct Library *MUIMasterBase;

IPTR Radio__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Radio_DATA   *data;
    struct TagItem      *tags;
    struct TagItem  	    *tag;
    int i;
    const char **entries = NULL;
    int entries_active = 0;
    int entries_num;
    struct TagItem *grouptags;
    Object **buttons, **imgobjs, **textobjs;
    int state;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Radio_Entries:
		    entries = (const char**)tag->ti_Data;
		    break;
	    case    MUIA_Radio_Active:
		    entries_active = tag->ti_Data;
		    break;
	}
    }

    if (!entries)
    {
	D(bug("Radio_New: No Entries specified!\n"));
	CoerceMethod(cl,obj,OM_DISPOSE);
	return 0;
    }

    /* Count the number of entries */
    for (i=0;entries[i];i++);

    entries_num = i;

    if ((entries_active < 0) || (entries_active > entries_num - 1))
	entries_active = 0;

    grouptags = AllocateTagItems(entries_num + 1);
    if (!grouptags)
	return FALSE;
    buttons = AllocVec(i * sizeof(Object *) * 3, MEMF_PUBLIC);
    if (!buttons)
    {
	FreeVec(grouptags);
	return FALSE;
    }
    imgobjs = buttons + i;
    textobjs = imgobjs + i;
    
    for (i = 0; i < entries_num; i++)
    {
	state = (entries_active == i) ? TRUE : FALSE;

	buttons[i] = HGroup,
	    Child, (IPTR)(imgobjs[i] = ImageObject,
	        MUIA_Image_FontMatch, TRUE,
	        MUIA_InputMode, MUIV_InputMode_Immediate,
	        MUIA_Selected, state,
	        MUIA_ShowSelState, FALSE,
	        MUIA_Image_Spec, MUII_RadioButton,
	        MUIA_Frame, MUIV_Frame_None,
   	        End),
	    Child, (IPTR)(textobjs[i] = TextObject,
	        MUIA_InputMode, MUIV_InputMode_Immediate,
                MUIA_ShowSelState, FALSE,
	        MUIA_Selected, state,
	        MUIA_Text_Contents, entries[i],
	        MUIA_Frame, MUIV_Frame_None,
	        MUIA_Text_PreParse, (IPTR)"\33l",
	        End),
	    End;
        
	grouptags[i].ti_Tag = MUIA_Group_Child;
        grouptags[i].ti_Data = (IPTR)buttons[i];
    }

    grouptags[i].ti_Tag = TAG_MORE;
    grouptags[i].ti_Data = (IPTR)msg->ops_AttrList;

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
			       TAG_MORE, (IPTR)grouptags);
    FreeTagItems(grouptags);
    if (!obj)
    {
	FreeVec(buttons);
	return FALSE;
    }
    data = INST_DATA(cl, obj);
    data->entries_active = entries_active;
    data->entries_num = entries_num;
    data->buttons = buttons;

    for (i = 0; i < entries_num; i++)
    {
	DoMethod(imgobjs[i], MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
		 (IPTR)obj, 3, MUIM_Set, MUIA_Radio_Active, i);
	DoMethod(textobjs[i], MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
		 (IPTR)obj, 3, MUIM_Set, MUIA_Radio_Active, i);
    }

    return (IPTR)obj;
}

IPTR Radio__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Radio_DATA   *data = INST_DATA(cl,obj);
    if (data->buttons) FreeVec(data->buttons);
    return DoSuperMethodA(cl,obj,msg);
}

IPTR Radio__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Radio_DATA *data;
    struct TagItem    *tags;
    struct TagItem  	    *tag;

    data = INST_DATA(cl, obj);
    
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Radio_Active:
		D(bug("Radio_Set(%p) MUIA_Radio_Active %ld\n",
		      obj, tag->ti_Data));
		    if (tag->ti_Data >= 0 && tag->ti_Data < data->entries_num &&
			tag->ti_Data != data->entries_active)
		    {
			DoMethod(data->buttons[data->entries_active],
				 MUIM_NoNotifySet, MUIA_Selected, FALSE);
			DoMethod(data->buttons[tag->ti_Data],
				 MUIM_NoNotifySet, MUIA_Selected, TRUE);
			data->entries_active = tag->ti_Data;
		    }
		    break;
	}
    }
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

IPTR Radio__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Radio_DATA *data = INST_DATA(cl, obj);
#define STORE *(msg->opg_Storage)

    switch(msg->opg_AttrID)
    {
	case	MUIA_Radio_Active:
		STORE = data->entries_active;
		return 1;
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

IPTR Radio__MUIM_Setup(struct IClass *cl, Object *obj, Msg msg)
{
    if (!DoSuperMethodA(cl, obj, msg))
	return FALSE;

    set(obj, MUIA_Group_HorizSpacing, muiGlobalInfo(obj)->mgi_Prefs->radiobutton_hspacing);
    set(obj, MUIA_Group_VertSpacing, muiGlobalInfo(obj)->mgi_Prefs->radiobutton_vspacing);

    return TRUE;
}

/**************************************************************************
 MUIM_Export - to export an objects "contents" to a dataspace object.
**************************************************************************/
IPTR Radio__MUIM_Export(struct IClass *cl, Object *obj, struct MUIP_Export *msg)
{
    struct Radio_DATA *data = INST_DATA(cl, obj);
    ULONG id;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
	LONG value = data->entries_active;
	DoMethod(msg->dataspace, MUIM_Dataspace_Add, 
		(IPTR) &value, 
		sizeof(value), 
		(IPTR) id);
    }
    return 0;
}

/**************************************************************************
 MUIM_Import - to import an objects "contents" from a dataspace object.
**************************************************************************/
IPTR Radio__MUIM_Import(struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
    ULONG id;
    LONG *s;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
	if ((s = (LONG*) DoMethod(msg->dataspace, MUIM_Dataspace_Find, (IPTR) id)))
	{
	    set(obj, MUIA_Radio_Active, *s);
	}
    }
    return 0;
}

#if ZUNE_BUILTIN_RADIO
BOOPSI_DISPATCHER(IPTR, Radio_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:      return Radio__OM_NEW(cl, obj, (struct opSet *) msg);
        case OM_DISPOSE:  return Radio__OM_DISPOSE(cl, obj, (Msg) msg);
        case OM_SET:      return Radio__OM_SET(cl, obj, (struct opSet *) msg);
        case OM_GET:      return Radio__OM_GET(cl, obj, (struct opGet *) msg);
        case MUIM_Setup:  return Radio__MUIM_Setup(cl, obj, msg);
        case MUIM_Export: return Radio__MUIM_Export(cl, obj, (Msg) msg);
        case MUIM_Import: return Radio__MUIM_Import(cl, obj, (Msg) msg);
        default:          return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Radio_desc = {
    MUIC_Radio,
    MUIC_Group, 
    sizeof(struct Radio_DATA), 
    (void*)Radio_Dispatcher 
};
#endif /* ZUNE_BUILTIN_RADIO */
