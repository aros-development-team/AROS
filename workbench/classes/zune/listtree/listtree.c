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
METHODSTUB(MUIM_Listtree_Remove)
METHODSTUB(MUIM_Listtree_GetNr)

IPTR Listtree__MUIM_Listtree_Insert(struct IClass *cl, Object *obj, struct MUIP_Listtree_Insert *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct MUIS_Listtree_TreeNode * _return = AllocPooled(data->pool, sizeof(struct MUIS_Listtree_TreeNode));

    if (_return == NULL)
        return (IPTR)NULL;

    _return->tn_Flags = (UWORD)msg->Flags;
    if (msg->Name)
    {
        LONG len = strlen(msg->Name) + 1;
        _return->tn_Name = AllocPooled(data->pool, len);
        CopyMem(msg->Name, _return->tn_Name, len);
    }

    if (data->constrhook)
        _return->tn_User = (APTR)CallHookPkt(data->constrhook, data->pool, msg->User);
    else
        _return->tn_User = msg->User;

    /* TEMP */
    if (msg->ListNode != (APTR)MUIV_Listtree_Insert_ListNode_Root)
        bug("[Listtree] MUIM_Listtree_Insert - unhandled value of ListNode %x\n", msg->ListNode);

    if ((msg->PrevNode != (APTR)MUIV_Listtree_Insert_PrevNode_Tail)
        && (msg->PrevNode != (APTR)MUIV_Listtree_Insert_PrevNode_Sorted))
        bug("[Listtree] MUIM_Listtree_Insert - unhandled value of PrevNode %x\n", msg->PrevNode);
    /* TEMP */
    /* TODO
     * add handling for cases where ListNode and PrevNode actually point to Treenode structure
     * The internal TreeNode structure needs to have then the poiter to NListTree node to be
     * able to traslate. The pointer is acquired as return from below DoMethod call
     */

    DoMethod(data->nlisttree, MUIM_NListtree_Insert, _return->tn_Name, _return, msg->ListNode,
            msg->PrevNode, _return->tn_Flags);

    return (IPTR)_return;
}

IPTR Listtree__MUIM_Listtree_GetEntry(struct IClass *cl, Object *obj, struct MUIP_Listtree_GetEntry *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);

    /* TEMP */
    if ((msg->Node != (APTR)MUIV_Listtree_GetEntry_ListNode_Root)
        && (msg->Node != (APTR)MUIV_Listtree_GetEntry_ListNode_Active))
        bug("[Listtree] MUIM_Listtree_GetEntry - unhandled value of Node %x\n", msg->Node);
    /* TEMP */
    /* TODO
     * add handling for cases where Node actually point to Treenode structure
     */

    struct MUI_NListtree_TreeNode * tn = (struct MUI_NListtree_TreeNode *) DoMethod(data->nlisttree,
            MUIM_NListtree_GetEntry, msg->Node, msg->Position, msg->Flags);

    if (tn)
        return (IPTR)tn->tn_User;
    else
        return (IPTR)NULL;
}
