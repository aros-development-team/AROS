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

#ifdef _AROS
#include <proto/muimaster.h>
#endif

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

    struct MUI_ImageSpec *cycle_image;

};

#define MAX(a,b) (((a)>(b))?(a):(b))

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Cycle_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_CycleData   *data;
    struct TagItem  	    *tag, *tags;
    int i;

    obj = (Object *)DoSuperNew(cl, obj,
		MUIA_InputMode, MUIV_InputMode_RelVerify,
		TAG_MORE, msg->ops_AttrList);
    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Cycle_Entries:
		    data->entries = (char*)tag->ti_Data;
		    break;
	}
    }

    if (!data->entries)
    {
	D(bug("Cycle_New: Now Entries specified!\n"));
	CoerceMethod(cl,obj,OM_DISPOSE);
	return NULL;
    }

    /* Count the number of entries */
    for (i=0;data->entries[i];i++);

    data->entries_num = i;

    return (IPTR)obj;
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
STATIC IPTR Cycle_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_CycleData   *data;
    struct RastPort rp;
    int i,w = 0;

    if (!(DoSuperMethodA(cl, obj, (Msg)msg)))
	return 0;

    data = INST_DATA(cl, obj);

    if (!(data->cycle_image = zune_image_spec_to_structure(MUII_Cycle, obj)))
    {
	CoerceMethod(cl,obj,MUIM_Cleanup);
	return 0;
    }

    zune_imspec_setup(&data->cycle_image, muiRenderInfo(obj));

    InitRastPort(&rp);
    SetFont(&rp,_font(obj));

    for (i=0;i<data->entries_num;i++)
    {
	int nw = TextLength(&rp,data->entries[i],strlen(data->entries[i]));
	if (nw > w) w = nw;
    }

    data->entries_width = w;
    data->entries_height = _font(obj)->tf_YSize;

    data->cycle_width = zune_imspec_get_minwidth(data->cycle_image);
    data->cycle_height = zune_imspec_get_minheight(data->cycle_image);

    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
STATIC IPTR Cycle_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_CycleData *data = INST_DATA(cl, obj);

    zune_imspec_cleanup(&data->cycle_image, muiRenderInfo(obj));
    zune_imspec_free(data->cycle_image);

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
STATIC IPTR Cycle_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_CycleData *data = INST_DATA(cl, obj);
    DoSuperMethodA(cl,obj,(Msg)msg);

    msg->MinMaxInfo->MinWidth  += data->cycle_width + data->entries_width;
    msg->MinMaxInfo->MinHeight += MAX(data->cycle_height,data->entries_height);

    msg->MinMaxInfo->DefWidth  += data->cycle_width + data->entries_width;
    msg->MinMaxInfo->DefHeight += MAX(data->cycle_height,data->entries_height);

    msg->MinMaxInfo->MaxWidth  = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight += MAX(data->cycle_height,data->entries_height);

    return 1;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
STATIC IPTR Cycle_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_CycleData *data = INST_DATA(cl, obj);
    char *entry = data->entries[data->entries_active];
    int width;

    DoSuperMethodA(cl,obj,(Msg)msg);

    SetFont(_rp(obj),_font(obj));

    zune_draw_image(muiRenderInfo(obj), data->cycle_image,
		 _mleft(obj), _mtop(obj), data->cycle_width, _mheight(obj),0,0,0);

    width = TextLength(_rp(obj),entry,strlen(entry));
    
    SetAPen(_rp(obj),_pens(obj)[MPEN_TEXT]);
    SetDrMd(_rp(obj),JAM1);
    Move(_rp(obj),(_mleft(obj) + data->cycle_width) + (_mwidth(obj) - data->cycle_width - width)/2,_mtop(obj)+(_mheight(obj) - data->entries_height)/2+_font(obj)->tf_Baseline);
    Text(_rp(obj),entry,strlen(entry));

    return 1;
}

#ifndef _AROS
__asm IPTR Cycle_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Cycle_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Cycle_New(cl, obj, (struct opSet *)msg);
	case MUIM_Setup: return Cycle_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup: return Cycle_Cleanup(cl, obj, (APTR)msg);
	case MUIM_AskMinMax: return Cycle_AskMinMax(cl, obj, (APTR)msg);
	case MUIM_Draw: return Cycle_Draw(cl,obj,(APTR)msg);

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
