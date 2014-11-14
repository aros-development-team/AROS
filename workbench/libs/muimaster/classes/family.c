/* 
    Copyright � 1999, David Le Corfec.
    Copyright � 2002-2013, The AROS Development Team.
    All rights reserved.

    $Id$
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

/*
 * Family class is the base class for objects that are able
 * to handle a list of children. This is e.g. the case for
 * MUIs Menustrip, Menu and Menuitem objects.
 * Group class and application class should also be a
 * subclass of Family class, but due to BOOPSI system
 * limitations, this is currently impossible.
 */

/*
Family.mui/MUIA_Family_Child [I..]        done (note : == MUIA_Group_Child)
Family.mui/MUIA_Family_List [..G]         done

Family.mui/MUIM_Family_AddHead            done
Family.mui/MUIM_Family_AddTail            done
Family.mui/MUIM_Family_Insert             done
Family.mui/MUIM_Family_Remove             done
Family.mui/MUIM_Family_Sort               done
Family.mui/MUIM_Family_Transfer           done
Notify.mui/MUIM_FindUData                 done
Notify.mui/MUIM_GetUData                  done
Notify.mui/MUIM_SetUData                  done
Notify.mui/MUIM_SetUDataOnce              done
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


/*
 * MUIM_Family_AddHead : Add an object as first object to the family.
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

        /* Some apps (Odyssey) expect _parent() will work before group tree is added to application tree */
        muiNotifyData(msg->obj)->mnd_ParentObject = obj;

        return TRUE;
    }
    else
        return FALSE;
}


/*
 * MUIM_Family_AddTail : Add an object as last object to the family.
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

        /* Some apps (Odyssey) expect _parent() will work before group tree is added to application tree */
        muiNotifyData(msg->obj)->mnd_ParentObject = obj;

        return TRUE;
    }
    else
        return FALSE;
}


/*
 * MUIM_Family_Insert : Add an object after another object to the family.
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

        /* Some apps (Odyssey) expect _parent() will work before group tree is added to application tree */
        muiNotifyData(msg->obj)->mnd_ParentObject = obj;

        return TRUE;
    }
    else
        return FALSE;
}


/*
 * MUIM_Family_Remove : Remove an object from a family.
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


/*
 * MUIM_Family_Sort : Sort the children of a family.
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


/*
 * MUIM_Family_Transfer : All the children of the family are removed and
 * added to another family in the same order.
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


/**************************************************************************
 MUIM_FindUData : tests if the MUIA_UserData of the object
 contains the given <udata> and returns the object pointer in this case.
**************************************************************************/
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


/*
 * MUIM_GetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and gets <attr> to <storage> for itself
 * in this case.
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


/*
 * MUIM_SetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and sets <attr> to <val> for itself in this case.
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


/*
 * MUIM_SetUDataOnce : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and sets <attr> to <val> for itself in this case.
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
