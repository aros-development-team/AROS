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
#include <proto/intuition.h>

#include <string.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "frame.h"

extern struct Library *MUIMasterBase;

struct MUI_FramedisplayData
{
    struct MUI_FrameSpec_intern fs_intern;
    char spec[8];
};


/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Framedisplay_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_FramedisplayData   *data;
    struct TagItem  	    *tag, *tags;
    
    D(bug("Framedisplay_New starts\n"));

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Framedisplay_Spec:		
		zune_frame_spec_to_intern((CONST_STRPTR)tag->ti_Data, &data->fs_intern);
	    break;
    	}
    }

    D(bug("Framedisplay_New(%lx) spec=%lx\n", obj, data->img));
    return (IPTR)obj;
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Framedisplay_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_FramedisplayData *data = INST_DATA(cl, obj);
    struct TagItem  	    *tag, *tags;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Framedisplay_Spec:
		zune_frame_spec_to_intern((CONST_STRPTR)tag->ti_Data, &data->fs_intern);
		MUI_Redraw(obj, MADF_DRAWOBJECT);
		break;
	}
    }
    
    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static IPTR Framedisplay_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_FramedisplayData *data = INST_DATA(cl, obj);
    switch (msg->opg_AttrID)
    {
	case MUIA_Framedisplay_Spec:
	    zune_frame_intern_to_spec(&data->fs_intern, (STRPTR)data->spec);
	    *msg->opg_Storage = data->spec;
	    return(TRUE);
    }

    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static IPTR Framedisplay_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl,obj,(Msg)msg);

    msg->MinMaxInfo->MinWidth += 8;
    msg->MinMaxInfo->MinHeight += 8;
   
    msg->MinMaxInfo->DefWidth += 30;
    msg->MinMaxInfo->DefHeight += 30;

    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return 1;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static IPTR Framedisplay_Draw(struct IClass *cl, Object *obj,struct MUIP_Draw *msg)
{
    struct MUI_FramedisplayData *data = INST_DATA(cl, obj);
    struct ZuneFrameGfx *zframe;
    APTR region;
    WORD ileft, itop, iright, ibottom;
    int i;

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (!(msg->flags & MADF_DRAWOBJECT))
	return 0;

    zframe = zune_zframe_get(&data->fs_intern);
    if (!zframe)
	return 0;
    zframe->draw[data->fs_intern.state](muiRenderInfo(obj), _mleft(obj), _mtop(obj),
					 _mwidth(obj), _mheight(obj));
    ileft = _mleft(obj) + zframe->xthickness + data->fs_intern.innerLeft;
    itop = _mtop(obj) + zframe->ythickness + data->fs_intern.innerTop;
    iright = _mright(obj) - zframe->xthickness - data->fs_intern.innerRight;
    ibottom = _mbottom(obj) - zframe->ythickness - data->fs_intern.innerBottom;

    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);

    region = MUI_AddClipping(muiRenderInfo(obj), ileft, itop,
			     iright - ileft + 1, ibottom - itop + 1);

    for (i = itop; i < ibottom + iright - ileft; i++)
    {
	if (!(i % 4))
	{
	    Move(_rp(obj), ileft, i);
	    Draw(_rp(obj), ileft + i - itop, itop);
	}
    }

    MUI_RemoveClipping(muiRenderInfo(obj), region);

    return 1;
}


BOOPSI_DISPATCHER(IPTR, Framedisplay_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Framedisplay_New(cl, obj, (struct opSet *)msg);
	case OM_SET: return Framedisplay_Set(cl, obj, (APTR)msg);
	case OM_GET: return Framedisplay_Get(cl, obj, (APTR)msg);
	case MUIM_AskMinMax: return Framedisplay_AskMinMax(cl,obj,(APTR)msg);
	case MUIM_Draw: return Framedisplay_Draw(cl,obj,(APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Framedisplay_desc = { 
    MUIC_Framedisplay, 
    MUIC_Area, 
    sizeof(struct MUI_FramedisplayData), 
    (void*)Framedisplay_Dispatcher 
};

