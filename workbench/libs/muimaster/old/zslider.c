/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

#ifdef _AROS
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <intuition/gadgetclass.h>
#endif

#include <zunepriv.h>
#include <builtin.h>

#include <Area.h>
#include <Numeric.h>
#include <Slider.h>
#include <sliderdata.h>
#include <Group.h>
#include <Window.h>
#include <renderinfo.h>
#include <macros.h>


/*
Slider.mui/MUIA_Slider_Horiz        
Slider.mui/MUIA_Slider_Level        
Slider.mui/MUIA_Slider_Max          
Slider.mui/MUIA_Slider_Min          
Slider.mui/MUIA_Slider_Quiet        
Slider.mui/MUIA_Slider_Reverse      d
*/


static ULONG 
mNew(struct IClass *cl, Object * obj, struct opSet *msg)
{
    struct MUI_SliderData *data;
    struct TagItem *tags, *tag;
    ULONG flags = 0;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
	switch (tag->ti_Tag)
	{
	case MUIA_Group_Horiz:
	case MUIA_Slider_Horiz:
	  _handle_bool_tag(flags, tag->ti_Data, SLIDER_HORIZ);
	  break;
	case MUIA_Slider_Quiet:
	  _handle_bool_tag(flags, tag->ti_Data, SLIDER_QUIET);
	  break;
	}
    }

    obj = (Object *)DoSuperNew(cl, obj,
	MUIA_Background, MUII_SliderBack,
	MUIA_Frame, MUIV_Frame_Slider,
	TAG_MORE, msg->ops_AttrList);

    if (!obj)
    {
kprintf("*** Slider->New: obj=0\n");
	return NULL;
    }

    data = INST_DATA(cl, obj);
    data->gadget = NULL;
    data->flags = flags;

#ifdef _AROS
    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
#else
    data->ehn.ehn_Events = GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK;
#endif

    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    return (ULONG)obj;
}

/*  static ULONG */
/*  mDispose(struct IClass *cl, Object *obj, Msg msg) */
/*  { */
/*      struct MUI_SliderData *data = INST_DATA(cl, obj); */
/*      Object *cont; */

/*  g_print("slider dispose\n"); */
/*      return DoSuperMethodA(cl, obj, msg); */
/*  } */

static ULONG
mSetup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl,obj,(Msg)msg))
	return FALSE;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehn);

    return TRUE;
}

static ULONG
mCleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehn);
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

static ULONG
mAskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    if (data->flags & SLIDER_HORIZ)
    {
	msg->MinMaxInfo->MinWidth  += 24;
	msg->MinMaxInfo->MinHeight += 12;
	msg->MinMaxInfo->DefWidth  += 42;
	msg->MinMaxInfo->DefHeight += 12;
	msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight += 12;
    }
    else
    {
	msg->MinMaxInfo->MinWidth  += 12;
	msg->MinMaxInfo->MinHeight += 24;
	msg->MinMaxInfo->DefWidth  += 12;
	msg->MinMaxInfo->DefHeight += 42;
	msg->MinMaxInfo->MaxWidth  += 12;
	msg->MinMaxInfo->MaxHeight  = MUI_MAXMAX;
    }

    return TRUE;
}

static ULONG
mShow(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);
    struct MUI_RenderInfo *mri  = muiRenderInfo(obj);

    data->gadget = (struct Gadget *)NewObject(NULL, PROPGCLASS,
	GA_Left,   _mleft(obj),
	GA_Top,    _mtop(obj),
	GA_Width,  _mwidth(obj),
	GA_Height, _mheight(obj),
	PGA_Freedom, data->flags & SLIDER_HORIZ ? FREEHORIZ : FREEVERT,
	PGA_Total,   100,
	PGA_Visible,  10,
	PGA_Top,       1,
	//PGA_NewLook, TRUE,
	TAG_DONE);
    if (data->gadget != NULL)
    {
kprintf("*** Slider->Show: got gadget\n");
	AddGadget(mri->mri_Window, data->gadget, 0);
	RefreshGList(data->gadget, mri->mri_Window, NULL, 1);
    }

    return 0;
}

static ULONG
mHide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);
    struct MUI_RenderInfo *mri  = muiRenderInfo(obj);

    if (data->gadget != NULL)
    {
	RemoveGadget(mri->mri_Window, data->gadget);
	DisposeObject(data->gadget);
	data->gadget = NULL;
    }

    return 0;
}

static ULONG
mDraw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);
    struct MUI_RenderInfo *mri;

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (!(msg->flags & MADF_DRAWOBJECT))
	return 0;

    if (_mwidth(obj) < 1 || _mheight(obj) < 1)
	return TRUE;

    mri = muiRenderInfo(obj);

    //SetAPen(_rp(obj), _pens(obj)[MPEN_FILL]);
    //SetDrMd(_rp(obj), JAM1);
    //RectFill(_rp(obj), _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj));
    if (data->gadget != NULL)
    {
kprintf("*** Slider->Draw: refresh gadget\n");
	RefreshGList(data->gadget, mri->mri_Window, NULL, 1);
    }

    return TRUE;
}

#ifdef _AROS

static ULONG
event_button(struct IClass *cl, Object *obj, struct IntuiMessage *imsg)
{
  return 0;
}

static ULONG
event_motion(struct IClass *cl, Object *obj, struct IntuiMessage *imsg)
{
  return 0;
}

#else

static ULONG
event_button_press(struct IClass *cl, Object *obj, GdkEventButton *evb)
{
}

static ULONG
event_button_release(struct IClass *cl, Object *obj, GdkEventButton *evb)
{
}

static ULONG
event_motion(struct IClass *cl, Object *obj, GdkEventMotion *evm)
{
}

#endif

static ULONG
mHandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_SliderData *data = INST_DATA(cl, obj);

    if (msg->imsg)
    {
#ifdef _AROS
	switch (msg->imsg->Class)
	{
	case IDCMP_MOUSEBUTTONS:
	  return event_button(cl, obj, msg->imsg);
	case IDCMP_MOUSEMOVE:
	  return event_motion(cl, obj, msg->imsg);
	}
#else
	switch (msg->imsg->type)
	{
	case GDK_BUTTON_PRESS:
	  return event_button_press(cl, obj, (GdkEventButton *)msg->imsg);
	case GDK_BUTTON_RELEASE:
	  return event_button_release(cl, obj, (GdkEventButton *)msg->imsg);
	case GDK_MOTION_NOTIFY:
	  return event_motion(cl, obj, (GdkEventMotion *)msg->imsg);
	default:
	  return 0;
	}
#endif
    }

    return MUI_EventHandlerRC_Eat;
}


/* static ULONG */
/* Slider_Dispatcher (struct IClass *cl, Object *obj, Msg msg) */
AROS_UFH3S(IPTR, Slider_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
    switch (msg->MethodID)
    {
	case OM_NEW:
	    return mNew(cl, obj, (struct opSet *)msg);
/*  	case OM_DISPOSE: */
/*  	    return mDispose(cl, obj, msg)); */
	case MUIM_Setup :
	    return mSetup(cl, obj, (APTR)msg);
	case MUIM_Cleanup :
	    return mCleanup(cl, obj, (APTR)msg);
	case MUIM_AskMinMax :
	    return mAskMinMax(cl, obj, (APTR)msg);
	case MUIM_Show :
	    return mShow(cl, obj, (APTR)msg);
	case MUIM_Hide :
	    return mHide(cl, obj, (APTR)msg);
	case MUIM_Draw :
	    return mDraw(cl, obj, (APTR)msg);
	case MUIM_HandleEvent:
	    return mHandleEvent(cl, obj, (APTR)msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Slider_desc = { 
    MUIC_Slider, 
    MUIC_Numeric, 
    sizeof(struct MUI_SliderData), 
    Slider_Dispatcher 
};
