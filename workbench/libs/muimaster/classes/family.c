/*
    Copyright (C) 1999, David Le Corfec.
    Copyright (C) 2002-2026, The AROS Development Team.
    All rights reserved.

*/

#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "muimaster_intern.h"
#include "mui.h"

extern struct Library *MUIMasterBase;

struct MUI_FamilyData
{
    struct List children;
};

/****** Family.mui/MUIC_Family ************
*
*   NAME
*       MUIC_Family
*
*   SUPERCLASS
*       Notify.mui
*
*   DESCRIPTION
*       Maintain an ordered list of owned child objects. Construction starts
*       with an empty list and appends each non-NULL MUIA_Family_Child or
*       MUIA_Group_Child tag in order. If any child tag is NULL, construction
*       disposes of the family and all children successfully attached to it,
*       then returns NULL. If superclass construction fails, Family has not yet
*       attached the supplied children.
*       Disposal disposes of every child still in the list, then the Notify
*       superclass. Remove a child before disposing of it separately. Removing a
*       child transfers responsibility for its lifetime back to the caller. The
*       list and its operations are not locked; serialize traversal, mutation,
*       and disposal.
*       Family does not automatically forward ConnectParent or DisconnectParent
*       to children. AddHead, AddTail, and Insert call the child's ConnectParent
*       only if the family already has global info, and set its parent field
*       even without an application connection.
*
*   METHODS
*       The inherited user-data methods are extended as follows. FindUData
*       compares the family first, then sends the same message to children in
*       list order, returning the first nonzero object result or zero. Deeper
*       traversal depends on each child's implementation.
*       GetUData gets the attribute from the family if its UserData matches;
*       otherwise it returns TRUE on the first child returning nonzero. TRUE
*       indicates a match, not a successful attribute getter. Its udata message
*       field is ULONG. No match leaves caller storage untouched.
*       SetUData sets the family on a match, sends the method to every child,
*       and returns TRUE even if no object matches. Setter and child return
*       values are ignored.
*       SetUDataOnce sets the family on a match and returns TRUE, otherwise
*       visits children until one returns nonzero. It returns FALSE when none
*       match. FindUData, SetUData, and SetUDataOnce use IPTR comparison values.
*       Do not mutate or dispose of traversed children from callbacks during
*       these walks.
*       OM_ADDMEMBER uses AddTail; OM_REMMEMBER uses Remove. Version and
*       Revision both return 1 for Family. Other methods, including the Notify
*       persistence stubs, are inherited unless a subclass overrides them.
*
*   EXAMPLE
*       Each supplied child must be a live, unattached object:
*       family = MUI_NewObject(MUIC_Family,
*                             MUIA_Family_Child, (IPTR)child,
*                             TAG_DONE);
*       After success, disposing family also disposes child.
*
*/
/****** Family.mui/MUIA_Family_Child ************
*
*   NAME
*       MUIA_Family_Child
*
*   FUNCTION
*       Initialize the family with a child object. Repeat the tag to append
*       multiple children in tag order. MUIA_Group_Child is accepted as an alias
*       by this constructor.
*
*   INPUTS
*       Object pointer; initialize only. Each child must be live, unattached,
*       and suitable for Family membership.
*
*   NOTES
*       Successful attachment makes the family responsible for disposing of the
*       child. A NULL child tag makes construction fail after processing all
*       tags and disposing of children attached to the new family. If superclass
*       construction fails before attachment, the supplied children have not
*       been adopted. There is no Family setter or getter for this tag.
*
*/
/****** Family.mui/MUIA_Family_List ************
*
*   NAME
*       MUIA_Family_List
*
*   FUNCTION
*       Return a borrowed pointer to the internal child list.
*
*   INPUTS
*       Pointer to struct MinList; get only. The implementation stores an Exec
*       struct List and returns its address.
*
*   NOTES
*       The pointer is valid only while the family lives. Use NextObject to
*       traverse its BOOPSI children. Do not free the list or alter its links
*       directly; use Family methods so parent fields and ownership remain
*       consistent. Traversal must not overlap mutation or disposal.
*
*/
/****** Family.mui/MUIA_Family_ChildCount ************
*
*   NAME
*       MUIA_Family_ChildCount
*
*   FUNCTION
*       Count the current immediate children by walking the list and return the
*       count through the attribute getter.
*
*   INPUTS
*       LONG in the public header; get only. The implementation accumulates the
*       result in the IPTR storage supplied to OM_GET.
*
*   NOTES
*       An empty family returns zero. Descendants below immediate children are
*       not counted. This is a traversal, not a cached count, and has no
*       synchronization against concurrent changes.
*
*/

static const int __version = 1;
static const int __revision = 1;

/*  static void */
/*  debuglist(struct List *list) */
/*  { */
/*      g_print("list %p:\nlh_Head@%p = %p\nlh_Tail@%p = %p\nlh_TailPred@%p = %p\n", */
/*              list, &list->lh_Head, list->lh_Head, */
/*              &list->lh_Tail, list->lh_Tail, */
/*              &list->lh_TailPred, list->lh_TailPred); */
/*  } */

/*  static void */
/*  printlist (struct List *list) */
/*  { */
/*      struct Node *node; */

/*      debuglist(list); */

/*      for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ) */
/*      { */
/*          g_print("%s (ln_Succ@%p = %p | ln_Pred@%p = %p)\n", "node->ln_Name", */
/*                  &node->ln_Succ, node->ln_Succ, &node->ln_Pred, node->ln_Pred); */
/*      } */
/*      g_print("\n"); */
/*  } */

/*
 * OM_NEW
 */
/* Construct Notify and initialize an empty child list. Append non-NULL
 * Family_Child and Group_Child tags. Any NULL child marks failure: dispose
 * the constructed family and all attached children after processing tags,
 * then return zero. Return the family pointer otherwise. AddTail results
 * are ignored.
 */
IPTR Family__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_FamilyData *data;
    struct TagItem *tags;
    struct TagItem *tag;
    BOOL bad_children = FALSE;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;
    /*
     * Initial local instance data
     */
    data = INST_DATA(cl, obj);
    NewList(&(data->children));

    /*
     * parse initial taglist
     */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        if (tag->ti_Tag == MUIA_Family_Child
            || tag->ti_Tag == MUIA_Group_Child)
        {
            if (tag->ti_Data)   /* add child */
                DoMethod(obj, MUIM_Family_AddTail, tag->ti_Data);
            else                /* fail and dispose children */
            {
                bad_children = TRUE;
            }
        }
    }

    if (bad_children)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    return (IPTR) obj;
}


/*
 * OM_DISPOSE
 */
/* With all other users stopped, dispose each child still listed, then
 * dispose Notify and return its result. Removed children are not owned
 * here. List traversal assumes child disposal does not invalidate the next
 * child.
 */
IPTR Family__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_FamilyData *data = INST_DATA(cl, obj);
    Object *cstate = (Object *) data->children.lh_Head;
    Object *child;

    while ((child = NextObject(&cstate)))
    {
/*  g_print("Family_Dispose: dispose child %p\n", child); */
        MUI_DisposeObject(child);
    }

    return DoSuperMethodA(cl, obj, msg);
}


/*
 * OM_GET
 */
/* Return the borrowed child-list pointer, current immediate-child count, or
 * local Version/Revision through writable opg_Storage and return TRUE.
 * Forward all other attribute requests to Notify.
 */
IPTR Family__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_FamilyData *data = INST_DATA(cl, obj);
    IPTR *store = msg->opg_Storage;

    switch (msg->opg_AttrID)
    {
    case MUIA_Family_List:
        *store = (IPTR) & data->children;
        return TRUE;

    case MUIA_Version:
        *store = __version;
        return TRUE;

    case MUIA_Revision:
        *store = __revision;
        return TRUE;

    case MUIA_Family_ChildCount:
        {
            Object *cstate = (Object *) data->children.lh_Head;
            *store = 0;
            while (NextObject(&cstate))
                (*store)++;
            return TRUE;
        }
    }

    return (DoSuperMethodA(cl, obj, (Msg) msg));
}


/****** Family.mui/MUIM_Family_AddHead ************
*
*   NAME
*       MUIM_Family_AddHead
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Family_AddHead, (IPTR)child);
*
*   FUNCTION
*       Attach child as the first child of the family.
*
*   INPUTS
*       child - Live, unattached MUI object, or NULL.
*
*   RESULT
*       FALSE for NULL; otherwise TRUE. Child method results are not used as an
*       error indication.
*
*   NOTES
*       The family takes responsibility for child disposal. Its parent field is
*       set to the family. If the family has global info, the child receives
*       ConnectParent. AddTail uses the child's OM_ADDTAIL method; AddHead links
*       its BOOPSI node directly.
*
*   BUGS
*       Membership is not checked. Do not add an object already linked to this
*       or another list, and do not add the family itself or create ownership
*       cycles.
*
*/
IPTR Family__MUIM_AddHead(struct IClass *cl, Object *obj,
    struct MUIP_Family_AddHead *msg)
{
    struct MUI_FamilyData *data = INST_DATA(cl, obj);

    if (msg->obj)
    {
        AddHead(&(data->children), (struct Node *)_OBJECT(msg->obj));

        /* if we are in an application tree, propagate pointers */
        if (muiNotifyData(obj)->mnd_GlobalInfo)
        {
            DoMethod(msg->obj, MUIM_ConnectParent, (IPTR)obj);
        }

        /* Some apps (Odyssey) expect _parent() will work before group tree
         * is added to application tree */
        muiNotifyData(msg->obj)->mnd_ParentObject = obj;

        return TRUE;
    }
    else
        return FALSE;
}


/****** Family.mui/MUIM_Family_AddTail ************
*
*   NAME
*       MUIM_Family_AddTail
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Family_AddTail, (IPTR)child);
*
*   FUNCTION
*       Attach child as the last child of the family.
*
*   INPUTS
*       child - Live, unattached MUI object, or NULL.
*
*   RESULT
*       FALSE for NULL; otherwise TRUE. Child method results are not used as an
*       error indication.
*
*   NOTES
*       The family takes responsibility for child disposal. Its parent field is
*       set to the family. If the family has global info, the child receives
*       ConnectParent. AddTail uses the child's OM_ADDTAIL method; AddHead links
*       its BOOPSI node directly.
*
*   BUGS
*       Membership is not checked. Do not add an object already linked to this
*       or another list, and do not add the family itself or create ownership
*       cycles.
*
*/
IPTR Family__MUIM_AddTail(struct IClass *cl, Object *obj,
    struct MUIP_Family_AddTail *msg)
{
    struct MUI_FamilyData *data = INST_DATA(cl, obj);

    if (msg->obj)
    {
        D(bug("Family_AddTail(%p): obj=%p node=%p\n", obj, msg->obj,
                _OBJECT(msg->obj)));
        DoMethod(msg->obj, OM_ADDTAIL, (IPTR) & data->children);

        /* if we are in an application tree, propagate pointers */
        if (muiNotifyData(obj)->mnd_GlobalInfo)
        {
            DoMethod(msg->obj, MUIM_ConnectParent, (IPTR)obj);
        }

        /* Some apps (Odyssey) expect _parent() will work before group tree
         * is added to application tree */
        muiNotifyData(msg->obj)->mnd_ParentObject = obj;

        return TRUE;
    }
    else
        return FALSE;
}


/****** Family.mui/MUIM_Family_Insert ************
*
*   NAME
*       MUIM_Family_Insert
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Family_Insert, (IPTR)child,
*       (IPTR)pred);
*
*   FUNCTION
*       Attach child immediately after pred in the family list.
*
*   INPUTS
*       child - Live, unattached MUI object, or NULL.
*       pred - Non-NULL immediate child already in this family.
*
*   RESULT
*       FALSE for a NULL child; otherwise TRUE.
*
*   NOTES
*       The family adopts the child, sets its parent field, and sends
*       ConnectParent if global info is available. Neither predecessor
*       membership nor existing child membership is checked. Use AddHead to
*       insert at the beginning.
*
*   BUGS
*       The implementation applies _OBJECT to pred before calling Exec Insert.
*       Passing NULL as pred is not a supported way to request head insertion in
*       this implementation.
*
*/
IPTR Family__MUIM_Insert(struct IClass *cl, Object *obj,
    struct MUIP_Family_Insert *msg)
{
    struct MUI_FamilyData *data = INST_DATA(cl, obj);

    if (msg->obj)
    {
        Insert(&(data->children), (struct Node *)_OBJECT(msg->obj),
            (struct Node *)_OBJECT(msg->pred));

        /* if we are in an application tree, propagate pointers */
        if (muiNotifyData(obj)->mnd_GlobalInfo)
        {
            DoMethod(msg->obj, MUIM_ConnectParent, (IPTR)obj);
        }

        /* Some apps (Odyssey) expect _parent() will work before group tree
         * is added to application tree */
        muiNotifyData(msg->obj)->mnd_ParentObject = obj;

        return TRUE;
    }
    else
        return FALSE;
}


/****** Family.mui/MUIM_Family_Remove ************
*
*   NAME
*       MUIM_Family_Remove
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Family_Remove, (IPTR)child);
*
*   FUNCTION
*       Send DisconnectParent, clear the child's stored parent, then send
*       OM_REMOVE to unlink it.
*
*   INPUTS
*       child - Immediate child currently belonging to this family, or NULL.
*
*   RESULT
*       FALSE for NULL; otherwise TRUE, regardless of child method results.
*
*   NOTES
*       The child is not disposed of. After successful removal, the caller must
*       dispose of it or transfer it to another owner. A Notify child retains
*       its global-info pointer because Notify's DisconnectParent is a no-op.
*
*   BUGS
*       The family does not verify membership. Passing an unrelated object can
*       unlink it from another list or operate on invalid node links.
*
*/
IPTR Family__MUIM_Remove(struct IClass *cl, Object *obj,
    struct MUIP_Family_Remove *msg)
{
    /* struct MUI_FamilyData *data = INST_DATA(cl, obj);
       struct Node *node; */

    if (msg->obj)
    {
/*          D(bug("Family_Remove(%p): obj=%p\n", obj, msg->obj)); */
        DoMethod(msg->obj, MUIM_DisconnectParent);
        muiNotifyData(msg->obj)->mnd_ParentObject = NULL;
        DoMethod(msg->obj, OM_REMOVE);
        return TRUE;
    }
    else
        return FALSE;
}


/****** Family.mui/MUIM_Family_Sort ************
*
*   NAME
*       MUIM_Family_Sort
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Family_Sort, (IPTR)child1, ...,
*       (IPTR)NULL);
*
*   FUNCTION
*       Reset the child-list header and relink objects in the supplied order. No
*       comparator is called.
*
*   INPUTS
*       child1, ... - Every current immediate child exactly once, in the desired
*       order, followed by NULL. For an empty family, pass only NULL.
*
*   RESULT
*       Always TRUE after relinking.
*
*   NOTES
*       Ownership, parent fields, and global-info pointers are not changed. The
*       sequence is read directly from the method message and is not copied.
*
*   BUGS
*       The implementation does not check completeness, duplicates, membership,
*       or termination. Omitted children are not disposed of or disconnected but
*       disappear from the family list. Duplicates or foreign objects can
*       corrupt list ownership. Use this only to reorder the complete current
*       membership.
*
*/
IPTR Family__MUIM_Sort(struct IClass *cl, Object *obj,
    struct MUIP_Family_Sort *msg)
{
    struct MUI_FamilyData *data = INST_DATA(cl, obj);
    int i;

    NewList(&(data->children));
    for (i = 0; msg->obj[i]; i++)
    {
        AddTail(&(data->children), (struct Node *)_OBJECT(msg->obj[i]));
    }
    return TRUE;
}


/****** Family.mui/MUIM_Family_Transfer ************
*
*   NAME
*       MUIM_Family_Transfer
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Family_Transfer, (IPTR)family);
*
*   FUNCTION
*       Remove each current child from the source and append it to the
*       destination in source order.
*
*   INPUTS
*       family - Different, live Family-compatible destination object that
*       accepts every child. It must not be a source descendant that would
*       create an ownership cycle.
*
*   RESULT
*       Always TRUE after the traversal. Remove and AddTail results are ignored.
*
*   NOTES
*       Existing destination children remain before transferred children. The
*       destination becomes responsible for disposal; parent and connection
*       changes follow Remove and AddTail. No rollback or destination validation
*       is performed. Keep both containers and all children alive and avoid
*       callback-driven membership changes.
*
*   BUGS
*       The source and destination must differ. There is no self-transfer guard,
*       and a failed destination attachment is not reported or rolled back.
*
*/
IPTR Family__MUIM_Transfer(struct IClass *cl, Object *obj,
    struct MUIP_Family_Transfer *msg)
{
    struct MUI_FamilyData *data = INST_DATA(cl, obj);
    Object *cstate = (Object *) data->children.lh_Head;
    Object *child;

    while ((child = NextObject(&cstate)))
    {
        DoMethod(obj, MUIM_Family_Remove, (IPTR) child);
        DoMethod(msg->family, MUIM_Family_AddTail, (IPTR) child);
    }
    return TRUE;
}


/* Compare this family first, then query children in list order with the
 * same message. Return the first matching borrowed object pointer, or zero.
 * Keep membership stable during traversal; descendant search is delegated
 * to child implementations.
 */
IPTR Family__MUIM_FindUData(struct IClass *cl, Object *obj,
    struct MUIP_FindUData *msg)
{
    struct MUI_FamilyData *data = INST_DATA(cl, obj);
    Object *cstate = (Object *) data->children.lh_Head;
    Object *child;

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
        return (IPTR) obj;

    while ((child = NextObject(&cstate)))
    {
        Object *found = (Object *) DoMethodA(child, (Msg) msg);
        if (found)
            return (IPTR) found;
    }
    return 0;
}


/* On a local UserData match, get attr into caller storage and return TRUE
 * regardless of getter success. Otherwise return TRUE for the first nonzero
 * child result, or FALSE after all children. The message comparison value
 * is ULONG; keep membership stable.
 */
IPTR Family__MUIM_GetUData(struct IClass *cl, Object *obj,
    struct MUIP_GetUData *msg)
{
    struct MUI_FamilyData *data = INST_DATA(cl, obj);
    Object *cstate = (Object *) data->children.lh_Head;
    Object *child;

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
    {
        get(obj, msg->attr, msg->storage);
        return TRUE;
    }
    while ((child = NextObject(&cstate)))
        if (DoMethodA(child, (Msg) msg))
            return TRUE;

    return FALSE;
}


/* Set the family on a UserData match and forward the same message to every
 * child. Always return TRUE, ignoring setter and child results. Do not let
 * synchronous callbacks mutate the child traversal.
 */
IPTR Family__MUIM_SetUData(struct IClass *cl, Object *obj,
    struct MUIP_SetUData *msg)
{
    struct MUI_FamilyData *data = INST_DATA(cl, obj);
    Object *cstate = (Object *) data->children.lh_Head;
    Object *child;

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
        set(obj, msg->attr, msg->val);

    while ((child = NextObject(&cstate)))
        DoMethodA(child, (Msg) msg);

    return TRUE;
}


/* Set the family on a UserData match and return TRUE; otherwise stop at the
 * first nonzero child result. Return FALSE if all children return zero.
 * Setter success is not checked. Keep the child list stable during
 * callbacks.
 */
IPTR Family__MUIM_SetUDataOnce(struct IClass *cl, Object *obj,
    struct MUIP_SetUDataOnce *msg)
{
    struct MUI_FamilyData *data = INST_DATA(cl, obj);
    Object *cstate = (Object *) data->children.lh_Head;
    Object *child;

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
    {
        set(obj, msg->attr, msg->val);
        return TRUE;
    }
    while ((child = NextObject(&cstate)))
        if (DoMethodA(child, (Msg) msg))
            return TRUE;

    return FALSE;
}

/****** Family.mui/MUIM_Family_GetChild ************
*
*   NAME
*       MUIM_Family_GetChild
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Family_GetChild, nr, (IPTR)ref);
*
*   FUNCTION
*       Return a borrowed immediate-child pointer selected by position or by a
*       reference child.
*
*   INPUTS
*       nr - Nonnegative zero-based index, MUIV_Family_GetChild_First (0), Last
*       (-1), Next (-2), or Previous (-3).
*       ref - Existing immediate child for Next or Previous. Ignored for an
*       index, First, or Last; pass NULL in those cases.
*
*   RESULT
*       The selected object pointer, or NULL for an empty list, out-of-range
*       index, absent reference, missing neighbor, or unsupported negative
*       selector.
*
*   NOTES
*       No ownership is transferred. Keep the family and child alive and prevent
*       mutation while using the result. Descendants below immediate children
*       are not searched.
*
*/
IPTR Family__MUIM_GetChild(struct IClass *cl, Object *obj,
    struct MUIP_Family_GetChild *msg)
{
    struct MUI_FamilyData *data = INST_DATA(cl, obj);
    Object *cstate = (Object *) data->children.lh_Head;
    Object *child, *prev = NULL;
    LONG counter = 0;

    while ((child = NextObject(&cstate)))
    {
        if ((msg->nr >= 0) && (msg->nr == counter))
            return (IPTR) child;

        if ((msg->ref != NULL) && (msg->ref == child))
        {
            if (msg->nr == MUIV_Family_GetChild_Next)
                return (IPTR) NextObject(&cstate);
            if (msg->nr == MUIV_Family_GetChild_Previous)
                return (IPTR) prev;
        }

        if (msg->nr == MUIV_Family_GetChild_First)
            return (IPTR) child;

        prev = child;
        counter++;
    }

    if (msg->nr == MUIV_Family_GetChild_Last)
        return (IPTR) prev;

    return (IPTR) NULL;
}

/* Dispatch construction, disposal, getters, child management, and recursive
 * user-data operations. Map OM_ADDMEMBER to AddTail and OM_REMMEMBER to
 * Remove. Return handler results and forward unknown methods to Notify.
 * Messages must contain all fields required by their method.
 */
BOOPSI_DISPATCHER(IPTR, Family_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Family__OM_NEW(cl, obj, (struct opSet *)msg);

    case OM_DISPOSE:
        return Family__OM_DISPOSE(cl, obj, msg);

    case OM_GET:
        return Family__OM_GET(cl, obj, (struct opGet *)msg);

    case MUIM_Family_AddHead:
        return Family__MUIM_AddHead(cl, obj, (APTR) msg);

    case OM_ADDMEMBER:
    case MUIM_Family_AddTail:
        return Family__MUIM_AddTail(cl, obj, (APTR) msg);

    case MUIM_Family_Insert:
        return Family__MUIM_Insert(cl, obj, (APTR) msg);

    case OM_REMMEMBER:
    case MUIM_Family_Remove:
        return Family__MUIM_Remove(cl, obj, (APTR) msg);

    case MUIM_Family_Sort:
        return Family__MUIM_Sort(cl, obj, (APTR) msg);

    case MUIM_Family_Transfer:
        return Family__MUIM_Transfer(cl, obj, (APTR) msg);

    case MUIM_FindUData:
        return Family__MUIM_FindUData(cl, obj, (APTR) msg);

    case MUIM_GetUData:
        return Family__MUIM_GetUData(cl, obj, (APTR) msg);

    case MUIM_SetUData:
        return Family__MUIM_SetUData(cl, obj, (APTR) msg);

    case MUIM_SetUDataOnce:
        return Family__MUIM_SetUDataOnce(cl, obj, (APTR) msg);

    case MUIM_Family_GetChild:
        return Family__MUIM_GetChild(cl, obj, (APTR) msg);
    }

    return (DoSuperMethodA(cl, obj, msg));
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Family_desc =
{
    MUIC_Family,
    MUIC_Notify,
    sizeof(struct MUI_FamilyData),
    (void *) Family_Dispatcher
};
