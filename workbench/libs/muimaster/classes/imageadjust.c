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

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_ImageadjustData
{
    Object *image;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Imageadjust_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ImageadjustData   *data;
    struct TagItem  	    *tag, *tags;
    Object *image;

    obj = (Object *)DoSuperNew(cl, obj,
			ButtonFrame,
			MUIA_InputMode, MUIV_InputMode_RelVerify,
    			MUIA_Group_Horiz, TRUE,
    			MUIA_Group_Spacing, 0,
    			Child, image = ImageObject, End,
			TAG_MORE, msg->ops_AttrList);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    data->image = image;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
    	}
    }

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
STATIC IPTR Imageadjust_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    DoSuperMethodA(cl,obj,msg);
    return 0;
}

/**************************************************************************
 OM_SET
**************************************************************************/
STATIC IPTR Imageadjust_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tags,*tag;
    struct MUI_ImageadjustData *data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
 	}
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static IPTR Imageadjust_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_GroupData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    }

    /* our handler didn't understand the attribute, we simply pass
    ** it to our superclass now
    */
    if (DoSuperMethodA(cl, obj, (Msg) msg)) return 1;

    return 0;
#undef STORE
}



#ifndef _AROS
__asm IPTR Imageadjust_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Imageadjust_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Imageadjust_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Imageadjust_Dispose(cl,obj,(APTR)msg);
	case OM_SET: return Imageadjust_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Imageadjust_Get(cl,obj,(APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Imageadjust_desc = { 
    MUIC_Imageadjust, 
    MUIC_Group, 
    sizeof(struct MUI_ImageadjustData), 
    (void*)Imageadjust_Dispatcher 
};

