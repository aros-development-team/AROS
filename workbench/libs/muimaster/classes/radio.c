/*
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

/* should avoid code duplication (width/height/image) with cycle gadget,
 * cleanup needed ! -dlc
 */

/*  #include <graphics/gfx.h> */
/*  #include <graphics/view.h> */
#include <clib/alib_protos.h>
#include <proto/exec.h>
/*  #include <proto/graphics.h> */
#include <proto/utility.h>
/*  #include <proto/intuition.h> */

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include <string.h>

#include "debug.h"

#include "mui.h"
/*  #include "imspec.h" */
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_RadioData
{
    const char **entries;
    int entries_active;
    int entries_num;
    Object **buttons;
};

#define MAX(a,b) (((a)>(b))?(a):(b))

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Radio_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_RadioData   *data;
    struct TagItem  	    *tag, *tags;
    int i;
    int j;
    const char **entries = NULL;
    int entries_active = 0;
    int entries_num;
    struct TagItem *grouptags;
    Object **buttons;
    int state;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags)); )
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
	return NULL;
    }

    if ((entries_active < 0) || (entries_active > entries_num - 1))
	entries_active = 0;

    /* Count the number of entries */
    for (i=0;entries[i];i++);

    entries_num = i;

    grouptags = AllocVec((1 + 2 * i) * sizeof(struct TagItem), MEMF_PUBLIC);
    if (!grouptags)
	return FALSE;
    buttons = AllocVec(i * sizeof(Object *), MEMF_PUBLIC);
    if (!buttons)
    {
	FreeVec(grouptags);
	return FALSE;
    }
    for (j=0,i=0;entries[i];i++)
    {
	state = (entries_active == i) ? TRUE : FALSE;
	grouptags[j].ti_Tag = MUIA_Group_Child;
	grouptags[j].ti_Data = (IPTR)buttons[i] = ImageObject,
	    MUIA_Image_Spec, MUII_RadioButton,
	    MUIA_ShowSelState, FALSE,
	    MUIA_Frame, MUIV_Frame_None,
	    MUIA_InputMode, MUIV_InputMode_Immediate,
	    MUIA_Selected, state,
	    End;
	j++;
	grouptags[j].ti_Tag = MUIA_Group_Child;
	grouptags[j].ti_Data = (IPTR)TextObject,
	    MUIA_Text_Contents, entries[i],
	    MUIA_FramePhantomHoriz, TRUE,
	    MUIA_Text_PreParse, "\33l",
	    End;
	j++;
    }

    grouptags[j].ti_Tag = TAG_MORE;
    grouptags[j].ti_Data = (IPTR)msg->ops_AttrList;

    obj = (Object *)DoSuperNew(cl, obj,
			       MUIA_Group_Columns, 2,
			       TAG_MORE, grouptags);
    FreeVec(grouptags);
    if (!obj)
    {
	FreeVec(buttons);
	return FALSE;
    }
    data = INST_DATA(cl, obj);
    data->entries = entries;
    data->entries_active = entries_active;
    data->entries_num = entries_num;
    data->buttons = buttons;

    for (i = 0; i < entries_num; i++)
    {
	DoMethod(data->buttons[i], MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
		 (IPTR)obj, 3, MUIM_Set, MUIA_Radio_Active, i);
    }

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Radio_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_RadioData   *data = INST_DATA(cl,obj);
    if (data->buttons) FreeVec(data->buttons);
    return DoSuperMethodA(cl,obj,msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Radio_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_RadioData *data;
    struct TagItem  	    *tag, *tags;

    data = INST_DATA(cl, obj);
    
    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Radio_Active:
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

/**************************************************************************
 OM_GET
**************************************************************************/
static IPTR Radio_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_RadioData *data = INST_DATA(cl, obj);
#define STORE *(msg->opg_Storage)

    switch(msg->opg_AttrID)
    {
	case	MUIA_Radio_Active:
		STORE = data->entries_active;
		return 1;
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

#ifndef _AROS
__asm IPTR Radio_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Radio_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Radio_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Radio_Dispose(cl, obj, (Msg)msg);
	case OM_SET: return Radio_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Radio_Get(cl, obj, (struct opGet *)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Radio_desc = {
    MUIC_Radio,
    MUIC_Group, 
    sizeof(struct MUI_RadioData), 
    (void*)Radio_Dispatcher 
};
