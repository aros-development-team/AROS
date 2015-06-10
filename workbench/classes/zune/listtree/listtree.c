/*
    Copyright © 2012-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <clib/alib_protos.h>
#include <mui/NListtree_mcc.h>

#undef TNF_OPEN
#undef TNF_LIST
#include "Listtree_mcc.h"
#include "listtree_private.h"

#include <aros/debug.h>

#define NEWHANDLE(attrname)                                         \
    case(attrname):                                                 \
        bug("[Listtree] OM_NEW:%s - unsupported\n", #attrname);     \
        break;

/*** Methods ****************************************************************/
Object *Listtree__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Listtree_DATA *data = NULL;
    struct TagItem *tag;
    struct TagItem *tags;
    Object *nlisttree = NULL;

    obj = (Object *) DoSuperNewTags(cl, obj, 0,
            Child, nlisttree = (Object *) NListtreeObject,
                                End,
            TAG_MORE, (IPTR) msg->ops_AttrList);

    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);
    data->nlisttree = nlisttree;

    NewList((struct List*)&data->nodes);
    data->pool = CreatePool(MEMF_ANY | MEMF_CLEAR, 16 * 1024, 8 * 1024);

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
        case(MUIA_Listtree_ConstructHook):
            data->constrhook = (struct Hook *)tag->ti_Data;
            break;
        case(MUIA_Listtree_DestructHook):
            data->destrhook = (struct Hook *)tag->ti_Data;
            break;
        NEWHANDLE(MUIA_Listtree_DisplayHook)
        NEWHANDLE(MUIA_Listtree_Title)
        NEWHANDLE(MUIA_Listtree_Format)
        NEWHANDLE(MUIA_Listtree_DragDropSort)
        NEWHANDLE(MUIA_Listtree_SortHook)
        NEWHANDLE(MUIA_Frame)
        NEWHANDLE(MUIA_List_Title)
        NEWHANDLE(MUIA_List_DragSortable)
        NEWHANDLE(MUIA_ContextMenu)
        NEWHANDLE(MUIA_List_MinLineHeight)
        default:
            bug("[Listtree] OM_NEW: unhandled %x\n", tag->ti_Tag);
        }
    }

    return obj;
}

#define SETHANDLE(attrname)                                         \
    case(attrname):                                                 \
        bug("[Listtree] OM_SET:%s - unsupported\n", #attrname);     \
        break;

IPTR Listtree__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem      *tstate = msg->ops_AttrList;
    struct TagItem      *tag;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        SETHANDLE(MUIA_Listtree_Active)
        SETHANDLE(MUIA_Listtree_Quiet)
        SETHANDLE(MUIA_Listtree_DoubleClick)
        case MUIB_List | 0x00000010: break;
        case MUIA_Prop_First: break;
        case MUIA_Prop_DoSmooth: break;
        case MUIA_NoNotify: break;
        case MUIA_Prop_Entries: break;
        case MUIA_Prop_Visible: break;
        case MUIA_Prop_DeltaFactor: break;
        default:
            bug("[Listtree] OM_SET: passing to parent class %x\n", tag->ti_Tag);
        }
    }


    return DoSuperMethodA(cl, obj, (Msg) msg);
}

#define GETHANDLE(attrname)                                         \
    case(attrname):                                                 \
        bug("[Listtree] OM_GET:%s - unsupported\n", #attrname);     \
        break;


#define MUIA_List_Prop_Entries  /* PRIV */ \
    (MUIB_MUI | 0x0042a8f5)     /* .sg LONG  PRIV */
#define MUIA_List_Prop_Visible  /* PRIV */ \
    (MUIB_MUI | 0x004273e9)     /* .sg LONG  PRIV */
#define MUIA_List_Prop_First    /* PRIV */ \
    (MUIB_MUI | 0x00429df3)     /* .sg LONG  PRIV */

#define MUIA_List_VertProp_Entries  /* PRIV */ \
    MUIA_List_Prop_Entries     /* PRIV */
#define MUIA_List_VertProp_Visible  /* PRIV */ \
    MUIA_List_Prop_Visible     /* PRIV */
#define MUIA_List_VertProp_First  /* PRIV */ \
    MUIA_List_Prop_First       /* PRIV */

IPTR Listtree__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    switch (msg->opg_AttrID)
    {
    GETHANDLE(MUIA_Listtree_Active)
    GETHANDLE(MUIA_Listtree_Quiet)
    GETHANDLE(MUIA_Listtree_DoubleClick)
    GETHANDLE(MUIA_List_Active)
    GETHANDLE(MUIA_Frame)
    GETHANDLE(MUIA_List_VertProp_Entries)
    GETHANDLE(MUIA_List_VertProp_Visible)
    GETHANDLE(MUIA_List_VertProp_First)
    case MUIA_Disabled: break;
    case MUIA_Parent: break;
    case MUIA_Group_ChildList: break;
    case MUIA_Prop_First: break;
    case MUIA_Prop_DoSmooth: break;
    case MUIA_Listview_List: break;
    case MUIA_Virtgroup_Left: break;
    case MUIA_Virtgroup_Top: break;
    case 0x9d510020 /*MUIA_NListview_NList*/: break;
    default:
        bug("[Listtree] OM_GET: passing to parent class %x\n", msg->opg_AttrID);
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

#define METHODSTUB(methodname)                                          \
IPTR Listtree__##methodname(struct IClass *cl, Object *obj, Msg msg)    \
{                                                                       \
    bug("[Listtree] Usupported : %s\n", #methodname);                   \
    return (IPTR)FALSE;                                                 \
}

METHODSTUB(MUIM_Listtree_Rename)
METHODSTUB(MUIM_Listtree_Open)
METHODSTUB(MUIM_Listtree_Close)
METHODSTUB(MUIM_Listtree_TestPos)
METHODSTUB(MUIM_Listtree_SetDropMark)
METHODSTUB(MUIM_Listtree_FindName)
METHODSTUB(MUIM_List_TestPos)
METHODSTUB(MUIM_Listtree_Insert)
METHODSTUB(MUIM_Listtree_Remove)
METHODSTUB(MUIM_Listtree_GetNr)
METHODSTUB(MUIM_Listtree_GetEntry)
