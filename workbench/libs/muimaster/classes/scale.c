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

#include "mui.h"
#include "muimaster_intern.h"
#include "textengine.h"
#include "support.h"
#include "support_classes.h"
#include "scale_private.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;


IPTR Scale__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Scale_DATA   *data;
    struct TagItem  	    *tag, *tags;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    data->horiz = TRUE;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Scale_Horiz:
		    data->horiz = tag->ti_Data;
		    break;
    	}
    }

    D(bug("muimaster.library/scale.c: Scale Object created at 0x%lx\n",obj));

    return (IPTR)obj;
}

IPTR Scale__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Scale_DATA   *data;
    struct TagItem  	   *tag, *tags;
    int need_redraw = 0;
    
    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Scale_Horiz:
		if (data->horiz != tag->ti_Data)
		    need_redraw = 1;
		data->horiz = tag->ti_Data;
		break;
    	}
    }

    if (need_redraw)
    {
    	MUI_Redraw(obj,MADF_DRAWOBJECT);
    }
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Scale__OM_GET(struct IClass *cl, Object * obj, struct opGet *msg)
{
    struct Scale_DATA *data = INST_DATA(cl, obj);
    ULONG *store = msg->opg_Storage;
    ULONG    tag = msg->opg_AttrID;

    switch (tag)
    {
    case MUIA_Scale_Horiz:
	*store = (ULONG)data->horiz;
	return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Scale__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Scale_DATA *data = INST_DATA(cl,obj);

    if (!(DoSuperMethodA(cl, obj, (Msg)msg))) return 0;

    {
	char *minlabel = "0% 100%";
	struct RastPort rp;

	InitRastPort(&rp);
	SetFont(&rp,_font(obj));

	data->label_minwidth = TextLength(&rp,minlabel,strlen(minlabel));
	data->label_height = _font(obj)->tf_YSize;

	DeinitRastPort(&rp);
    }

    return 1;
}

IPTR Scale__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct Scale_DATA *data = INST_DATA(cl,obj);
    DoSuperMethodA(cl,obj,(Msg)msg);

    if (data->horiz)
    {
	msg->MinMaxInfo->MinWidth += data->label_minwidth;
	msg->MinMaxInfo->MinHeight += data->label_height + 4;
	msg->MinMaxInfo->DefWidth += data->label_minwidth;
	msg->MinMaxInfo->DefHeight += data->label_height + 4;
	msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight += data->label_height + 4;
    }
    else
    {
    }
    return 0;
}

IPTR Scale__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Scale_DATA *data = INST_DATA(cl,obj);
    //ULONG val;

    DoSuperMethodA(cl,obj,(Msg)msg);
    D(bug("muimaster.library/scale.c: Draw Scale Object at 0x%lx %ldx%ldx%ldx%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj)));

    if (data->horiz)
    {
	int  i;
	//int  subdiv;
	char buf[255];
	int  k;
	BOOL drawpct;

	/* main horizontal bar */
    	SetAPen(_rp(obj),_pens(obj)[MPEN_TEXT]);
	Move(_rp(obj), _mleft(obj), _mtop(obj) + 3);
	Draw(_rp(obj), _mright(obj), _mtop(obj) + 3);

	/* testing possible layouts: 1 = 0...100 ; 2 = 0...50..100 ; etc ... */
	for (i = 2; i < _mwidth(obj); i *= 2)
	{
	    int j;
	    int total_width = 0;
	    int too_big = 0;

	    for (j = 0; j <= i; j++)
	    {
		//int pct = j * 100 / i;

		snprintf(buf, 255, "%d%%", j);
		total_width += TextLength(_rp(obj),buf,strlen(buf));
		if (total_width > (3 * _mwidth(obj) / 8))
		{
		    too_big = 1;
		    break;
		}
	    }
	    if (too_big)
		break;
	}

	for (k = 0, drawpct = TRUE; k <= i; k++)
	{
	    /* draw 0% */
	    if (k == 0)
	    {
		ZText *ztext = zune_text_new(NULL,"0%",ZTEXT_ARG_NONE,0);
		if (ztext)
		{
		    zune_text_get_bounds(ztext, obj);
		    zune_text_draw(ztext, obj, _mleft(obj),_mright(obj),_mtop(obj) + 4);
		    zune_text_destroy(ztext);
		}
		Move(_rp(obj), _mleft(obj), _mtop(obj));
		Draw(_rp(obj), _mleft(obj), _mtop(obj) + 3);
		drawpct = FALSE;
	    }
	    else if (k == i) /* draw 100% */
	    {
		ZText *ztext = zune_text_new("\33r","100%",ZTEXT_ARG_NONE,0);
		if (ztext)
		{
		    zune_text_get_bounds(ztext, obj);
		    zune_text_draw(ztext, obj, _mleft(obj),_mright(obj),_mtop(obj) + 4);
		    zune_text_destroy(ztext);
		}
		Move(_rp(obj), _mright(obj), _mtop(obj));
		Draw(_rp(obj), _mright(obj), _mtop(obj) + 3);
		drawpct = FALSE;
	    }
	    else if (drawpct == TRUE) /* draw intermediate values and lines */
	    {
		ZText *ztext;
		int val = k * 100 / i;

		snprintf(buf, 255, "%d%%", val);
		ztext = zune_text_new(NULL,buf,ZTEXT_ARG_NONE,0);
		if (ztext)
		{
		    int width;
		    zune_text_get_bounds(ztext, obj);
		    width = TextLength(_rp(obj),buf,strlen(buf));
		    zune_text_draw(ztext, obj, _mleft(obj) + _mwidth(obj) * k / i - width / 2,_mright(obj),_mtop(obj) + 4);
		    zune_text_destroy(ztext);
		}
		Move(_rp(obj), _mleft(obj) + _mwidth(obj) * k / i, _mtop(obj));
		Draw(_rp(obj), _mleft(obj) + _mwidth(obj) * k / i, _mtop(obj) + 3);
		drawpct = FALSE;
	    }
	    else /* draw intermediate lines */
	    {
	        Move(_rp(obj), _mleft(obj) + _mwidth(obj) * k / i, _mtop(obj) + 1);
		Draw(_rp(obj), _mleft(obj) + _mwidth(obj) * k / i, _mtop(obj) + 3);
		drawpct = TRUE;
	    }
	}
    }
    else
    {
    }
    return 0;
}

#if ZUNE_BUILTIN_SCALE
BOOPSI_DISPATCHER(IPTR, Scale_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Scale__OM_NEW(cl, obj, (struct opSet *)msg);
	case OM_SET: return Scale__OM_SET(cl, obj, (struct opSet *)msg);
	case OM_GET: return Scale__OM_GET(cl, obj, (struct opGet *)msg);
	case MUIM_Setup: return Scale__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
	case MUIM_AskMinMax: return Scale__MUIM_AskMinMax(cl, obj, (struct MUIP_AskMinMax*)msg);
	case MUIM_Draw: return Scale__MUIM_Draw(cl, obj, (struct MUIP_Draw*)msg);
        default: return DoSuperMethodA(cl, obj, msg);
    }    
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Scale_desc =
{ 
    MUIC_Scale, 
    MUIC_Area, 
    sizeof(struct Scale_DATA), 
    (void*)Scale_Dispatcher 
};
#endif /* ZUNE_BUILTIN_SCALE */
