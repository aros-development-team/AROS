/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "frame.h"
#include "framedisplay_private.h"

extern struct Library *MUIMasterBase;


IPTR Framedisplay__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Framedisplay_DATA   *data;
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

IPTR Framedisplay__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Framedisplay_DATA *data = INST_DATA(cl, obj);
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

IPTR Framedisplay__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Framedisplay_DATA *data = INST_DATA(cl, obj);
    switch (msg->opg_AttrID)
    {
	case MUIA_Framedisplay_Spec:
	    zune_frame_intern_to_spec(&data->fs_intern, (STRPTR)data->spec);
	    *msg->opg_Storage = (IPTR)data->spec;
	    return(TRUE);
    }

    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

IPTR Framedisplay__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl,obj,(Msg)msg);

    msg->MinMaxInfo->MinWidth += 8;
    msg->MinMaxInfo->MinHeight += 8;
   
    msg->MinMaxInfo->DefWidth += 16;
    msg->MinMaxInfo->DefHeight += 16;

    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return 1;
}

IPTR Framedisplay__MUIM_Draw(struct IClass *cl, Object *obj,struct MUIP_Draw *msg)
{
    struct Framedisplay_DATA *data = INST_DATA(cl, obj);
    const struct ZuneFrameGfx *zframe;
    APTR region;
    WORD ileft, itop, iright, ibottom;
    int i;

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (!(msg->flags & MADF_DRAWOBJECT))
	return 0;

    zframe = zune_zframe_get(&data->fs_intern);
    if (!zframe)
	return 0;
    zframe->draw(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
		 _mwidth(obj), _mheight(obj));
    ileft = _mleft(obj) + zframe->ileft + data->fs_intern.innerLeft;
    itop = _mtop(obj) + zframe->itop + data->fs_intern.innerTop;
    iright = _mright(obj) - zframe->iright - data->fs_intern.innerRight;
    ibottom = _mbottom(obj) - zframe->ibottom - data->fs_intern.innerBottom;

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

IPTR Framedisplay__MUIM_DragQuery(struct IClass *cl, Object *obj, struct MUIP_DragQuery *msg)
{
    struct MUI_FrameSpec *dummy;

    if (msg->obj == obj)
	return MUIV_DragQuery_Refuse;
    if (!get(msg->obj, MUIA_Framedisplay_Spec, &dummy))
	return MUIV_DragQuery_Refuse;
    return MUIV_DragQuery_Accept;
}

IPTR Framedisplay__MUIM_DragDrop(struct IClass *cl, Object *obj, struct MUIP_DragDrop *msg)
{
    struct MUI_FrameSpec *spec;

    get(msg->obj, MUIA_Framedisplay_Spec, &spec);
    set(obj, MUIA_Framedisplay_Spec, (IPTR)spec);
    return 0;
}


#if ZUNE_BUILTIN_FRAMEDISPLAY
BOOPSI_DISPATCHER(IPTR, Framedisplay_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:         return Framedisplay__OM_NEW(cl, obj, (struct opSet *)msg);
	case OM_SET:         return Framedisplay__OM_SET(cl, obj, (APTR)msg);
	case OM_GET:         return Framedisplay__OM_GET(cl, obj, (APTR)msg);
	case MUIM_AskMinMax: return Framedisplay__MUIM_AskMinMax(cl,obj,(APTR)msg);
	case MUIM_Draw:      return Framedisplay__MUIM_Draw(cl,obj,(APTR)msg);
	case MUIM_DragQuery: return Framedisplay__MUIM_DragQuery(cl,obj,(APTR)msg);
	case MUIM_DragDrop:  return Framedisplay__MUIM_DragDrop(cl,obj,(APTR)msg);
        default:             return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Framedisplay_desc =
{ 
    MUIC_Framedisplay, 
    MUIC_Area, 
    sizeof(struct Framedisplay_DATA), 
    (void*)Framedisplay_Dispatcher 
};
#endif /* ZUNE_BUILTIN_FRAMEDISPLAY */
