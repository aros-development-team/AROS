/*
    Copyright (C) 2012-2020, The AROS Development Team. All rights reserved.
*/

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <clib/alib_protos.h>
#include <mui/NListtree_mcc.h>
#include <mui/NList_mcc.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
    APTR            nlistimg;
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

/* NotifySimulate_Function
 * Translate Active, DoubleClick or Quiet child notifications into
 * superclass OM_SET calls on the outer object. Active translates the
 * internal node to its tn_User proxy. Borrow the two-slot message and live
 * hook class; return superclass result. This does not fix the raw
 * internal-node result of OM_GET.
 */
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

/* DisplayHook_Proxy
 * Synchronize flags and invoke the borrowed display hook with the column
 * array as object and proxy node as message, or NULL for the title. Return
 * hook result. The adapter then rewrites the first image escape in each
 * column in place. Require terminated column pointers and writable, valid
 * image-escape strings with live matching ListImage handles. No generic
 * string validation or reliable address-width conversion is supplied.
 */
static IPTR DisplayHook_Proxy(struct Hook *hook, Object *obj, struct MUIP_NListtree_DisplayMessage *msg)
{
    struct Hook * displayhook = (struct Hook *)hook->h_Data;
    APTR tn = NULL;
    IPTR _ret = 0;
    int i = 0;
    STRPTR column;

    if (!displayhook)
        return 0;

    SYNC_TREENODE_FLAGS(msg->TreeNode);

    tn  = msg->TreeNode ? msg->TreeNode->tn_User : NULL;

    _ret = CallHookPkt(displayhook, msg->Array, tn);

    while ((column = msg->Array[i++]) != NULL)
    {
        char * s = NULL;
        /* Unholy Hack: see comment in in MUIM_List_CreateImage
         * Modify the pointer value given by user in display hook. User was thinking he was given
         * correct structure by MUIM_List_CreateImage, but that is not true for internal NListtree.
         * Modify the address so it can be used by NListtree. This works only because the msg->Array
         * buffer in case of O[] has to be dinamically allocated and writable (so that user can put
         * address there)
         */
        if ((s = strstr(column, "O[")) != NULL)
        {
            char tmp[16], *e;
            int len_o, len_h;

            s += 2;
            e = strchr(s, ']');
            len_o = e - s;

            struct ListImage *li = (struct ListImage *)strtoul(s, NULL, 16);
            sprintf(tmp, "%lx", li->nlistimg);
            len_h = strlen(tmp);

            if (len_o == len_h)
                memcpy(s, tmp, len_h);
            else if (len_o == len_h + 1)
            {
                memcpy(s, tmp, len_h);
                s[len_h] = ']'; s[len_h + 1] = ' ';
            }
            else
            {
                bug("[Listtree] CreateImage workaround BROKEN, see comment in code!\n");
                /* Doing nothing will lead to NList using original address as BitMapImage. It will
                 * then check BitMapImage->control (first ULONG) and will find it 0(NULL) so neither
                 * MUIA_Image_Spec nor MUIM_NList_CreateImage in which case it will just ingore
                 * the object and not render anything.
                 */
            }
        }
    }

    return _ret;
}

/* SortHook_Proxy
 * Synchronize both node flags and invoke the borrowed sort hook with proxy
 * node one as object and proxy node two as message. Return its comparison
 * result, or zero without a hook. Do not remove or invalidate nodes during
 * comparison.
 */
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

/* DestructHook_Proxy
 * Invoke the optional borrowed destructor with pool as object and proxy
 * tn_User as message, then free the proxy from that pool. Require a
 * matching live proxy and pool. Return zero; caller aliases become invalid.
 * Payload ownership follows the application hook.
 */
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

/* ConstructHook_Proxy
 * Allocate a cleared proxy from the instance pool. Invoke the optional
 * constructor with pool as object and incoming payload as message, storing
 * its result in tn_User; otherwise retain the payload. Return proxy or NULL
 * on allocation failure. A NULL application-hook result is stored, not
 * treated as construction failure. Backend reference, name and flags are
 * filled only after Insert returns.
 */
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
/* Listtree__OM_NEW
 * Create the internal NListtree subclass, pass selected tags to it and
 * ContextMenu to List, allocate the proxy pool and install
 * hook/notification bridges. Borrow application hooks until disposal.
 * Return superclass-created object or NULL. Child creation and pool failure
 * are not fully checked or unwound; fixed 20-slot tag arrays have no
 * overflow guard. Do not assume general inherited-tag forwarding.
 */
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

/* Listtree__OM_DISPOSE
 * Save the pool pointer, dispose the superclass and child first, then
 * delete the pool. Child destruction hooks need the pool during superclass
 * disposal. Return superclass result; all proxies and pool allocations
 * become invalid.
 */
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

/* Listtree__OM_SET
 * Translate Quiet, List_Active and selected scroll attributes to the child.
 * Active dereferences a real Listtree proxy; it does not handle NULL or
 * sentinel values. Replace List_First tags with TAG_IGNORE, then forward
 * the original list to the superclass. Borrow writable tags; NoNotify is
 * not propagated into separate child set calls.
 */
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
        case MUIA_Draggable:
        case MUIA_Listview_DragType:
            break;

        default:
            D(bug("[Listtree] OM_SET: passing to parent class %x\n", tag->ti_Tag));
        }
    }


    return DoSuperMethodA(cl, obj, (Msg) msg);
}

#define FORWARDGET(AATTR, BATTR)                            \
    case(AATTR):                                            \
        *(msg->opg_Storage) = XGET(data->nlisttree, BATTR); \
        return TRUE;

/* Listtree__OM_GET
 * Read supported child attributes into valid storage and return TRUE.
 * Active is returned as an untranslated internal NListtree pointer. Return
 * FALSE without a child; delegate other getters. Returned pointers are
 * borrowed, not new nodes.
 */
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

/* Listtree__MUIM_Listtree_Insert
 * Translate documented parent/predecessor selectors or live same-tree
 * proxies, then call NListtree_Insert. On a returned node synchronize proxy
 * name/flags and attach its backend reference. Return proxy or NULL. Proxy
 * allocation failures are not fully handled by the backend/adapter path; no
 * complete low-memory recovery guarantee.
 */
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

/* Listtree__MUIM_Listtree_GetEntry
 * Translate Root/Active or a live proxy and delegate position/flags.
 * Synchronize found flags and return its borrowed proxy or NULL. The hidden
 * root has no proxy. Do not use the raw Active getter result as an input
 * proxy.
 */
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

/* Listtree__MUIM_Listtree_Remove
 * Translate supported list/node selectors or live proxies and delegate
 * removal. Return backend result, not a count or success guarantee.
 * Destruction releases proxy and application-owned payload according to the
 * destructor hook; invalidate all removed-node aliases.
 */
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

/* Listtree__MUIM_List_TestPos
 * Call the child List_TestPos into temporary storage. On success copy
 * entry, column, flags and offsets to valid caller storage and return TRUE;
 * otherwise return FALSE without copying. Borrow object and result storage.
 */
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

/* Listtree__MUIM_Listtree_TestPos
 * Require writable Result storage. Initialize its node to NULL, delegate
 * hit testing, copy type/list fields and translate a found node to its
 * borrowed proxy after flag synchronization. Return whether a node exists,
 * ignoring backend return value.
 */
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

/* Listtree__MUIM_Listtree_GetNr
 * Translate Active or a real proxy and forward flags. Return backend
 * position/count/boolean. No NULL/root selector is accepted by this
 * adapter. Active requires an actual active node for backend paths that
 * dereference it.
 */
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

/* Listtree__MUIM_Listtree_Rename
 * Translate Active or a live proxy, delegate and synchronize returned name.
 * Return proxy or NULL. User replacement runs destructor/constructor and
 * can replace the proxy without restoring its backend reference or flags;
 * do not treat that path as a supported proxy-preserving operation.
 */
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

/* Listtree__MUIM_List_Redraw
 * Translate a non-NULL entry pointer before examining pos. For Redraw_Entry
 * call NList_RedrawEntry; otherwise delegate pos to NList_Redraw. Even for
 * other positions, entry must be NULL or a live proxy. Return backend
 * result.
 */
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

/* Listtree__MUIM_Listtree_Open
 * Translate Root/Parent/Active and Head/Tail/Active/All selectors or real
 * proxies, then delegate expansion with unchanged flags. Return backend
 * result. Do not assume cached proxy flags update immediately.
 */
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

/* Listtree__MUIM_Listtree_FindName
 * Translate Root/Active or a live list proxy, delegate name search and
 * return a found borrowed proxy or NULL. No local flag synchronization
 * occurs. Require a readable name and valid search base.
 */
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

/* DoSetupMethod
 * Assign the supplied render information to the object before invoking
 * MUIM_Setup. Borrow a live object and valid rendering context; return
 * setup result. Caller owns matched cleanup and lifetime.
 */
IPTR DoSetupMethod(Object * obj, struct MUI_RenderInfo * info)
{
    /* MUI set the correct render info *before* it calls MUIM_Setup so please
     * only use this function instead of DoMethodA() */
    muiRenderInfo(obj) = info;
    return DoMethod(obj, MUIM_Setup, (IPTR) info);
}

/* Listtree__MUIM_List_CreateImage
 * Require the outer object to be set up, otherwise return zero. Create
 * backend image, allocate a List-compatible wrapper, connect and set up the
 * borrowed image object. Return the wrapper for matching DeleteImage.
 * Allocation and setup results are not fully checked; no complete failure
 * rollback is supplied.
 */
IPTR Listtree__MUIM_List_CreateImage(struct IClass *cl, Object *obj, struct MUIP_List_CreateImage *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);

    if (!(_flags(obj) & MADF_SETUP))
        return 0;

    IPTR _ret = DoMethod(data->nlisttree, MUIM_NList_CreateImage, msg->obj, msg->flags);

    /* There is a use case where an image object created in a Listtree can be passed as O[address]
     * in the text in the display callback of List. The way to trigger this usecase is on Odyssey Web
     * Browser to take a link from a "Bookmarks" Listree and assign it to "Quick Links", which is a List.
     *
     * Since Listtree just wraps around NListtree and the return structures from List_CreateImage and
     * NList_CreateImage are different, this would normally not work.
     *
     * The workaround is as follows:
     *  1) create List-compatible return structure here, so that List-side works correctly
     *  2) have that structure contain pointer to NList-compatible structure
     *  3) Unholy Hack: in display hook, modify the address given by user to point to NList-compatible
     *     structure, assuming List-compatible structure was given
     *  4) Keep fingers crossed
     */
    struct ListImage *li = AllocPooled(data->pool, sizeof(struct ListImage));
    li->node.mln_Succ = NULL;
    li->node.mln_Pred = NULL;
    li->obj = msg->obj;
    li->nlistimg = (APTR)_ret;

    /* Setup the msg->obj as the List is doing */
    DoMethod(li->obj, MUIM_ConnectParent, (IPTR) obj);
    DoSetupMethod(li->obj, muiRenderInfo(obj));

    return (IPTR)li;
}

/* Listtree__MUIM_List_DeleteImage
 * Accept NULL as a no-op. Otherwise require a live handle from this object,
 * disconnect its image, free the wrapper and delegate backend deletion.
 * Return backend result. Do not reuse the handle or delete twice; this
 * wrapper does not itself dispose the image object.
 */
IPTR Listtree__MUIM_List_DeleteImage(struct IClass *cl, Object *obj, struct MUIP_List_DeleteImage *msg)
{
    struct Listtree_DATA *data = INST_DATA(cl, obj);
    struct ListImage * li = (struct ListImage *)msg->listimg;
    APTR nlistimg = NULL;

    if (!li)
        return 0;

    /* DoMethod(li->obj, MUIM_Cleanup); // Called in MUIM_NList_DeleteImage */
    DoMethod(li->obj, MUIM_DisconnectParent);
    nlistimg = li->nlistimg;
    FreePooled(data->pool, li, sizeof(struct ListImage));

    return DoMethod(data->nlisttree, MUIM_NList_DeleteImage, nlistimg);
}

/* Listtree__MUIM_Listtree_Close
 * Translate supported list/node selectors or live proxies, then delegate
 * collapse with unchanged flags. Return backend result. Parent resolution
 * differs from Open in the backend; cached proxy flags need later
 * synchronization.
 */
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

/* MUIM_CreateDragImage
 * Forward the unchanged drag-image creation message to the child and return
 * its result. Image ownership follows the child implementation.
 */
FORWARDNLISTTREEMETHOD(MUIM_CreateDragImage)
/* MUIM_DeleteDragImage
 * Forward the unchanged drag-image deletion message to the child. Require a
 * matching live drag image; return child result.
 */
FORWARDNLISTTREEMETHOD(MUIM_DeleteDragImage)

/* Listtree__MUIM_Notify
 * Additionally register List_Prop_First notifications on the child, then
 * register all notifications through the superclass. List_First logs that
 * notifications are not fired. Return superclass result; no automatic
 * mapping of every inherited list notification.
 */
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
/* MUIM_Listtree_SetDropMark
 * Unsupported operation: log a diagnostic and return FALSE. Entry and
 * Values do not change a drop mark.
 */
METHODSTUB(MUIM_Listtree_SetDropMark)
