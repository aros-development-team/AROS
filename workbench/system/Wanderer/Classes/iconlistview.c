/*
Copyright © 2002-2011, The AROS Development Team. 
$Id$
*/

#define DEBUG 0

#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>

#ifdef __AROS__
#include <aros/debug.h>
#include <clib/alib_protos.h>
#else
#include "../portable_macros.h"
#define WANDERER_BUILTIN_ICONLISTVIEW 1
#endif

#include <proto/exec.h>
#include <proto/utility.h>

#if defined(__AMIGA__) && !defined(__PPC__)
#define NO_INLINE_STDARG
#endif
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <libraries/mui.h>
//#include "muimaster_intern.h"
//#include "support.h"
#include "iconlist.h"
#include "iconlistview.h"
#include "iconlistview_private.h"

#ifndef __AROS__

#ifdef DEBUG
  #define D(x) if (DEBUG) x
  #ifdef __amigaos4__
  #define bug DebugPrintF
  #else
  #define bug kprintf
  #endif
#else
  #define  D(...)
#endif

#define ScrollbuttonObject MUI_MakeObject(MUIO_Button, (IPTR)"scroll"

#endif

extern struct Library *MUIMasterBase;

///IconListview_Layout_Function()
ULONG IconListview_Layout_Function(struct Hook *hook, Object *obj, struct MUI_LayoutMsg *lm)
{
    struct IconListview_DATA *data = (struct IconListview_DATA *)hook->h_Data;
    
    switch (lm->lm_Type)
    {
        case MUILM_MINMAX:
        {
            /* Calulate the minmax dimension of the group,
            ** We only have a fixed number of children, so we need no NextObject()
            */
            
            WORD maxxxxwidth = 0;
            WORD maxxxxheight = 0;

            maxxxxwidth = _minwidth(data->iconlist) + _minwidth(data->vert);
            if (_minwidth(data->horiz) > maxxxxwidth) maxxxxwidth = _minwidth(data->horiz);
                lm->lm_MinMax.MinWidth = maxxxxwidth;

            maxxxxheight = _minheight(data->iconlist) + _minheight(data->horiz);
            if (_minheight(data->vert) > maxxxxheight) maxxxxheight = _minheight(data->vert);
                lm->lm_MinMax.MinHeight = maxxxxheight;

            maxxxxwidth = _defwidth(data->iconlist) + _defwidth(data->vert);
            if (_defwidth(data->horiz) > maxxxxwidth) maxxxxwidth = _defwidth(data->horiz);
                lm->lm_MinMax.DefWidth = maxxxxwidth;

            maxxxxheight = _defheight(data->iconlist) + _defheight(data->horiz);
            if (_defheight(data->vert) > maxxxxheight) maxxxxheight = _defheight(data->vert);
                lm->lm_MinMax.DefHeight = maxxxxheight;

            lm->lm_MinMax.MaxWidth  = MUI_MAXMAX;
            lm->lm_MinMax.MaxHeight = MUI_MAXMAX;
            
            return 0;
        }

        case MUILM_LAYOUT:
        {
            /* 
            Now place the objects between (0,0,lm->lm_Layout.Width-1,lm->lm_Layout.Height-1)
            */

            LONG virt_width   = 0;
            LONG virt_height  = 0;
            LONG vert_width   = _minwidth(data->vert);
            LONG horiz_height = _minheight(data->horiz);
            LONG lay_width    = lm->lm_Layout.Width;
            LONG lay_height   = lm->lm_Layout.Height;
            LONG cont_width;
            LONG cont_height;

            /* layout the virtual group a first time, to determine the virtual width/height */
            MUI_Layout(data->iconlist,0,0,lay_width,lay_height,0);

            get(data->iconlist, MUIA_IconList_Width, &virt_width);
            get(data->iconlist, MUIA_IconList_Height, &virt_height);

            virt_width += _subwidth(data->iconlist);
            virt_height += _subheight(data->iconlist);

            if (virt_width > lay_width && virt_height > lay_height)
            {
                /* We need all scrollbars and the button */
                set(data->vert, MUIA_ShowMe, TRUE); /* We could also overload MUIM_Show... */
                set(data->horiz, MUIA_ShowMe, TRUE);
                set(data->button, MUIA_ShowMe, TRUE);
                cont_width = lay_width - vert_width;
                cont_height = lay_height - horiz_height;
                MUI_Layout(data->vert, cont_width, 0, vert_width, cont_height, 0);
                MUI_Layout(data->horiz, 0, cont_height, cont_width, horiz_height, 0);
                MUI_Layout(data->button, cont_width, cont_height, vert_width, horiz_height, 0);
            } 
            else
            {
                if (virt_height > lay_height)
                {
                    set(data->vert, MUIA_ShowMe, TRUE);
                    set(data->horiz, MUIA_ShowMe, FALSE);
                    set(data->button, MUIA_ShowMe, FALSE);
    
                    cont_width = lay_width - vert_width;
                    cont_height = lay_height;
                    MUI_Layout(data->vert, cont_width, 0, vert_width, cont_height,0);
                } 
                else
                {
                    if (virt_width > lay_width)
                    {
                        set(data->vert, MUIA_ShowMe, FALSE);
                        set(data->horiz, MUIA_ShowMe, TRUE);
                        set(data->button, MUIA_ShowMe, FALSE);
        
                        cont_width = lay_width;
                        cont_height = lay_height - horiz_height;
                        MUI_Layout(data->horiz, 0, cont_height, cont_width, horiz_height, 0);
                    } 
                    else
                    {
                        set(data->vert, MUIA_ShowMe, FALSE);
                        set(data->horiz, MUIA_ShowMe, FALSE);
                        set(data->button, MUIA_ShowMe, FALSE);
                        
                        cont_width = lay_width;
                        cont_height = lay_height;
                    }
                }
            }

            /* Layout the group a second time, note that setting _mwidth() and _mheight() should be enough, or we invent a new flag */
            MUI_Layout(data->iconlist,0,0,cont_width,cont_height,0);
            
            return 1;
        }
    }
    return 0;
}
///

///IconListview_Function()
ULONG IconListview_Function(struct Hook *hook, APTR dummyobj, void **msg)
{
    struct IconListview_DATA *data = (struct IconListview_DATA *)hook->h_Data;
    int type = (IPTR)msg[0];
    LONG val = (IPTR)msg[1];

    switch (type)
    {
        case 1:
        {
            get(data->vert,MUIA_Prop_First,&val);
            SetAttrs(data->iconlist,MUIA_Virtgroup_Top, val, MUIA_NoNotify, TRUE, TAG_DONE);
            break;
        }

        case 2:
        {
            get(data->horiz,MUIA_Prop_First,&val);
            SetAttrs(data->iconlist,MUIA_Virtgroup_Left, val, MUIA_NoNotify, TRUE, TAG_DONE);
            break;
        }
        case 3: 
            nnset(data->horiz, MUIA_Prop_First, val); 
            break;
        case 4: 
            nnset(data->vert, MUIA_Prop_First, val); 
            break;
    }
    
    return 0;
}
///

///OM_NEW()
IPTR IconListview__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct IconListview_DATA *data;
    //struct TagItem *tags,*tag;
    Object *iconlist = (Object*)GetTagData(MUIA_IconListview_IconList, 0, msg->ops_AttrList);
    Object *vert,*horiz,*button,*group;

    struct Hook *layout_hook = AllocVec(sizeof(struct Hook), MEMF_CLEAR);
    int usewinborder;

    if (!layout_hook) 
        return 0;
    usewinborder = GetTagData(MUIA_IconListview_UseWinBorder, FALSE, msg->ops_AttrList);

    if (!usewinborder) 
        button = ScrollbuttonObject, End;
    else 
        button = NULL;

    #ifndef __amigaos4__
    layout_hook->h_Entry = HookEntry;
    #endif
    layout_hook->h_SubEntry = (HOOKFUNC)IconListview_Layout_Function;

    group = MUI_NewObject(MUIC_Group,
                (usewinborder ? TAG_IGNORE : MUIA_Group_LayoutHook), layout_hook,
                (iconlist ? Child : TAG_IGNORE), iconlist,
                Child, vert= MUI_NewObject(MUIC_Scrollbar,
                    (usewinborder ? MUIA_Prop_UseWinBorder : TAG_IGNORE), MUIV_Prop_UseWinBorder_Right,
                    MUIA_Prop_DeltaFactor, 20,
                    MUIA_Group_Horiz, FALSE,
                TAG_DONE),
                Child, horiz = MUI_NewObject(MUIC_Scrollbar,
                    (usewinborder ? MUIA_Prop_UseWinBorder : TAG_IGNORE), MUIV_Prop_UseWinBorder_Bottom,
                    MUIA_Prop_DeltaFactor, 20,
                    MUIA_Group_Horiz, TRUE,
                TAG_DONE),
                (usewinborder ? TAG_IGNORE : Child), button,
            TAG_DONE);

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
            MUIA_Group_Horiz, FALSE,
            MUIA_Group_HorizSpacing, 0,
            MUIA_Group_VertSpacing, 0,
            MUIA_Frame, MUIV_Frame_None,

            Child, (IPTR) group,
            TAG_DONE);

    if (!obj)
    {
        MUI_DisposeObject(group);
        FreeVec(layout_hook);
        return 0;
    }

    data = INST_DATA(cl, obj);

D(bug("[IconListview] %s: SELF = 0x%p\n", __PRETTY_FUNCTION__, obj));

    data->vert = vert;
    data->horiz = horiz;
    data->button = button;
    data->iconlist = iconlist;

    #ifndef __amigaos4__
    data->hook.h_Entry = HookEntry;
    #endif
    data->hook.h_SubEntry = (HOOKFUNC)IconListview_Function;
    data->hook.h_Data = data;
    data->layout_hook = layout_hook;
    layout_hook->h_Data = data;

    DoMethod(vert, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, 1, MUIV_TriggerValue);
    DoMethod(horiz, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, 2, MUIV_TriggerValue);
    DoMethod(iconlist, MUIM_Notify, MUIA_Virtgroup_Left, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, 3, MUIV_TriggerValue);
    DoMethod(iconlist, MUIM_Notify, MUIA_Virtgroup_Top, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, 4, MUIV_TriggerValue);
    DoMethod(iconlist, MUIM_Notify, MUIA_IconList_Width, MUIV_EveryTime, (IPTR)horiz, 3, MUIM_NoNotifySet, MUIA_Prop_Entries, MUIV_TriggerValue);
    DoMethod(iconlist, MUIM_Notify, MUIA_IconList_Height, MUIV_EveryTime, (IPTR)vert, 3, MUIM_NoNotifySet, MUIA_Prop_Entries, MUIV_TriggerValue);

D(bug("[IconListview] obj = %ld\n", obj));
    return (IPTR)obj;
}
///

///OM_DISPOSE()
IPTR IconListview__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct IconListview_DATA *data = INST_DATA(cl, obj);

    FreeVec(data->layout_hook);

    return DoSuperMethodA(cl,obj,msg);
}

IPTR IconListview__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem         *tag, *tags;
    
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Background:
            D(bug("[IconListview] %s: MUIA_Background!\n", __PRETTY_FUNCTION__));
            break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}
///


///OM_GET()
/**************************************************************************
OM_GET
**************************************************************************/
IPTR IconListview__OM_GET(struct IClass *CLASS, Object *obj, struct opGet *message)
{
#define STORE *(message->opg_Storage)

    D(bug("[IconListview]: %s()\n", __PRETTY_FUNCTION__));

    switch (message->opg_AttrID)
    {
        /* TODO: Get the version/revision from our config.. */
        case MUIA_Version:                              STORE = (IPTR)1; return 1;
        case MUIA_Revision:                             STORE = (IPTR)3; return 1;
    }

    return DoSuperMethodA(CLASS, obj, (Msg) message);
#undef STORE
}
///

///MUIM_Show()
IPTR IconListview__MUIM_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct IconListview_DATA *data = INST_DATA(cl, obj);
    IPTR top    = 0;
    IPTR left   = 0;
    IPTR width  = 0;
    IPTR height = 0;

    get(data->iconlist, MUIA_Virtgroup_Left, &left);
    get(data->iconlist, MUIA_Virtgroup_Top, &top);
    get(data->iconlist, MUIA_IconList_Width, &width);
    get(data->iconlist, MUIA_IconList_Height, &height);

    SetAttrs(
        data->horiz, 
        MUIA_Prop_First, left,
        MUIA_Prop_Entries, width,
        MUIA_Prop_Visible, _mwidth(data->iconlist),
        TAG_DONE
    );

    SetAttrs(
        data->vert,  
        MUIA_Prop_First, top,
        MUIA_Prop_Entries, height,
        MUIA_Prop_Visible, _mheight(data->iconlist),
        TAG_DONE
    );

    return DoSuperMethodA(cl,obj,(Msg)msg);
}
///

#if WANDERER_BUILTIN_ICONLISTVIEW
BOOPSI_DISPATCHER(IPTR,IconListview_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:            return IconListview__OM_NEW(cl, obj, (struct opSet *) msg);
        case OM_DISPOSE:        return IconListview__OM_DISPOSE(cl, obj, msg);
        case MUIM_Show:         return IconListview__MUIM_Show(cl, obj, (struct MUIP_Show*)msg);
        case OM_SET:            return IconListview__OM_SET(cl, obj, (struct opGet *) msg);
        case OM_GET:            return IconListview__OM_GET(cl, obj, (struct opSet *) msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

#ifdef __AROS__
const struct __MUIBuiltinClass _MUI_IconListview_desc =
{
    MUIC_IconListview, 
    MUIC_Group, 
    sizeof(struct IconListview_DATA), 
    (void*)IconListview_Dispatcher 
};
#endif
#endif /* ZUNE_BUILTIN_ICONLISTVIEW */

#ifndef __AROS__
struct MUI_CustomClass  *initIconListviewClass(void)
{
  return (struct MUI_CustomClass *) MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct IconListview_DATA), ENTRY(IconListview_Dispatcher));
}

#endif
