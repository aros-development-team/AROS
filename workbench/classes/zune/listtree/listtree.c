/*
    Copyright © 2012-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

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
#include "support.h"

#include <aros/debug.h>

#define MUIA_List_ListArea      (MUIB_List | 0x00000003)

#define MUIA_List_Prop_Entries  (MUIB_MUI | 0x0042a8f5)     /* .sg LONG  PRIV */
#define MUIA_List_Prop_Visible  (MUIB_MUI | 0x004273e9)     /* .sg LONG  PRIV */
#define MUIA_List_Prop_First    (MUIB_MUI | 0x00429df3)     /* .sg LONG  PRIV */

#define MUIA_List_VertProp_Entries  MUIA_List_Prop_Entries     /* PRIV */
#define MUIA_List_VertProp_Visible  MUIA_List_Prop_Visible     /* PRIV */
#define MUIA_List_VertProp_First    MUIA_List_Prop_First       /* PRIV */

struct ListImage
{
    struct MinNode  node;
    Object          *obj;
};

#define MADF_SETUP             (1<< 28) /* PRIV - zune-specific */

/* Relations:
 * MUIS_Listtree_Treenode -> MUI_NListtree_Treenode via MUIS_Listtree_TreeNodeInt.ref
 * MUI_NListtree_Treenode -> MUIS_Listtree_Treenode via MUI_NListtree_Treenode.tn_User
 */
struct MUIS_Listtree_TreeNodeInt
{
    struct MUIS_Listtree_TreeNode base;
    struct MUI_NListtree_TreeNode *ref;
};

#define SYNC_TREENODE_FLAGS(tn)                                                 \
   if (tn && tn->tn_User)                                                       \
      ((struct MUIS_Listtree_TreeNode *)tn->tn_User)->tn_Flags = tn->tn_Flags;

#define SYNC_TREENODE_NAME(tn)                                                  \
   if (tn && tn->tn_User)                                                       \
      ((struct MUIS_Listtree_TreeNode *)tn->tn_User)->tn_Name = tn->tn_Name;

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
        setti[0].ti_Data    = val ? (IPTR)((struct MUI_NListtree_TreeNode *)val)->tn_User : 0;
        break;
    case(MUIA_NListtree_DoubleClick):
        setti[0].ti_Tag     = MUIA_Listtree_DoubleClick;
        setti[0].ti_Data    = val;
        break;
    case(MUIA_NListtree_Quiet):
        setti[0].ti_Tag     = MUIA_Listtree_Quiet;
        setti[0].ti_Data    = val;
        break;
    default:
        bug("[Listtree] NotifySimulate_Function - unhandled attribute %08x\n", (ULONG)attr);
    }

    /* Super method OM_SET call will go to Notify class and trigger notifications */
    return DoSuperMethodA(cl, obj, (Msg) &setmsg);
}

static IPTR DisplayHook_Proxy(struct Hook *hook, Object *obj, struct MUIP_NListtree_DisplayMessage *msg)
{
    struct Hook * displayhook = (struct Hook *)hook->h_Data;
    APTR tn = NULL;

    if (!displayhook)
        return 0;

    SYNC_TREENODE_FLAGS(msg->TreeNode);

    tn  = msg->TreeNode ? msg->TreeNode->tn_User : NULL;

    return CallHookPkt(displayhook, msg->Array, tn);
}

static IPTR SortHook_Proxy(struct Hook *hook, Object *obj, struct MUIP_NListtree_CompareMessage *msg)
{
    struct Hook * sorthook = (struct Hook *)hook->h_Data;
    APTR tn1 = NULL, tn2 = NULL;

    if (!sorthook)
        return 0;

    SYNC_TREENODE_FLAGS(msg->TreeNode1);
    SYNC_TREENODE_FLAGS(msg->TreeNode2);

    tn1 = msg->TreeNode1 ? msg->TreeNode1->tn_User : NULL;
    tn2 = msg->TreeNode2 ? msg->TreeNode2->tn_User : NULL;

    return CallHookPkt(sorthook, tn1, tn2);
}

static IPTR DestructHook_Proxy(struct Hook *hook, Object *obj, struct MUIP_NListtree_DestructMessage *msg)
{
    struct Listtree_DATA * data = (struct Listtree_DATA *)hook->h_Data;
    struct MUIS_Listtree_TreeNode * tn = (struct MUIS_Listtree_TreeNode *)msg->UserData;
    if (!data)
        return 0;

    if (data->destrhook && tn)
        CallHookPkt(data->destrhook, data->pool, tn->tn_User);

    FreePooled(data->pool, tn, sizeof(struct MUIS_Listtree_TreeNodeInt));

    return 0;
}

static IPTR ConstructHook_Proxy(struct Hook *hook, Object *obj, struct MUIP_NListtree_ConstructMessage *msg)
{
    struct Listtree_DATA * data = (struct Listtree_DATA *)hook->h_Data;
    struct MUIS_Listtree_TreeNode * tn = NULL;
    if(!data)
        return 0;

    tn = AllocPooled(data->pool, sizeof(struct MUIS_Listtree_TreeNodeInt));

    if (tn == NULL)
        return 0;

    if (data->constrhook)
        tn->tn_User = (APTR)CallHookPkt(data->constrhook, data->pool, msg->UserData);
    else
        tn->tn_User = msg->UserData;

    return (IPTR)tn;
}

#define CONV(AATTR, BATTR)                                          \
    case(AATTR):                                                    \
        convtags[i].ti_Tag = BATTR;                                 \
        convtags[i++].ti_Data = tag->ti_Data;                       \
        break;

#define COPY(AATTR)                                                 \
    case(AATTR):                                                    \
        supertags[i].ti_Tag = AATTR;                                \
        supertags[i++].ti_Data = tag->ti_Data;                      \
        break;

#define NOTIFY_FORWARD(AATTR)                                       \
    DoMethod(data->nlisttree, MUIM_Notify, AATTR, MUIV_EveryTime,   \
        obj, 4, MUIM_CallHook, &data->notifysimulatehook, AATTR, MUIV_TriggerValue);

/*** Methods ****************************************************************/
Object *Listtree__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Listtree_DATA *data = NULL;
    struct TagItem *tag;
    struct TagItem *tags;
    Object *nlisttree = NULL;
    struct TagItem convtags[20];
    struct TagItem supertags[20];
    LONG i;

    /* Convert tags designated for NListtree */
    for (i = 0, tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
        CONV(MUIA_Frame,                    MUIA_Frame)
        CONV(MUIA_Listtree_Format,          MUIA_NListtree_Format)
        CONV(MUIA_Listtree_Title,           MUIA_NListtree_Title)
        CONV(MUIA_Listtree_DragDropSort,    MUIA_NListtree_DragDropSort)
        CONV(MUIA_List_Title,               MUIA_NList_Title)
        CONV(MUIA_List_DragSortable,        MUIA_NList_DragSortable)
        CONV(MUIA_List_MinLineHeight,       MUIA_NList_MinLineHeight)
        }
    }
    convtags[i].ti_Tag  = TAG_DONE;

    /* Copy tags designated for super class */
    for (i = 0, tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
        COPY(MUIA_ContextMenu) /* ContextMenuBuild/Choice will be called on child classes of Listtree */
        }
    }
    supertags[i].ti_Tag  = TAG_DONE;

    nlisttree = (Object *) NewObjectA(CL_NListtreeInt->mcc_Class, NULL, convtags);

    obj = (Object *) DoSuperNewTags(cl, obj, 0,
            MUIA_List_ListArea, nlisttree,
            TAG_MORE, (IPTR)supertags,
            TAG_DONE);

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
        case(MUIA_Listtree_DisplayHook):
            data->displayhook = (struct Hook *)tag->ti_Data;
            break;
        case(MUIA_Listtree_SortHook):
            data->sorthook = (struct Hook *)tag->ti_Data;
            break;

        /* Forwarded to NListtree */
        case(MUIA_List_MinLineHeight):
        case(MUIA_List_DragSortable):
        case(MUIA_List_Title):
        case(MUIA_Listtree_DragDropSort):
        case(MUIA_Listtree_Format):
        case(MUIA_Listtree_Title):
        case(MUIA_Frame):
            break;

        /* Forwarded to super class */
        case(MUIA_ContextMenu):
            break;

        default:
            bug("[Listtree] OM_NEW: unhandled %x\n", tag->ti_Tag);
        }
    }

    /* Setup connection */
    set(data->nlisttree, MUIA_NListtreeInt_Listtree, obj);

    /* Setup root node */
    {
        /*
         * Leave the tn_User of root node as NULL. It is expected that
         * parent of first level item is returned as NULL in Listtree
         */
    }

    /* Setup hook proxies */
    if (data->displayhook)
    {
        data->displayhookproxy.h_Entry      = HookEntry;
        data->displayhookproxy.h_SubEntry   = (HOOKFUNC)DisplayHook_Proxy;
        data->displayhookproxy.h_Data       = data->displayhook;
        nnset(data->nlisttree, MUIA_NListtree_DisplayHook, &data->displayhookproxy);
    }
    if (data->sorthook)
    {
        data->sorthookproxy.h_Entry      = HookEntry;
        data->sorthookproxy.h_SubEntry   = (HOOKFUNC)SortHook_Proxy;
        data->sorthookproxy.h_Data       = data->sorthook;
        nnset(data->nlisttree, MUIA_NListtree_CompareHook, &data->sorthookproxy);
    }

    /* Construct hook is mandatory to allocate proxy structures */
    {
        data->constructhookproxy.h_Entry      = HookEntry;
        data->constructhookproxy.h_SubEntry   = (HOOKFUNC)ConstructHook_Proxy;
        data->constructhookproxy.h_Data       = data;
        nnset(data->nlisttree, MUIA_NListtree_ConstructHook, &data->constructhookproxy);
    }
    /* Destroy hook is mandatory to free proxy structures */
    {
        data->destructhookproxy.h_Entry      = HookEntry;
        data->destructhookproxy.h_SubEntry   = (HOOKFUNC)DestructHook_Proxy;
        data->destructhookproxy.h_Data       = data;
        nnset(data->nlisttree, MUIA_NListtree_DestructHook, &data->destructhookproxy);
    }

    /* Setup notification forwarding */
    NOTIFY_FORWARD(MUIA_NListtree_Active)
    NOTIFY_FORWARD(MUIA_NListtree_DoubleClick)
    NOTIFY_FORWARD(MUIA_NListtree_Quiet)

    return obj;
}

IPTR Listtree__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);

    APTR pool = data->pool;
    IPTR result =  DoSuperMethodA(cl, obj, msg);

    /* Destruct hook called by dispose on NListree will need the pool,
     * so destroy it only after super dispose is called */
    DeletePool(pool);

    return result;
}

#define FORWARDSET(AATTR, BATTR)                    \
    case(AATTR):                                    \
        set(data->nlisttree, BATTR, tag->ti_Data);  \
        break;

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
        FORWARDSET(MUIA_List_Prop_First, MUIA_NList_Prop_First)
        FORWARDSET(MUIA_List_Prop_Visible, MUIA_NList_Prop_Visible)

        case(MUIA_Listtree_Active):
            set(data->nlisttree, MUIA_NListtree_Active,
                    ((struct MUIS_Listtree_TreeNodeInt *)tag->ti_Data)->ref);
            break;

        /* Setting MUIA_List_First causes weird behaviour of scroll bar */
        case(MUIA_List_First):
            /* set(data->nlisttree, MUIA_NList_First, tag->ti_Data); */ /* Don't set directly */
            tag->ti_Tag = TAG_IGNORE; /* Don't set via forward via Group(List)->NListtree */
            break;


        case MUIA_Listtree_DoubleClick:
        case MUIA_Listview_SelectChange:
        case MUIA_Prop_First:
        case MUIA_Prop_DoSmooth:
        case MUIA_NoNotify:
        case MUIA_Prop_Entries:
        case MUIA_Prop_Visible:
        case MUIA_Prop_DeltaFactor:
        case MUIA_Timer:
        case MUIA_Selected:
        case MUIA_Pressed:
            break;

        default:
            bug("[Listtree] OM_SET: passing to parent class %x\n", tag->ti_Tag);
        }
    }


    return DoSuperMethodA(cl, obj, (Msg) msg);
}

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
    FORWARDGET(MUIA_List_VertProp_First, MUIA_NList_Prop_First)
    FORWARDGET(MUIA_List_VertProp_Entries, MUIA_NList_Prop_Entries)
    FORWARDGET(MUIA_List_VertProp_Visible, MUIA_NList_Prop_Visible)

    case MUIA_Disabled:
    case MUIA_Parent:
    case MUIA_Group_ChildList:
    case MUIA_Prop_First:
    case MUIA_Prop_DoSmooth:
    case MUIA_Listview_List:
    case MUIA_Virtgroup_Left:
    case MUIA_Virtgroup_Top:
    case 0x9d510020 /*MUIA_NListview_NList*/:
    case MUIA_Listview_DoubleClick:
    case MUIA_Listview_SelectChange:
    case MUIA_Timer:
    case MUIA_Selected:
        break;

    default:
        bug("[Listtree] OM_GET: passing to parent class %x\n", msg->opg_AttrID);
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR Listtree__MUIM_Listtree_Insert(struct IClass *cl, Object *obj, struct MUIP_Listtree_Insert *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct MUI_NListtree_TreeNode * ln = NULL, * pn = NULL, * created = NULL;

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

    created = (struct MUI_NListtree_TreeNode *)DoMethod(data->nlisttree,
                    MUIM_NListtree_Insert, msg->Name, msg->User, ln, pn, msg->Flags);

    if (created)
    {
        SYNC_TREENODE_FLAGS(created);
        SYNC_TREENODE_NAME(created);
        ((struct MUIS_Listtree_TreeNodeInt *)created->tn_User)->ref = created;
        return (IPTR)created->tn_User;
    }
    else
        return (IPTR)NULL;
}

IPTR Listtree__MUIM_Listtree_GetEntry(struct IClass *cl, Object *obj, struct MUIP_Listtree_GetEntry *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct MUI_NListtree_TreeNode * tn = NULL, * found = NULL;

    switch ((IPTR)msg->Node)
    {
    case(MUIV_Listtree_GetEntry_ListNode_Root):
    case(MUIV_Listtree_GetEntry_ListNode_Active):
        tn = msg->Node;
        break;
    default:
        tn = ((struct MUIS_Listtree_TreeNodeInt *)msg->Node)->ref;
    }

    found = (struct MUI_NListtree_TreeNode *) DoMethod(data->nlisttree,
                MUIM_NListtree_GetEntry, tn, msg->Position, msg->Flags);

    if (found)
    {
        SYNC_TREENODE_FLAGS(found);
        return (IPTR)found->tn_User;
    }
    else
        return (IPTR)NULL;
}

IPTR Listtree__MUIM_Listtree_Remove(struct IClass *cl, Object *obj, struct MUIP_Listtree_Remove *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct MUI_NListtree_TreeNode * tn = NULL, * ln = NULL;

    switch((IPTR)msg->ListNode)
    {
    case(MUIV_Listtree_Remove_ListNode_Root):
    case(MUIV_Listtree_Remove_ListNode_Active):
        ln = msg->ListNode;
        break;
    default:
        ln = ((struct MUIS_Listtree_TreeNodeInt *)msg->ListNode)->ref;
    }

    switch((IPTR)msg->TreeNode)
    {
    case(MUIV_Listtree_Remove_TreeNode_Head):
    case(MUIV_Listtree_Remove_TreeNode_Tail):
    case(MUIV_Listtree_Remove_TreeNode_Active):
    case(MUIV_Listtree_Remove_TreeNode_All):
        tn = msg->TreeNode;
        break;
    default:
        tn = ((struct MUIS_Listtree_TreeNodeInt *)msg->TreeNode)->ref;
    }

    /* Deallocating of MUIS_Listtree_TreeNode is happening in the DestructHook */
    return DoMethod(data->nlisttree, MUIM_NListtree_Remove, ln, tn, msg->Flags);

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
        SYNC_TREENODE_FLAGS(res.tpr_TreeNode);
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

IPTR Listtree__MUIM_Listtree_Open(struct IClass *cl, Object *obj, struct MUIP_Listtree_Open *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct MUI_NListtree_TreeNode * tn = NULL, * ln = NULL;

    switch((IPTR)msg->ListNode)
    {
    case(MUIV_Listtree_Open_ListNode_Root):
    case(MUIV_Listtree_Open_ListNode_Parent):
    case(MUIV_Listtree_Open_ListNode_Active):
        ln = msg->ListNode;
        break;
    default:
        ln = ((struct MUIS_Listtree_TreeNodeInt *)msg->ListNode)->ref;
    }

    switch((IPTR)msg->TreeNode)
    {
    case(MUIV_Listtree_Open_TreeNode_Head):
    case(MUIV_Listtree_Open_TreeNode_Tail):
    case(MUIV_Listtree_Open_TreeNode_Active):
    case(MUIV_Listtree_Open_TreeNode_All):
        tn = msg->TreeNode;
        break;
    default:
        tn = ((struct MUIS_Listtree_TreeNodeInt *)msg->TreeNode)->ref;
    }

    return DoMethod(data->nlisttree, MUIM_NListtree_Open, ln, tn, msg->Flags);
}

IPTR Listtree__MUIM_Listtree_FindName(struct IClass *cl, Object *obj, struct MUIP_Listtree_FindName *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct MUI_NListtree_TreeNode * ln = NULL, * found = NULL;

    switch((IPTR)msg->ListNode)
    {
    case(MUIV_Listtree_FindName_ListNode_Root):
    case(MUIV_Listtree_FindName_ListNode_Active):
        ln = msg->ListNode;
        break;
    default:
        ln = ((struct MUIS_Listtree_TreeNodeInt *)msg->ListNode)->ref;
    }

    found = (struct MUI_NListtree_TreeNode *) DoMethod(data->nlisttree, MUIM_NListtree_FindName,
                ln, msg->Name, msg->Flags);

    if (found)
        return (IPTR)found->tn_User;
    else
        return (IPTR)NULL;
}

IPTR DoSetupMethod(Object * obj, struct MUI_RenderInfo * info)
{
    /* MUI set the correct render info *before* it calls MUIM_Setup so please
     * only use this function instead of DoMethodA() */
    muiRenderInfo(obj) = info;
    return DoMethod(obj, MUIM_Setup, (IPTR) info);
}

IPTR Listtree__MUIM_List_CreateImage(struct IClass *cl, Object *obj, struct MUIP_List_CreateImage *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);

    if (!(_flags(obj) & MADF_SETUP))
        return 0;

    IPTR _ret = DoMethod(data->nlisttree, MUIM_NList_CreateImage, msg->obj, msg->flags);

    /* There is a use case where an image object created in a Listtree can be passed as O[address]
     * in the text in the display callback of List. Since Listtree just wraps around NListtree and the
     * return structures from List_CreateImage and NList_CreateImage are different, this would normally
     * not work. Luckily, List needs only the msg->obj and it is at the same offset in ListImage and
     * in structure returned by NList. The case will work as long as this is met.
     */
    struct ListImage * li = (struct ListImage *)_ret;
    if (li->obj != msg->obj)
        bug("[Listtree] CreateImage condition BROKEN, see comment in code!\n");

    /* Setup the msg->obj as the List is doing */
    DoMethod(li->obj, MUIM_ConnectParent, (IPTR) obj);
    DoSetupMethod(li->obj, muiRenderInfo(obj));

    return _ret;
}

IPTR Listtree__MUIM_List_DeleteImage(struct IClass *cl, Object *obj, struct MUIP_List_DeleteImage *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct ListImage * li = (struct ListImage *)msg->listimg;

    if (!li)
        return 0;

    /* DoMethod(li->obj, MUIM_Cleanup); // Called in MUIM_NList_DeleteImage */
    DoMethod(li->obj, MUIM_DisconnectParent);
    return DoMethod(data->nlisttree, MUIM_NList_DeleteImage, msg->listimg);
}

IPTR Listtree__MUIM_Listtree_Close(struct IClass *cl, Object *obj, struct MUIP_Listtree_Close *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct MUI_NListtree_TreeNode * tn = NULL, * ln = NULL;

    switch((IPTR)msg->ListNode)
    {
    case(MUIV_Listtree_Close_ListNode_Root):
    case(MUIV_Listtree_Close_ListNode_Parent):
    case(MUIV_Listtree_Close_ListNode_Active):
        ln = msg->ListNode;
        break;
    default:
        ln = ((struct MUIS_Listtree_TreeNodeInt *)msg->ListNode)->ref;
    }

    switch((IPTR)msg->TreeNode)
    {
    case(MUIV_Listtree_Close_TreeNode_Head):
    case(MUIV_Listtree_Close_TreeNode_Tail):
    case(MUIV_Listtree_Close_TreeNode_Active):
    case(MUIV_Listtree_Close_TreeNode_All):
        tn = msg->TreeNode;
        break;
    default:
        tn = ((struct MUIS_Listtree_TreeNodeInt *)msg->TreeNode)->ref;
    }

    return DoMethod(data->nlisttree, MUIM_NListtree_Close, ln, tn, msg->Flags);
}

#define FORWARDNLISTTREEMETHOD(methodname)                                          \
IPTR Listtree__##methodname(struct IClass *cl, Object *obj, Msg msg)                \
{                                                                                   \
    struct Listtree_DATA *data = INST_DATA(cl, obj);                                \
    return DoMethodA(data->nlisttree, msg);                                         \
}

FORWARDNLISTTREEMETHOD(MUIM_CreateDragImage)
FORWARDNLISTTREEMETHOD(MUIM_DeleteDragImage)

IPTR Listtree__MUIM_Notify(struct IClass *cl, Object *obj, struct MUIP_Notify *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);

    /* NList expects this notification to be set and uses its content */
    if (msg->TrigAttr == MUIA_List_Prop_First)
        DoMethodA(data->nlisttree, msg);

    if (msg->TrigAttr == MUIA_List_First)
        bug("Listtree.mcc: notifications on MUIA_List_First are not fired!\n");

    return DoSuperMethodA(cl, obj, msg);
}

#define METHODSTUB(methodname)                                          \
IPTR Listtree__##methodname(struct IClass *cl, Object *obj, Msg msg)    \
{                                                                       \
    bug("[Listtree] Usupported : %s\n", #methodname);                   \
    return (IPTR)FALSE;                                                 \
}

/* SetDropMark has no counterpart in NListtree and no documentation */
METHODSTUB(MUIM_Listtree_SetDropMark)
