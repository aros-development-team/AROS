/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>
#include <stdlib.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>

#include <proto/dos.h>
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
#include "frameadjust_private.h"

extern struct Library *MUIMasterBase;

static Object*MakeSpacingSlider (void)
{
    Object *obj = MUI_MakeObject(MUIO_Slider, (IPTR)"", 0, 9);
    set(obj, MUIA_CycleChain, 1);
    return obj;
}

struct SliderFuncMsg
{
    Object *slider;
    struct Frameadjust_DATA *data;
};

static void SliderFunc(struct Hook *hook, Object *obj, struct SliderFuncMsg *msg)
{
    struct Frameadjust_DATA *data = msg->data;
    Object *slider = msg->slider;
    ULONG val;
    char fs[10];

    get(slider, MUIA_Numeric_Value, &val);

    if (slider == data->SL_top)
    {
	nnset(data->SL_bottom, MUIA_Numeric_Value, val);
	data->fs_intern.innerTop = val;
	data->fs_intern.innerBottom = val;
    }
    else if (slider == data->SL_left)
    {
	nnset(data->SL_right, MUIA_Numeric_Value, val);
	data->fs_intern.innerLeft = val;
	data->fs_intern.innerRight = val;
    }
    else if (slider == data->SL_bottom)
    {
	data->fs_intern.innerBottom = val;
    }
    else
    {
	data->fs_intern.innerRight = val;
    }

    zune_frame_intern_to_spec(&data->fs_intern, fs);
    set(data->FD_display, MUIA_Framedisplay_Spec, (IPTR)fs);
}


struct FramesFuncMsg
{
    ULONG type;
    ULONG state;
    struct Frameadjust_DATA *data;
};

static void FramesFunc(struct Hook *hook, Object *obj, struct FramesFuncMsg *msg)
{
    struct Frameadjust_DATA *data = msg->data;
    char fs[10];

    data->fs_intern.type = msg->type;
    data->fs_intern.state = msg->state;
    zune_frame_intern_to_spec(&data->fs_intern, fs);
    set(data->FD_display, MUIA_Framedisplay_Spec, (IPTR)fs);
}

static Object *MakeFrameDisplay(int i, int state)
{
    struct MUI_FrameSpec_intern fsi;
    char fs[10];
    Object *obj;

    if (i < 0 || i > 10)
	return HVSpace;

    fsi.innerTop = fsi.innerLeft = fsi.innerBottom = fsi.innerRight = 9;
    fsi.type = i;
    fsi.state = state;
    zune_frame_intern_to_spec(&fsi, fs);

    obj = MUI_NewObject(MUIC_Framedisplay,
			MUIA_FixWidth, 12,
			MUIA_FixHeight, 12,
			ButtonFrame,
			InnerSpacing(6, 6),
			MUIA_Background, MUII_ButtonBack,
			MUIA_InputMode, MUIV_InputMode_RelVerify,
			MUIA_Framedisplay_Spec, (IPTR)fs,
			TAG_DONE);
    set(obj, MUIA_CycleChain, 1);
    return obj;
}

IPTR Frameadjust__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Frameadjust_DATA   *data;
    struct TagItem  	    *tag, *tags;
    Object *FD_display;
    Object *SL_top, *SL_left, *SL_right, *SL_bottom;
    Object *GR_fd;
    int lut[] = { 0, 1, 2, 3, 4, 6, 9, 10, 8, 7, 5 };
    int i;

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
			       
        MUIA_Group_Horiz,        TRUE,
        MUIA_Group_HorizSpacing, 20,
        
        Child, (IPTR) (FD_display = MUI_NewObject
        (
            MUIC_Framedisplay,
            MUIA_FixWidth, 32,
            TAG_DONE
        )),
        Child, (IPTR) VGroup,
            MUIA_Group_VertSpacing, 10,
            Child, (IPTR) (GR_fd = RowGroup(2), End), /* RowGroup */
            Child, (IPTR) HGroup,
                Child, (IPTR) Label("Inner Spacing:"),
                Child, (IPTR) RowGroup(2),
                    Child, (IPTR) Label2("Left"),
                    Child, (IPTR) (SL_left = MakeSpacingSlider()),
                    Child, (IPTR) HSpace(8),
                    Child, (IPTR) Label2("Top"),
                    Child, (IPTR) (SL_top = MakeSpacingSlider()),
                    Child, (IPTR) Label2("Right"),
                    Child, (IPTR) (SL_right = MakeSpacingSlider()),
                    Child, (IPTR) HSpace(8),
                    Child, (IPTR) Label2("Bottom"),
                    Child, (IPTR) (SL_bottom = MakeSpacingSlider()),
                End, /* RowGroup */
            End, /* HGroup */
        End, /* VGroup */
        
        TAG_MORE, (IPTR) msg->ops_AttrList
    );

    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);
    data->FD_display = FD_display;
    data->SL_left = SL_left;
    data->SL_top = SL_top;
    data->SL_bottom = SL_bottom;
    data->SL_right = SL_right;
    data->slider_hook.h_Entry = HookEntry;
    data->slider_hook.h_SubEntry = (HOOKFUNC)SliderFunc;
    data->frames_hook.h_Entry = HookEntry;
    data->frames_hook.h_SubEntry = (HOOKFUNC)FramesFunc;

    for (i = 0; i < 11; i++)
    {
	Object *obj;

	obj = MakeFrameDisplay(lut[i], 0);
	DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 5,
		 MUIM_CallHook, (IPTR)&data->frames_hook, lut[i], 0, (IPTR)data);
	DoMethod(GR_fd, OM_ADDMEMBER, (IPTR)obj);
    }

    DoMethod(GR_fd, OM_ADDMEMBER, (IPTR)HVSpace);

    for (i = 1; i < 11; i++)
    {
	Object *obj;

	obj = MakeFrameDisplay(lut[i], 1);
	DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 5,
		 MUIM_CallHook, (IPTR)&data->frames_hook, lut[i], 1, (IPTR)data);
	DoMethod(GR_fd, OM_ADDMEMBER, (IPTR)obj);
    }


    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Frameadjust_Spec:
		zune_frame_spec_to_intern((CONST_STRPTR)tag->ti_Data, &data->fs_intern);
		set(data->FD_display, MUIA_Framedisplay_Spec, tag->ti_Data);
		set(data->SL_left, MUIA_Numeric_Value, data->fs_intern.innerLeft);
		set(data->SL_top, MUIA_Numeric_Value, data->fs_intern.innerTop);
		set(data->SL_right, MUIA_Numeric_Value, data->fs_intern.innerRight);
		set(data->SL_bottom, MUIA_Numeric_Value, data->fs_intern.innerBottom);
		break;
	}
    }

    DoMethod(data->SL_left, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
	     (IPTR)obj, 4, MUIM_CallHook,
	     (IPTR)&data->slider_hook, (IPTR)data->SL_left, (IPTR)data);
    DoMethod(data->SL_top, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
	     (IPTR)obj, 4, MUIM_CallHook,
	     (IPTR)&data->slider_hook, (IPTR)data->SL_top, (IPTR)data);
    DoMethod(data->SL_right, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
	     (IPTR)obj, 4, MUIM_CallHook,
	     (IPTR)&data->slider_hook, (IPTR)data->SL_right, (IPTR)data);
    DoMethod(data->SL_bottom, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
	     (IPTR)obj, 4, MUIM_CallHook,
	     (IPTR)&data->slider_hook, (IPTR)data->SL_bottom, (IPTR)data);

    return (IPTR)obj;
}

IPTR Frameadjust__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Frameadjust_DATA *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
	case MUIA_Frameadjust_Spec:
	    zune_frame_intern_to_spec(&data->fs_intern, (STRPTR)data->spec);
	    *msg->opg_Storage = (IPTR)data->spec;
	    return(TRUE);
    }

    return(DoSuperMethodA(cl, obj, (Msg) msg));
}

#if ZUNE_BUILTIN_FRAMEADJUST
BOOPSI_DISPATCHER(IPTR, Frameadjust_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Frameadjust__OM_NEW(cl, obj, (struct opSet *)msg);
	case OM_GET: return Frameadjust__OM_GET(cl,obj,(APTR)msg);
        default:     return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Frameadjust_desc =
{ 
    MUIC_Frameadjust, 
    MUIC_Group,
    sizeof(struct Frameadjust_DATA), 
    (void*)Frameadjust_Dispatcher 
};
#endif /* ZUNE_BUILTIN_FRAMEADJUST */
