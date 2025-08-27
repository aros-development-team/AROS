/*
    Copyright  2002-2003, The AROS Development Team. All rights reserved.
*/

#include "intuition/classusr.h"
#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>
#include <stdlib.h>

#include <clib/alib_protos.h>
#include <graphics/gfx.h>
#include <graphics/view.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <string.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "frame.h"
#include "frameadjust_private.h"
#include "locale.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"

extern struct Library *MUIMasterBase;

static Object *MakeSpacingSlider(void)
{
    Object *obj = MUI_MakeObject(MUIO_Slider, (IPTR) "", 0, 9);
    set(obj, MUIA_CycleChain, 1);
    return obj;
}

static Object *MakeRoundedSlider(ULONG min, ULONG max, ULONG def)
{
    Object *obj = MUI_MakeObject(MUIO_Slider, (IPTR) "", min, max);
    if (obj) {
        set(obj, MUIA_CycleChain, 1);
        set(obj, MUIA_Numeric_Value, def);
    }
    return obj;
}

struct SliderFuncMsg {
    STACKED Object *slider;
    STACKED struct Frameadjust_DATA *data;
};

static void SliderFunc(struct Hook *hook, Object *obj,
                       struct SliderFuncMsg *msg)
{
    struct Frameadjust_DATA *data = msg->data;
    Object *slider = msg->slider;
    ULONG val = 0;
    char fs[10];

    get(slider, MUIA_Numeric_Value, &val);

    if (slider == data->SL_top) {
        nnset(data->SL_bottom, MUIA_Numeric_Value, val);
        data->fs_intern.innerTop = val;
        data->fs_intern.innerBottom = val;
    } else if (slider == data->SL_left) {
        nnset(data->SL_right, MUIA_Numeric_Value, val);
        data->fs_intern.innerLeft = val;
        data->fs_intern.innerRight = val;
    } else if (slider == data->SL_bottom) {
        data->fs_intern.innerBottom = val;
    } else {
        data->fs_intern.innerRight = val;
    }

    zune_frame_intern_to_spec(&data->fs_intern, fs);
    set(data->FD_display, MUIA_Framedisplay_Spec, (IPTR)fs);
}

struct RoundedFuncMsg {
    STACKED Object *obj;
    STACKED struct Frameadjust_DATA *data;
};

static void RoundedFunc(struct Hook *hook, Object *obj,
                        struct RoundedFuncMsg *msg)
{
    struct Frameadjust_DATA *data = msg->data;
    BOOL use_rounded;
    ULONG radius, width;
    char fs[16];

    get(data->rounded_check, MUIA_Selected, &use_rounded);
    get(data->radius_slider, MUIA_Numeric_Value, &radius);
    get(data->width_slider, MUIA_Numeric_Value, &width);

    if (use_rounded) {
        /* Set frame type to FST_ROUNDED and update radius/width */
        data->fs_intern.type = FST_ROUNDED; /* FST_ROUNDED */
        data->fs_intern.border_radius = radius;
        data->fs_intern.border_width = width;
    } else {
        /* When unchecked, reset to a basic frame type and clear rounded properties
         */
        if (data->fs_intern.type == FST_ROUNDED) { /* FST_ROUNDED */
            data->fs_intern.type = 2; /* FST_BEVEL - a reasonable default */
        }
        data->fs_intern.border_radius = 0;
        data->fs_intern.border_width = 0;
    }

    zune_frame_intern_to_spec(&data->fs_intern, fs);
    set(data->FD_display, MUIA_Framedisplay_Spec, (IPTR)fs);
}

struct FramesFuncMsg {
    STACKED ULONG type;
    STACKED ULONG state;
    STACKED struct Frameadjust_DATA *data;
};

static void FramesFunc(struct Hook *hook, Object *obj,
                       struct FramesFuncMsg *msg)
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

    if (i < 0 || i > (10 + 16))
        return HVSpace;

    fsi.innerTop = fsi.innerLeft = fsi.innerBottom = fsi.innerRight = 9;
    fsi.border_radius = 0;
    fsi.border_width = 0;
    fsi.type = i;
    fsi.state = state;
    zune_frame_intern_to_spec(&fsi, fs);

    obj = MUI_NewObject(MUIC_Framedisplay, MUIA_FixWidth, 32, MUIA_FixHeight, 32,
                        MUIA_Frame, "022220", InnerSpacing(6, 6), MUIA_Background,
                        MUII_ButtonBack, MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Framedisplay_Spec, (IPTR)fs, TAG_DONE);
    set(obj, MUIA_CycleChain, 1);
    return obj;
}

IPTR Frameadjust__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Frameadjust_DATA *data;
    struct TagItem *tags;
    struct TagItem *tag;
    Object *FD_display;
    Object *SL_top, *SL_left, *SL_right, *SL_bottom;
    Object *rounded_check, *radius_slider, *width_slider;
    Object *GR_fd;
    Object *GR_fd1;
    Object *GR_fd2;
    int lut[] = {0,  1,  2,  3,  4,  6,  9,  10, 8,  7,  5,  11, 12, 13,
                 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26
                };
    int i;

    // clang-format off
    obj = (Object *) DoSuperNewTags
          (
              cl, obj, NULL,

              MUIA_Group_Horiz,        TRUE,
              MUIA_Group_HorizSpacing, 10,
              Child, (IPTR) VGroup,
              Child, (IPTR) HGroup, GroupFrame, MUIA_FrameTitle, (IPTR) _(MSG_FRAMEADJUST_EXAMPLE),
              Child, (IPTR) (FD_display = MUI_NewObject(MUIC_Framedisplay, MUIA_FixWidth, 64, MUIA_FixHeight, 64, TAG_DONE)),
              End,
              Child, (IPTR) HVSpace,
              End, /* VGroup */
              Child, (IPTR) VGroup,
              MUIA_Group_VertSpacing, 10,
              Child, (IPTR) VGroup, GroupFrame, MUIA_FrameTitle, (IPTR) _(MSG_FRAMEADJUST_ROUNDED_FRAME),
              Child, (IPTR) HGroup,
              Child, (IPTR) Label2(_(MSG_FRAMEADJUST_ENABLE)),
              Child, (IPTR) (rounded_check = MUI_MakeObject(MUIO_Checkmark, (IPTR)_(MSG_FRAMEADJUST_ENABLE))),
              Child, (IPTR) HVSpace,
              End, /* HGroup */
              Child, (IPTR) HGroup,
              Child, (IPTR) Label2(_(MSG_FRAMEADJUST_RADIUS)),
              Child, (IPTR) (radius_slider = MakeRoundedSlider(0, 15, 5)),
              Child, (IPTR) Label2(_(MSG_FRAMEADJUST_WIDTH)),
              Child, (IPTR) (width_slider = MakeRoundedSlider(1, 5, 1)),
              End, /* HGroup */
              End, /* VGroup */
              Child, (IPTR) VGroup,
              Child, (IPTR) HGroup, GroupFrame, MUIA_FrameTitle, (IPTR)_(MSG_FRAMEADJUST_BUILTIN_FRAMES),
              Child, (IPTR) (GR_fd = (Object *)RowGroup(2), End), /* RowGroup */
              End, /* VGroup */
              Child, (IPTR) VGroup,
              Child, (IPTR) VGroup, GroupFrame, MUIA_FrameTitle, (IPTR)_(MSG_FRAMEADJUST_CUSTOM_FRAMES),
              MUIA_Group_VertSpacing, 10,
              Child, (IPTR) (GR_fd1 = (Object *)RowGroup(2), End), /* RowGroup */
              Child, (IPTR) HGroup, Child, (IPTR) (GR_fd2 = (Object *)RowGroup(2), End), Child, (IPTR) HVSpace, End,/* RowGroup */
              End, /* HGroup */
              Child, (IPTR) HVSpace,
              End,
              End, /* HGroup */
              Child, (IPTR) VGroup, GroupFrame, MUIA_FrameTitle, (IPTR) _(MSG_FRAMEADJUST_INNER_SPACING),
              Child, (IPTR) RowGroup(2),
              Child, (IPTR) Label2(_(MSG_FRAMEADJUST_LEFT)),
              Child, (IPTR) (SL_left = MakeSpacingSlider()),
              Child, (IPTR) Label2(_(MSG_FRAMEADJUST_TOP)),
              Child, (IPTR) (SL_top = MakeSpacingSlider()),
              Child, (IPTR) Label2(_(MSG_FRAMEADJUST_RIGHT)),
              Child, (IPTR) (SL_right = MakeSpacingSlider()),
              Child, (IPTR) Label2(_(MSG_FRAMEADJUST_BOTTOM)),
              Child, (IPTR) (SL_bottom = MakeSpacingSlider()),
              End, /* RowGroup */
              End, /* VGroup */
              End, /* VGroup */

              TAG_MORE, (IPTR) msg->ops_AttrList
          );
    // clang-format on
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);
    data->FD_display = FD_display;
    data->SL_left = SL_left;
    data->SL_top = SL_top;
    data->SL_bottom = SL_bottom;
    data->SL_right = SL_right;
    data->rounded_check = rounded_check;
    data->radius_slider = radius_slider;
    data->width_slider = width_slider;
    data->slider_hook.h_Entry = HookEntry;
    data->slider_hook.h_SubEntry = (HOOKFUNC)SliderFunc;
    data->frames_hook.h_Entry = HookEntry;
    data->frames_hook.h_SubEntry = (HOOKFUNC)FramesFunc;
    data->rounded_hook.h_Entry = HookEntry;
    data->rounded_hook.h_SubEntry = (HOOKFUNC)RoundedFunc;

    for (i = 0; i < 11; i++) {
        Object *obj;

        obj = MakeFrameDisplay(lut[i], 0);
        DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 5, MUIM_CallHook,
                 (IPTR)&data->frames_hook, lut[i], 0, (IPTR)data);
        DoMethod(GR_fd, OM_ADDMEMBER, (IPTR)obj);
    }

    DoMethod(GR_fd, OM_ADDMEMBER, (IPTR)HVSpace);

    for (i = 1; i < 11; i++) {
        Object *obj;

        obj = MakeFrameDisplay(lut[i], 1);
        DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 5, MUIM_CallHook,
                 (IPTR)&data->frames_hook, lut[i], 1, (IPTR)data);
        DoMethod(GR_fd, OM_ADDMEMBER, (IPTR)obj);
    }

    /* Frame type 11 is rounded frame, which is configured seperately. Therefore
     * we skip to 12 here */

    for (i = 12; i < (12 + 11); i++) {
        Object *obj;

        obj = MakeFrameDisplay(lut[i], 0);
        DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 5, MUIM_CallHook,
                 (IPTR)&data->frames_hook, lut[i], 0, (IPTR)data);
        DoMethod(GR_fd1, OM_ADDMEMBER, (IPTR)obj);
    }

    for (i = 12; i < (12 + 11); i++) {
        Object *obj;

        obj = MakeFrameDisplay(lut[i], 1);
        DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 5, MUIM_CallHook,
                 (IPTR)&data->frames_hook, lut[i], 1, (IPTR)data);
        DoMethod(GR_fd1, OM_ADDMEMBER, (IPTR)obj);
    }

    for (i = 20; i < (20 + 5); i++) {
        Object *obj;

        obj = MakeFrameDisplay(lut[i], 0);
        DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 5, MUIM_CallHook,
                 (IPTR)&data->frames_hook, lut[i], 0, (IPTR)data);
        DoMethod(GR_fd2, OM_ADDMEMBER, (IPTR)obj);
    }

    for (i = 20; i < (20 + 5); i++) {
        Object *obj;

        obj = MakeFrameDisplay(lut[i], 1);
        DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 5, MUIM_CallHook,
                 (IPTR)&data->frames_hook, lut[i], 1, (IPTR)data);
        DoMethod(GR_fd2, OM_ADDMEMBER, (IPTR)obj);
    }

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));) {
        switch (tag->ti_Tag) {
        case MUIA_Frameadjust_Spec:
            zune_frame_spec_to_intern((CONST_STRPTR)tag->ti_Data, &data->fs_intern);
            set(data->FD_display, MUIA_Framedisplay_Spec, tag->ti_Data);
            set(data->SL_left, MUIA_Numeric_Value, data->fs_intern.innerLeft);
            set(data->SL_top, MUIA_Numeric_Value, data->fs_intern.innerTop);
            set(data->SL_right, MUIA_Numeric_Value, data->fs_intern.innerRight);
            set(data->SL_bottom, MUIA_Numeric_Value, data->fs_intern.innerBottom);
            /* Set rounded frame controls */
            if (data->fs_intern.type == FST_ROUNDED) { /* FST_ROUNDED */
                set(data->rounded_check, MUIA_Selected, TRUE);
                set(data->radius_slider, MUIA_Numeric_Value,
                    data->fs_intern.border_radius);
                set(data->width_slider, MUIA_Numeric_Value,
                    data->fs_intern.border_width);
            } else {
                set(data->rounded_check, MUIA_Selected, FALSE);
            }
            break;
        }
    }

    DoMethod(data->SL_left, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->slider_hook,
             (IPTR)data->SL_left, (IPTR)data);
    DoMethod(data->SL_top, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->slider_hook,
             (IPTR)data->SL_top, (IPTR)data);
    DoMethod(data->SL_right, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->slider_hook,
             (IPTR)data->SL_right, (IPTR)data);
    DoMethod(data->SL_bottom, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->slider_hook,
             (IPTR)data->SL_bottom, (IPTR)data);

    DoMethod(data->rounded_check, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->rounded_hook, (IPTR)obj,
             (IPTR)data);
    DoMethod(data->radius_slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->rounded_hook, (IPTR)obj,
             (IPTR)data);
    DoMethod(data->width_slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->rounded_hook, (IPTR)obj,
             (IPTR)data);

    return (IPTR)obj;
}

IPTR Frameadjust__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Frameadjust_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID) {
    case MUIA_Frameadjust_Spec:
        zune_frame_intern_to_spec(&data->fs_intern, (STRPTR)data->spec);
        *msg->opg_Storage = (IPTR)data->spec;
        return (TRUE);
    }

    return (DoSuperMethodA(cl, obj, (Msg)msg));
}

#if ZUNE_BUILTIN_FRAMEADJUST
BOOPSI_DISPATCHER(IPTR, Frameadjust_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID) {
    case OM_NEW:
        return Frameadjust__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Frameadjust__OM_GET(cl, obj, (APTR)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Frameadjust_desc = {
    MUIC_Frameadjust, MUIC_Group, sizeof(struct Frameadjust_DATA),
    (void *)Frameadjust_Dispatcher
};
#endif /* ZUNE_BUILTIN_FRAMEADJUST */
