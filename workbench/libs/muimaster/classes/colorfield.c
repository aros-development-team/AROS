/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdio.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <string.h>

#ifdef __AROS__
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "textengine.h"
#include "support.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

struct MUI_ColorfieldData
{
    UBYTE dummy;
    
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Colorfield_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ColorfieldData   *data;
    struct TagItem  	    *tag, *tags;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
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

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Colorfield_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ColorfieldData   *data = INST_DATA(cl,obj);

    return DoSuperMethodA(cl,obj,msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Colorfield_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ColorfieldData   *data;
    struct TagItem  	    *tag, *tags;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
    	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG  Colorfield_Get(struct IClass *cl, Object * obj, struct opGet *msg)
{
    struct MUI_ColorfieldData *data = INST_DATA(cl, obj);
    ULONG *store               = msg->opg_Storage;
    ULONG    tag               = msg->opg_AttrID;

    switch (tag)
    {
    	default:
	    return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    return TRUE;
}


/**************************************************************************
 MUIM_Setup
**************************************************************************/
static IPTR Colorfield_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_ColorfieldData *data = INST_DATA(cl,obj);

    if (!(DoSuperMethodA(cl, obj, (Msg)msg))) return 0;

    return 1;
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static IPTR Colorfield_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_ColorfieldData *data = INST_DATA(cl,obj);

    DoSuperMethodA(cl,obj,(Msg)msg);

    msg->MinMaxInfo->MinWidth  += 8;
    msg->MinMaxInfo->MinHeight += 8;
    msg->MinMaxInfo->DefWidth  += 8;
    msg->MinMaxInfo->DefHeight += 8;
    msg->MinMaxInfo->MaxWidth  += 8;
    msg->MinMaxInfo->MaxHeight += 8;
	
    return 0;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static IPTR Colorfield_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_ColorfieldData *data = INST_DATA(cl,obj);
    //ULONG val;

    DoSuperMethodA(cl,obj,(Msg)msg);

    return 0;
}


BOOPSI_DISPATCHER(IPTR, Colorfield_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Colorfield_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Colorfield_Dispose(cl, obj, (Msg)msg);
	case OM_SET: return Colorfield_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Colorfield_Get(cl, obj, (struct opGet *)msg);
	case MUIM_Setup: return Colorfield_Setup(cl, obj, (struct MUIP_Setup *)msg);
	case MUIM_AskMinMax: return Colorfield_AskMinMax(cl, obj, (struct MUIP_AskMinMax*)msg);
	case MUIM_Draw: return Colorfield_Draw(cl, obj, (struct MUIP_Draw*)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Colorfield_desc = { 
    MUIC_Colorfield, 
    MUIC_Area, 
    sizeof(struct MUI_ColorfieldData), 
    (void*)Colorfield_Dispatcher 
};

