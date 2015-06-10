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
#include <mui/NList_mcc.h>

#undef TNF_OPEN
#undef TNF_LIST
#include "Listtree_mcc.h"
#include "listtree_private.h"

#include <aros/debug.h>

struct MUIS_Listtree_TreeNodeInt
{
    struct MUIS_Listtree_TreeNode base;
    struct MUI_NListtree_TreeNode *ref;
};

static IPTR NotifySimulate_Function(struct Hook *hook, Object *obj, void ** msg)
{
    struct opSet setmsg;
    struct TagItem setti[] = {{0,0},{TAG_DONE, TAG_DONE}};

    IPTR attr           = (IPTR)msg[0];
    IPTR val            = (IPTR)msg[1];
    struct IClass * cl  = hook->h_Data;

    setmsg.MethodID         = OM_SET;
    setmsg.ops_AttrList     = setti;
    setmsg.ops_GInfo        = NULL;

    switch(attr)
    {
    case(MUIA_NListtree_Active):
        setti[0].ti_Tag     = MUIA_Listtree_Active;
        setti[0].ti_Data    = (IPTR)((struct MUI_NListtree_TreeNode *)val)->tn_User;
        break;
    default:
        bug("[Listtree] NotifySimulate_Function - unhandled attribute %x\n", attr);
    }

    /* Super method OM_SET call will go to Notify class and trigger notifications */
    return DoSuperMethodA(cl, obj, (Msg) &setmsg);
}

#define NEWHANDLE(attrname)                                         \
    case(attrname):                                                 \
        bug("[Listtree] OM_NEW:%s - unsupported\n", #attrname);     \
        break;

#define CONV(AATTR, BATTR)                                          \
    case(AATTR):                                                    \
        convtags[i].ti_Tag = BATTR;                                 \
        convtags[i++].ti_Data = tag->ti_Data;                       \
        break;

#define CONVFIN                     \
    convtags[i].ti_Tag  = TAG_DONE; \
    convtags[i].ti_Data = TAG_DONE;

/*** Methods ****************************************************************/
Object *Listtree__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Listtree_DATA *data = NULL;
    struct TagItem *tag;
    struct TagItem *tags;
    Object *nlisttree = NULL;
    struct TagItem convtags[20];
    LONG i = 0;

    /* Convert tags designated for NListtree */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
        CONV(MUIA_Frame,                    MUIA_Frame)
        CONV(MUIA_Listtree_Format,          MUIA_NListtree_Format)
        CONV(MUIA_Listtree_Title,           MUIA_NListtree_Title)
        CONV(MUIA_Listtree_DragDropSort,    MUIA_NListtree_DragDropSort)
        CONV(MUIA_List_Title,               MUIA_NList_Title)
        CONV(MUIA_List_DragSortable,        MUIA_NList_DragSortable)
//        CONV(MUIA_ContextMenu,              MUIA_ContextMenu) // FIXME causes a crash when right clicking
        CONV(MUIA_List_MinLineHeight,       MUIA_NList_MinLineHeight)
        }
    }
    CONVFIN

    /* TODO:
     * set up a DestructHook which will call proxy MUIS_Listtree_TreeNode destrhook and
     * free it afterwards
     */
    obj = (Object *) DoSuperNewTags(cl, obj, 0,
            Child, nlisttree = (Object *) NListtreeObject,
                                TAG_MORE, (IPTR)convtags,
                                End,
            );

    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);
    data->nlisttree = nlisttree;
    data->notifysimulatehook.h_Entry        = HookEntry;
    data->notifysimulatehook.h_SubEntry     = (HOOKFUNC)NotifySimulate_Function;
    data->notifysimulatehook.h_Data         = cl;

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
        case(MUIA_List_MinLineHeight):
        case(MUIA_ContextMenu):
        case(MUIA_List_DragSortable):
        case(MUIA_List_Title):
        case(MUIA_Listtree_DragDropSort):
        case(MUIA_Listtree_Format):
        case(MUIA_Listtree_Title):
        case(MUIA_Frame):
            /* Forwarded to NListtree */
            break;
        NEWHANDLE(MUIA_Listtree_DisplayHook)
        NEWHANDLE(MUIA_Listtree_SortHook)
        default:
            bug("[Listtree] OM_NEW: unhandled %x\n", tag->ti_Tag);
        }
    }

    /* Setup notification forwarding */
    DoMethod(data->nlisttree, MUIM_Notify, MUIA_NListtree_Active, MUIV_EveryTime,
            obj, 4, MUIM_CallHook, &data->notifysimulatehook, MUIA_NListtree_Active, MUIV_TriggerValue);

    return obj;
}

#define SETHANDLE(attrname)                                         \
    case(attrname):                                                 \
        bug("[Listtree] OM_SET:%s - unsupported\n", #attrname);     \
        break;

#define FORWARDSET(AATTR, BATTR)                    \
    case(AATTR):                                    \
        set(data->nlisttree, BATTR, tag->ti_Data);  \
        break;

#define IGNORESET(AATTR)    case(AATTR): break;

IPTR Listtree__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct TagItem      *tstate = msg->ops_AttrList;
    struct TagItem      *tag;

    if (!data->nlisttree)
        return DoSuperMethodA(cl, obj, (Msg) msg);

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        FORWARDSET(MUIA_Listtree_Quiet, MUIA_NListtree_Quiet)
        FORWARDSET(MUIA_List_Active, MUIA_NList_Active)

        IGNORESET(MUIA_Listview_SelectChange)

        case(MUIA_Listtree_Active):
            set(data->nlisttree, MUIA_NListtree_Active,
                    ((struct MUIS_Listtree_TreeNodeInt *)tag->ti_Data)->ref);
            break;

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

#define FORWARDGET(AATTR, BATTR)                            \
    case(AATTR):                                            \
        *(msg->opg_Storage) = XGET(data->nlisttree, BATTR); \
        return TRUE;

IPTR Listtree__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);

    if (!data->nlisttree)
        return FALSE;

    switch (msg->opg_AttrID)
    {
    FORWARDGET(MUIA_Frame, MUIA_Frame)
    FORWARDGET(MUIA_Listtree_DoubleClick, MUIA_NListtree_DoubleClick)
    FORWARDGET(MUIA_List_Active, MUIA_NList_Active)
    FORWARDGET(MUIA_Listtree_Active, MUIA_NListtree_Active)
    FORWARDGET(MUIA_Listtree_Quiet, MUIA_NListtree_Quiet)
    FORWARDGET(MUIA_List_Visible, MUIA_NList_Visible)

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

METHODSTUB(MUIM_Listtree_Open)
METHODSTUB(MUIM_Listtree_Close)
METHODSTUB(MUIM_Listtree_SetDropMark)
METHODSTUB(MUIM_Listtree_FindName)

IPTR Listtree__MUIM_Listtree_Insert(struct IClass *cl, Object *obj, struct MUIP_Listtree_Insert *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct MUIS_Listtree_TreeNodeInt * _int = AllocPooled(data->pool, sizeof(struct MUIS_Listtree_TreeNodeInt));
    struct MUIS_Listtree_TreeNode * _return = NULL;
    struct MUI_NListtree_TreeNode * ln = NULL, * pn = NULL;

    if (_int == NULL)
        return (IPTR)NULL;

    _return =  &_int->base;

    _return->tn_Flags = (UWORD)msg->Flags;
    if (data->constrhook)
        _return->tn_User = (APTR)CallHookPkt(data->constrhook, data->pool, msg->User);
    else
        _return->tn_User = msg->User;

    switch((IPTR)msg->ListNode)
    {
    case(MUIV_Listtree_Insert_ListNode_Root):
    case(MUIV_Listtree_Insert_ListNode_Active):
        ln = msg->ListNode;
        break;
    default:
        ln = ((struct MUIS_Listtree_TreeNodeInt *)msg->ListNode)->ref;
    }

    switch((IPTR)msg->PrevNode)
    {
    case(MUIV_Listtree_Insert_PrevNode_Head):
    case(MUIV_Listtree_Insert_PrevNode_Tail):
    case(MUIV_Listtree_Insert_PrevNode_Active):
    case(MUIV_Listtree_Insert_PrevNode_Sorted):
        pn = msg->PrevNode;
        break;
    default:
        pn = ((struct MUIS_Listtree_TreeNodeInt *)msg->PrevNode)->ref;
    }

    _int->ref = (struct MUI_NListtree_TreeNode *)DoMethod(data->nlisttree,
                    MUIM_NListtree_Insert, msg->Name, _return, ln, pn, msg->Flags);

    _return->tn_Name = _int->ref->tn_Name;

    return (IPTR)_return;
}

IPTR Listtree__MUIM_Listtree_GetEntry(struct IClass *cl, Object *obj, struct MUIP_Listtree_GetEntry *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct MUI_NListtree_TreeNode * n = NULL;

    switch ((IPTR)msg->Node)
    {
    case(MUIV_Listtree_GetEntry_ListNode_Root):
    case(MUIV_Listtree_GetEntry_ListNode_Active):
        n = msg->Node;
        break;
    default:
        n = ((struct MUIS_Listtree_TreeNodeInt *)msg->Node)->ref;
    }

    struct MUI_NListtree_TreeNode * tn = (struct MUI_NListtree_TreeNode *) DoMethod(data->nlisttree,
            MUIM_NListtree_GetEntry, n, msg->Position, msg->Flags);

    if (tn)
        return (IPTR)tn->tn_User;
    else
        return (IPTR)NULL;
}

IPTR Listtree__MUIM_Listtree_Remove(struct IClass *cl, Object *obj, struct MUIP_Listtree_Remove *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);

    /* TODO: handle remaining enumeration values */
    if ((msg->ListNode == (APTR)MUIV_Listtree_Remove_ListNode_Root) &&
            ((msg->TreeNode == (APTR)MUIV_Listtree_Remove_TreeNode_Active) ||
             (msg->TreeNode == (APTR)MUIV_Listtree_Remove_TreeNode_All)))
    {
        /* Deallocating of MUIS_Listtree_TreeNode is happening in the DestructHook */
        return DoMethod(data->nlisttree, MUIM_NListtree_Remove, msg->ListNode, msg->TreeNode, msg->Flags);
    }

    /* TODO
     * add handling for cases where ListNode/TreeNode actually point to Treenode structure
     */
    bug("[Listtree] MUIM_Listtree_Remove unsupported code path Listnode: %x, Treenode: %x, Flags: %d\n", msg->ListNode, msg->TreeNode, msg->Flags);

    return (IPTR)FALSE;
}

IPTR Listtree__MUIM_List_TestPos(struct IClass *cl, Object *obj, struct MUIP_List_TestPos *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);

    struct MUI_NList_TestPos_Result res;
    if (DoMethod(data->nlisttree, MUIM_List_TestPos, msg->x, msg->y, &res))
    {
        msg->res->entry     = res.entry;
        msg->res->column    = res.column;
        msg->res->flags     = res.flags;
        msg->res->xoffset   = res.xoffset;
        msg->res->yoffset   = res.yoffset;
        return TRUE;
    }

    return FALSE;
}

IPTR Listtree__MUIM_Listtree_TestPos(struct IClass *cl, Object *obj, struct MUIP_Listtree_TestPos *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);

    struct MUI_NListtree_TestPos_Result res;
    struct MUIS_Listtree_TestPos_Result * _ret = (struct MUIS_Listtree_TestPos_Result *)msg->Result;

    _ret->tpr_TreeNode = NULL;

    DoMethod(data->nlisttree, MUIM_NListtree_TestPos, msg->X, msg->Y, &res);

    _ret->tpr_Flags     = res.tpr_Type;
    _ret->tpr_ListEntry = res.tpr_ListEntry;
    _ret->tpr_ListFlags = res.tpr_ListFlags;

    if (res.tpr_TreeNode != NULL)
    {
        _ret->tpr_TreeNode  = res.tpr_TreeNode->tn_User;
        return TRUE;
    }

    return FALSE;
}

IPTR Listtree__MUIM_Listtree_GetNr(struct IClass *cl, Object *obj, struct MUIP_Listtree_GetNr *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct MUI_NListtree_TreeNode * tn = NULL;

    switch((IPTR)msg->TreeNode)
    {
    case(MUIV_Listtree_GetNr_TreeNode_Active):
        tn = msg->TreeNode;
        break;
    default:
        tn = ((struct MUIS_Listtree_TreeNodeInt *)msg->TreeNode)->ref;
    }

    return DoMethod(data->nlisttree, MUIM_NListtree_GetNr, tn, msg->Flags);
}

IPTR Listtree__MUIM_Listtree_Rename(struct IClass *cl, Object *obj, struct MUIP_Listtree_Rename *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct MUI_NListtree_TreeNode * tn = NULL, * renamed = NULL;

    switch((IPTR)msg->TreeNode)
    {
    case(MUIV_Listtree_Rename_TreeNode_Active):
        tn = msg->TreeNode;
        break;
    default:
        tn = ((struct MUIS_Listtree_TreeNodeInt *)msg->TreeNode)->ref;
    }

    renamed = (struct MUI_NListtree_TreeNode *)DoMethod(data->nlisttree,
                 MUIM_NListtree_Rename, tn, msg->NewName, msg->Flags);

    if (renamed)
    {
        ((struct MUIS_Listtree_TreeNode *)renamed->tn_User)->tn_Name = renamed->tn_Name;
        return (IPTR)renamed->tn_User;
    }
    else
        return (IPTR)NULL;
}

IPTR Listtree__MUIM_List_Redraw(struct IClass *cl, Object *obj, struct MUIP_List_Redraw *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct MUI_NListtree_TreeNode * entry = msg->entry ?
            ((struct MUIS_Listtree_TreeNodeInt *)msg->entry)->ref : NULL;

    switch(msg->pos)
    {
    case(MUIV_List_Redraw_Entry):
        return DoMethod(data->nlisttree, MUIM_NList_RedrawEntry, entry);
    default:
        return DoMethod(data->nlisttree, MUIM_NList_Redraw, msg->pos);
    }
}
