/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <clib/alib_protos.h>
#include <libraries/commodities.h>
#include <proto/commodities.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"

#include "notify.h"

#define MYDEBUG
#include "debug.h"

extern struct Library *MUIMasterBase;

/* RawDoFmt output callback. Write one byte through strPtrPtr and advance
 * it. The caller supplies sufficient writable output space; no bounds check
 * is made. The callback has no return value or retained state.
 */
AROS_UFH2S(void, cpy_func,
    AROS_UFHA(UBYTE, chr, D0),
    AROS_UFHA(STRPTR *, strPtrPtr, A3))
{
    AROS_USERFUNC_INIT

    *(*strPtrPtr)++ = chr;

    AROS_USERFUNC_EXIT
}

/* RawDoFmt counting callback. Increment the caller-owned LONG counter for
 * each emitted byte, including the terminator. The caller must keep the
 * count within LONG range; chr is unused.
 */
AROS_UFH2S(void, len_func,
    AROS_UFHA(UBYTE, chr, D0),
    AROS_UFHA(LONG *, lenPtr, A3))
{
    AROS_USERFUNC_INIT

    (*lenPtr)++;

    AROS_USERFUNC_EXIT
}


/****** Notify.mui/MUIC_Notify ************
*
*   NAME
*       MUIC_Notify
*
*   SUPERCLASS
*       rootclass
*
*   DESCRIPTION
*       Provide attributes, synchronous notifications, user-data matching, and
*       utility methods for Zune objects. This class has no child traversal;
*       container subclasses may extend the user-data methods.
*       Construction allocates private attribute storage and returns NULL if
*       allocation or superclass construction fails. Omitted HelpLine, ObjectID,
*       and UserData values start at zero; HelpNode, AppMessage, parent, and
*       global-info pointers start at NULL. Disposal frees local notification
*       records and attribute storage. It does not dispose of notification
*       destinations or free borrowed strings and messages.
*       Serialize access to the object and its notification list. Notifications
*       execute in the calling task during attribute setting; this class
*       provides no locking, queue, or cancellation timeout.
*
*   NOTES
*       All method entries describe the Notify implementation. A subclass may
*       override its behavior or return values. Version and Revision both return
*       1 here; they are class values, not muimaster.library version numbers.
*       An OM_SET first stores recognized attributes, then examines the supplied
*       tags for notifications in tag order and registration order. Unchanged
*       values can still trigger a notification. Attributes not stored by Notify
*       can still be trigger attributes when their tags reach this method.
*       Notify returns zero from OM_SET and does not forward it to rootclass.
*
*   BUGS
*       Notification dispatch does not protect its current list node against
*       deletion by a callback. Do not dispose of the source object or remove
*       active notification records during notification dispatch. Suppression of
*       recursive delivery for one active record is not a lifetime guard.
*
*   EXAMPLE
*       Copy each UserData tag value to ObjectID on the same object:
*       DoMethod(obj, MUIM_Notify, MUIA_UserData, MUIV_EveryTime,
*                MUIV_Notify_Self, 3, MUIM_Set, MUIA_ObjectID,
*                MUIV_TriggerValue);
*       Register outside notification dispatch; check the returned BOOL.
*
*/

/****** Notify.mui/MUIA_ApplicationObject ************
*
*   NAME
*       MUIA_ApplicationObject
*
*   FUNCTION
*       Return the application pointer from the current global-info structure,
*       or NULL if that structure is absent. The returned object is borrowed.
*
*   INPUTS
*       Object pointer; get only.
*
*   NOTES
*       DisconnectParent does not clear global info in this implementation. A
*       non-NULL result is not proof that the application is still alive.
*
*/

/****** Notify.mui/MUIA_AppMessage ************
*
*   NAME
*       MUIA_AppMessage
*
*   FUNCTION
*       Return the last message pointer stored by OM_SET. The constructor
*       ignores this tag. Initial value is NULL.
*
*   INPUTS
*       Pointer to struct AppMessage; declared get only.
*
*   NOTES
*       AROS also accepts this tag in OM_SET despite the header access flags.
*       The pointer is borrowed: Notify neither copies, replies to, nor frees
*       the message. Keep it valid while accessing it; storage here does not
*       extend its lifetime.
*
*/

/****** Notify.mui/MUIA_HelpLine ************
*
*   NAME
*       MUIA_HelpLine
*
*   FUNCTION
*       Store a help-line value; the initial value is zero. Notify stores and
*       returns it without validation.
*
*   INPUTS
*       LONG; initialize, set, get.
*
*   NOTES
*       Notify does not display help or interpret this value.
*
*/

/****** Notify.mui/MUIA_HelpNode ************
*
*   NAME
*       MUIA_HelpNode
*
*   FUNCTION
*       Store a borrowed help-node string pointer; the initial value is NULL.
*
*   INPUTS
*       STRPTR; initialize, set, get.
*
*   NOTES
*       The string is not copied or freed. Keep its storage valid while the
*       attribute refers to it. Notify does not open or interpret the help node.
*
*/

/****** Notify.mui/MUIA_NoNotify ************
*
*   NAME
*       MUIA_NoNotify
*
*   FUNCTION
*       Suppress this OM_SET call's entire notification pass if any supplied
*       MUIA_NoNotify tag has the value TRUE. Attribute storage still takes
*       place.
*
*   INPUTS
*       BOOL; set only.
*
*   NOTES
*       The flag is local to one call and is not stored. A FALSE tag later in
*       the same list does not cancel suppression. The comparison is exactly
*       against TRUE, not against any nonzero value. Notify has no getter for
*       this attribute.
*
*/

/****** Notify.mui/MUIA_ObjectID ************
*
*   NAME
*       MUIA_ObjectID
*
*   FUNCTION
*       Store an object identifier; the initial value is zero. Values are
*       converted to ULONG.
*
*   INPUTS
*       ULONG; initialize, set, get.
*
*   NOTES
*       Notify does not enforce uniqueness or persist data under this
*       identifier. Its Import and Export methods are stubs; persistence depends
*       on subclasses.
*
*/

/****** Notify.mui/MUIA_Parent ************
*
*   NAME
*       MUIA_Parent
*
*   FUNCTION
*       Return the stored parent pointer, initially NULL. The pointer is
*       borrowed.
*
*   INPUTS
*       Object pointer; get only.
*
*   NOTES
*       Notify's ConnectParent and DisconnectParent do not update this field.
*       Containers maintain it separately. Do not infer ownership or continued
*       parent lifetime solely from a non-NULL value.
*
*/

/****** Notify.mui/MUIA_UserData ************
*
*   NAME
*       MUIA_UserData
*
*   FUNCTION
*       Store an uninterpreted application value, initially zero. Pointer-sized
*       values are preserved by Notify.
*
*   INPUTS
*       IPTR storage; initialize, set, get.
*
*   NOTES
*       The header annotation says ULONG, but mnd_UserData and the setter/getter
*       use IPTR. No referenced memory is owned or freed. MUIM_GetUData is an
*       exception: its declared udata input is ULONG; other Notify user-data
*       search/set messages use IPTR.
*
*/

/****** Notify.mui/MUIA_Version ************
*
*   NAME
*       MUIA_Version
*
*   FUNCTION
*       Return 1 for Notify itself.
*
*   INPUTS
*       LONG; get only.
*
*   NOTES
*       Subclasses can override this attribute. Do not use it to infer the
*       muimaster.library version or availability of methods on another class.
*
*/

/****** Notify.mui/MUIA_Revision ************
*
*   NAME
*       MUIA_Revision
*
*   FUNCTION
*       Return 1 for Notify itself.
*
*   INPUTS
*       LONG; get only.
*
*   NOTES
*       Subclasses can override this attribute. This value describes the class,
*       not the library.
*
*/

static const int __version = 1;
static const int __revision = 1;

struct MUI_NotifyAttributes
{
    STRPTR                 na_HelpNode;
    LONG                   na_HelpLine;
    struct AppMessage     *na_AppMessage;
};

/*
 * Notification handler
 */
typedef struct NotifyNode
{
    struct MinNode nn_Node;
    BOOL nn_Active;     /* TRUE if notification is currently being handled */
                        /* It's used to prevent loops */
    ULONG nn_TrigAttr;
    IPTR nn_TrigVal;
    APTR nn_DestObj;
    ULONG nn_NumParams;
    IPTR *nn_Params;    /* FIXME: use nn_Params[1] and tweak stuff below */
    IPTR *nn_NewParams; /* For MUIV_EveryTime */
} *NNode;

typedef struct NotifyNodeIX
{
    struct NotifyNode nn;
    IX ix;
} *NNodeIX;

/* Allocate an owned notification node and copies of FollowParams message
 * words. Parse an input-event expression when required. Return NULL on
 * parse or allocation failure, releasing partial node storage. data is
 * unused. The caller validates the count and must prevent size overflow;
 * pointed-to argument data remains borrowed.
 */
static struct NotifyNode *CreateNNode(struct MUI_NotifyData *data,
    struct MUIP_Notify *msg)
{
    ULONG i, paramsize;

    struct NotifyNode *nnode;

    if ((msg->TrigAttr == MUIA_Window_InputEvent)
        && (msg->TrigVal != MUIV_EveryTime))
    {
        IX ix = { IX_VERSION };

        if (ParseIX((CONST_STRPTR) msg->TrigVal, &ix) != 0)
            return NULL;

        if ((nnode =
                (struct NotifyNode *)mui_alloc_struct(struct NotifyNodeIX)))
        {
            ((struct NotifyNodeIX *)nnode)->ix = ix;
        }
    }
    else
    {
        nnode = mui_alloc_struct(struct NotifyNode);
    }

    if (!nnode)
        return NULL;

    nnode->nn_Active = FALSE;
    nnode->nn_TrigAttr = msg->TrigAttr;
    nnode->nn_TrigVal = msg->TrigVal;
    nnode->nn_DestObj = msg->DestObj;
    nnode->nn_NumParams = msg->FollowParams;

    /* Allocate one more IPTR (FollowParams + 1) as some ext apps/classes
       forget trailing NULLs in methods like MUIM_MultiSet and MUI seems
       like it can live with that (without crashing) */

    paramsize = (msg->FollowParams + 1);
    if (msg->TrigVal == MUIV_EveryTime)
    {
        paramsize *= 2;
    }

    if ((nnode->nn_Params = (IPTR *) mui_alloc(paramsize * sizeof(IPTR))))
    {
        IPTR *par = (IPTR *) & msg->FollowParams;

        for (i = 0; i < msg->FollowParams; i++)
        {
            nnode->nn_Params[i] = *(par + i + 1);
        }

        if (msg->TrigVal == MUIV_EveryTime)
        {
            nnode->nn_NewParams = nnode->nn_Params + msg->FollowParams + 1;
            for (i = 0; i < msg->FollowParams; i++)
            {
                nnode->nn_NewParams[i] = *(par + i + 1);
            }
        }

        return nnode;
    }

    mui_free(nnode);

    return NULL;
}

/* Free a valid owned notification node and its parameter allocation. The
 * caller must unlink it first if registered and ensure that no dispatch
 * still uses it. data is unused; destinations and pointed-to arguments are
 * not freed.
 */
static void DeleteNNode(struct MUI_NotifyData *data,
    struct NotifyNode *nnode)
{
    mui_free(nnode->nn_Params);
    mui_free(nnode);
}


/* Forward declaration of the attribute-store and synchronous-notification
 * handler defined below. It requires a valid tag list and returns zero.
 */
IPTR Notify__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg);

/*
 * OM_NEW
 */
/* Construct the rootclass object and private attributes; initialize
 * recognized tags without sending notifications. Return the object or zero
 * on failure, disposing the partially constructed object if attribute
 * allocation fails. String pointers are borrowed.
 */
IPTR Notify__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_NotifyData *data;
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    data->mnd_Attributes = mui_alloc_struct(struct MUI_NotifyAttributes);
    if (data->mnd_Attributes == NULL)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return FALSE;
    }

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_HelpLine:
            data->mnd_Attributes->na_HelpLine = (LONG) tag->ti_Data;
            break;

        case MUIA_HelpNode:
            data->mnd_Attributes->na_HelpNode = (STRPTR) tag->ti_Data;
            break;

        case MUIA_ObjectID:
            data->mnd_ObjectID = (ULONG) tag->ti_Data;
            break;

        case MUIA_UserData:
            data->mnd_UserData = (IPTR) tag->ti_Data;
            break;
        }
    }

    return (IPTR) obj;
}


/*
 * OM_DISPOSE
 */
/* After all use and notification dispatch have ended, free private
 * attributes and registered nodes, then dispose through rootclass and
 * return its result. Borrowed data and destination objects are not freed.
 */
IPTR Notify__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MinNode *node, *tmp;
    struct MUI_NotifyData *data = INST_DATA(cl, obj);

    mui_free(data->mnd_Attributes);

    if (data->mnd_NotifyList)
    {
        for (node = data->mnd_NotifyList->mlh_Head; node->mln_Succ;
            node = tmp)
        {
            tmp = node->mln_Succ;
            DeleteNNode(data, (struct NotifyNode *)node);
        }
        mui_free(data->mnd_NotifyList);
    }

    return DoSuperMethodA(cl, obj, msg);
}

/* Match one tag against a live registered node and synchronously deliver
 * its copied message. Resolve supported destination selectors and
 * substitute EveryTime arguments. Skip an already active node. The caller
 * must keep the node, source and destination alive through return;
 * callbacks must not delete them. No result is returned.
 */
static void check_notify(NNode nnode, Object *obj, struct TagItem *tag)
{
    IPTR *params;
    APTR destobj;
    int i;
    BOOL donotify = FALSE;

    /* is it the good attribute? */
    if (tag->ti_Tag != nnode->nn_TrigAttr)
        return;

    if (nnode->nn_TrigVal == MUIV_EveryTime)
    {
        donotify = TRUE;
    }
    else if (nnode->nn_TrigAttr == MUIA_Window_InputEvent)
    {
        if (MatchIX((struct InputEvent *)tag->ti_Data,
                &((struct NotifyNodeIX *)nnode)->ix))
        {
            donotify = TRUE;
        }
    }
    else if (nnode->nn_TrigVal == tag->ti_Data)
    {
        donotify = TRUE;
    }

    /* Is the notification already being performed? */
    if (donotify && nnode->nn_Active)
    {
        static int counter;

        bug("%s: MUI Notification loop detected! (#%d)\n", FindTask(NULL)->tc_Node.ln_Name, counter++);
#if DEBUG
        D(bug("  Source object: 0x%p", obj));

        switch ((IPTR) nnode->nn_DestObj)
        {
        case MUIV_Notify_Application:
            D(bug("  Dest object: 0x%p (MUIV_Notify_Application)\n",
                    _app(obj)));
            break;
        case MUIV_Notify_Self:
            D(bug("  Dest object: 0x%p (MUIV_Notify_Self)\n", obj));
            destobj = obj;
            break;
        case MUIV_Notify_Window:
            if (muiRenderInfo(obj)) /* otherwise _win(obj) does NULL access! */
            {
                D(bug("  Dest object: 0x%p (MUIV_Notify_Window)\n",
                        _win(obj)));
            }
            else
            {
                D(bug("  Dest object: INVALID "
                    "(MUIV_Notify_Window, but no muiRenderInfo)\n"));
            }
            break;
        default:
            D(bug("  Dest object: 0x%p\n", nnode->nn_DestObj));
            break;
        }
        D(bug("  Attribute: 0x%08x   Value: %p\n", tag->ti_Tag,
                tag->ti_Data));

#endif
        return;
    }

    if (donotify)
    {
        switch ((IPTR) nnode->nn_DestObj)
        {
        case MUIV_Notify_Application:
            destobj = _app(obj);
            break;
        case MUIV_Notify_Self:
            destobj = obj;
            break;
        case MUIV_Notify_Window:
            if (muiRenderInfo(obj)) /* otherwise _win(obj) does NULL access! */
            {
                destobj = _win(obj);
            }
            else
            {
                return;
            }
            break;
        default:
            destobj = nnode->nn_DestObj;
        }

        params = nnode->nn_Params;
        if (nnode->nn_TrigVal == MUIV_EveryTime)
        {
            params = nnode->nn_NewParams;

            for (i = 1; i < nnode->nn_NumParams; i++)
            {
                switch (nnode->nn_Params[i])
                {
                case MUIV_TriggerValue:
                    params[i] = tag->ti_Data;
                    break;

                case MUIV_NotTriggerValue:
                    params[i] = !tag->ti_Data;
                    break;
                }
            }
        }

        nnode->nn_Active = TRUE;

        /* call method */
        DoMethodA(destobj, (Msg) params);

        nnode->nn_Active = FALSE;
    }
}

/*
 * OM_SET
 */
/* Store recognized tags, then notify in tag and registration order unless
 * any NoNotify tag equals TRUE. NoNotify is per call. Return zero even
 * after successful storage; do not forward OM_SET to rootclass.
 */
IPTR Notify__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    BOOL no_notify = FALSE;
    struct TagItem *tag;
    struct MinNode *node;

    /* There are many ways to find out what tag items provided by set()
     ** we do know. The best way should be using NextTagItem() and simply
     ** browsing through the list.
     */
    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
      case MUIA_AppMessage:
          data->mnd_Attributes->na_AppMessage = (struct AppMessage *)tag->ti_Data;
          break;

        case MUIA_HelpLine:
            data->mnd_Attributes->na_HelpLine = (LONG) tag->ti_Data;
            break;

        case MUIA_HelpNode:
            data->mnd_Attributes->na_HelpNode = (STRPTR) tag->ti_Data;
            break;

        case MUIA_NoNotify:
            if (tag->ti_Data == TRUE)
                no_notify = TRUE;
            break;

        case MUIA_ObjectID:
            data->mnd_ObjectID = (ULONG) tag->ti_Data;
            break;

        case MUIA_UserData:
            data->mnd_UserData = tag->ti_Data;
            break;
        }
    }

    /*
     * check for notifications
     */
    if (!data->mnd_NotifyList || no_notify)
        return 0;

    tags = msg->ops_AttrList;
    while ((tag = NextTagItem(&tags)))
    {
        ForeachNode (data->mnd_NotifyList, node)
        {
            check_notify((NNode) node, obj, tag);
        }
    }

    return 0;
}


/*
 * OM_GET
 */
/* Store recognized attribute values through a writable opg_Storage and
 * return TRUE; forward unrecognized attributes to rootclass. Returned
 * pointers are borrowed. A NULL global-info pointer yields a NULL
 * ApplicationObject.
 */
IPTR Notify__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
/* small macro to simplify return value storage */
#define STORE *(msg->opg_Storage)

    struct MUI_NotifyData *data = INST_DATA(cl, obj);

//kprintf("*** Notify->GET\n");
    switch (msg->opg_AttrID)
    {
    case MUIA_ApplicationObject:
        if (data->mnd_GlobalInfo)
            STORE = (IPTR) data->mnd_GlobalInfo->mgi_ApplicationObject;
        else
            STORE = 0;
        return TRUE;

    case MUIA_AppMessage:
        STORE = (IPTR) data->mnd_Attributes->na_AppMessage;
        return TRUE;

    case MUIA_HelpLine:
        STORE = (IPTR) data->mnd_Attributes->na_HelpLine;
        return TRUE;

    case MUIA_HelpNode:
        STORE = (IPTR) data->mnd_Attributes->na_HelpNode;
        return TRUE;

    case MUIA_ObjectID:
        STORE = (IPTR) data->mnd_ObjectID;
        return TRUE;

    case MUIA_Parent:
        STORE = (IPTR) data->mnd_ParentObject;
        return TRUE;

    case MUIA_Revision:
        STORE = __revision;
        return TRUE;

    case MUIA_UserData:
        STORE = data->mnd_UserData;
        return TRUE;

    case MUIA_Version:
        STORE = __version;
        return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


/****** Notify.mui/MUIM_CallHook ************
*
*   NAME
*       MUIM_CallHook
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_CallHook, (IPTR)hook, param1);
*
*   FUNCTION
*       Invoke the hook synchronously with obj as the hook object and the
*       address of param1 as its message.
*
*   INPUTS
*       hook - Non-NULL readable Hook structure.
*       param1 - First IPTR of the hook-specific message; append further
*       arguments as required by the hook.
*
*   RESULT
*       The hook result, or FALSE if h_Entry is NULL.
*
*   NOTES
*       No argument storage is copied or retained. The hook must not retain
*       pointers into the transient method message. A NULL hook pointer is not
*       checked.
*
*/
IPTR Notify__MUIM_CallHook(struct IClass *cl, Object *obj,
    struct MUIP_CallHook *msg)
{
    if (msg->Hook->h_Entry)
        return CallHookPkt(msg->Hook, obj, &msg->param1);
    else
        return FALSE;
}


/* nothing to export */

/****** Notify.mui/MUIM_FindUData ************
*
*   NAME
*       MUIM_FindUData
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_FindUData, udata);
*
*   FUNCTION
*       Compare this object's MUIA_UserData with udata.
*
*   INPUTS
*       udata - IPTR value to match.
*
*   RESULT
*       The borrowed object pointer on equality; zero otherwise.
*
*   NOTES
*       Only this object is examined. A container subclass may also search
*       children. The result does not transfer ownership or retain the object.
*
*/
IPTR Notify__MUIM_FindUData(struct IClass *cl, Object *obj,
    struct MUIP_FindUData *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);

    if (data->mnd_UserData == msg->udata)
    {
        return (IPTR) obj;
    }
    return 0L;
}


/****** Notify.mui/MUIM_GetUData ************
*
*   NAME
*       MUIM_GetUData
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_GetUData, udata, attr, (IPTR)storage);
*
*   FUNCTION
*       If UserData matches, request attr from this object into storage.
*
*   INPUTS
*       udata - ULONG comparison value in struct MUIP_GetUData.
*       attr - Attribute identifier.
*       storage - Writable IPTR result location, required on a match.
*
*   RESULT
*       TRUE for a UserData match, FALSE otherwise. The attribute getter's
*       success result is ignored.
*
*   NOTES
*       No match leaves storage untouched. On a match, an unsupported getter may
*       also leave it unchanged; TRUE is not proof of a valid attribute result.
*       Only this object is examined.
*
*/
IPTR Notify__MUIM_GetUData(struct IClass *cl, Object *obj,
    struct MUIP_GetUData *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);

    if (data->mnd_UserData == msg->udata)
    {
        get(obj, msg->attr, msg->storage);
        return TRUE;
    }
    return FALSE;
}


/* nothing to import */

/****** Notify.mui/MUIM_KillNotify ************
*
*   NAME
*       MUIM_KillNotify
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_KillNotify, TrigAttr);
*
*   FUNCTION
*       Remove and free the first registered notification whose trigger
*       attribute matches TrigAttr.
*
*   INPUTS
*       TrigAttr - ULONG trigger-attribute identifier.
*
*   RESULT
*       1 if a record was removed; 0 if none matched or no notification list
*       exists.
*
*   NOTES
*       It does not remove all matching records. Repeat outside notification
*       dispatch if all matches must be removed. The destination object remains
*       caller-owned.
*
*   BUGS
*       Removing an active record, or disposing of its source during a callback,
*       can invalidate notification dispatch's pointers.
*
*/
IPTR Notify__MUIM_KillNotify(struct IClass *cl, Object *obj,
    struct MUIP_KillNotify *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);
    struct MinNode *node;
    struct NotifyNode *nnode;

    if (!data->mnd_NotifyList)
        return 0;

    for (node = data->mnd_NotifyList->mlh_Head; node->mln_Succ;
        node = node->mln_Succ)
    {
        nnode = (NNode) node;
        if (msg->TrigAttr == nnode->nn_TrigAttr)
        {
            Remove((struct Node *)node);
            DeleteNNode(data, nnode);
            return 1;
        }
    }
    return 0;
}


/****** Notify.mui/MUIM_KillNotifyObj ************
*
*   NAME
*       MUIM_KillNotifyObj
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_KillNotifyObj, TrigAttr, (IPTR)dest);
*
*   FUNCTION
*       Remove the first notification with both the specified trigger attribute
*       and stored destination.
*
*   INPUTS
*       TrigAttr - ULONG trigger-attribute identifier.
*       dest - The exact destination value supplied at registration.
*
*   RESULT
*       1 if a record was removed; 0 if no record matched.
*
*   NOTES
*       Destination comparison uses the stored value, not its resolved target.
*       For a Self registration, use MUIV_Notify_Self, not the object pointer.
*       Only one record is removed; no destination object is disposed of.
*
*   BUGS
*       Do not remove notification records during notification dispatch; its
*       list traversal is not protected against deletion.
*
*/
IPTR Notify__MUIM_KillNotifyObj(struct IClass *cl, Object *obj,
    struct MUIP_KillNotifyObj *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);
    struct MinNode *node;
    struct NotifyNode *nnode;

    if (!data->mnd_NotifyList)
        return 0;

    for (node = data->mnd_NotifyList->mlh_Head; node->mln_Succ;
        node = node->mln_Succ)
    {
        nnode = (NNode) node;
        if ((msg->TrigAttr == nnode->nn_TrigAttr)
            && (msg->dest == nnode->nn_DestObj))
        {
            Remove((struct Node *)node);
            DeleteNNode(data, nnode);
            return 1;
        }
    }
    return 0;
}


/****** Notify.mui/MUIM_MultiSet ************
*
*   NAME
*       MUIM_MultiSet
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_MultiSet, attr, val, (IPTR)first, ...,
*       (IPTR)NULL);
*
*   FUNCTION
*       Set one attribute on each object in a NULL-terminated list, in list
*       order.
*
*   INPUTS
*       attr - Attribute identifier.
*       val - IPTR value.
*       first, ... - Live destination object pointers followed by NULL. A NULL
*       first entry gives an empty list.
*
*   RESULT
*       Always TRUE after traversing the list, including an empty list.
*       Individual setter return values are ignored.
*
*   NOTES
*       The call can synchronously trigger notifications on every destination.
*       It does not retain the objects or copy data referenced by val. A missing
*       terminator causes reads beyond the method message.
*
*/
IPTR Notify__MUIM_MultiSet(struct IClass *cl, Object *obj,
    struct MUIP_MultiSet *msg)
{
    IPTR *destobj_p;
    for (destobj_p = (IPTR *) & msg->obj; (*destobj_p) != 0; destobj_p++)
    {
        set((APTR) *destobj_p, msg->attr, msg->val);
    }
    return TRUE;
}


/****** Notify.mui/MUIM_NoNotifySet ************
*
*   NAME
*       MUIM_NoNotifySet
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_NoNotifySet, attr, val);
*
*   FUNCTION
*       Call SetAttrs with MUIA_NoNotify set to TRUE and the requested
*       attribute/value pair.
*
*   INPUTS
*       attr - Attribute identifier.
*       val - IPTR attribute value.
*
*   RESULT
*       The SetAttrs result. Notify's own OM_SET returns zero even when it
*       stores the value.
*
*   NOTES
*       Suppression applies to Notify's notification pass for this set call.
*       Subclass behavior and setter side effects still apply; this is not a
*       persistent notification-disable switch.
*
*/
IPTR Notify__MUIM_NoNotifySet(struct IClass *cl, Object *obj,
    struct MUIP_NoNotifySet *msg)
{
    return SetAttrs(obj, MUIA_NoNotify, TRUE, msg->attr, msg->val,
        TAG_DONE);
}


/****** Notify.mui/MUIM_Notify ************
*
*   NAME
*       MUIM_Notify
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Notify, TrigAttr, TrigVal,
*       (IPTR)DestObj, FollowParams, ...);
*
*   FUNCTION
*       Append a notification that sends a copied method message when a matching
*       tag reaches Notify's OM_SET.
*
*   INPUTS
*       TrigAttr - ULONG attribute identifier.
*       TrigVal - IPTR value to match, or MUIV_EveryTime. For
*       MUIA_Window_InputEvent with a non-EveryTime value, supply a readable
*       ParseIX description string instead.
*       DestObj - Live object pointer, MUIV_Notify_Self,
*       MUIV_Notify_Application, or MUIV_Notify_Window.
*       FollowParams - Number of following IPTR words, including the destination
*       MethodID; must be at least 1. Supply exactly that many words.
*       ... - Destination MethodID followed by its arguments.
*
*   RESULT
*       TRUE if the notification was registered; FALSE for fewer than one follow
*       word, allocation failure, or an invalid input-event description. No
*       destination call is made during registration.
*
*   NOTES
*       The method words are copied; pointed-to objects and data are not. Keep
*       those live until no registered notification can use them. Duplicate
*       registrations are accepted and run in registration order. Sizes must fit
*       the ULONG allocation calculation; there is no overflow check.
*       For MUIV_EveryTime only, argument words after MethodID equal to
*       MUIV_TriggerValue or MUIV_NotTriggerValue are replaced with the current
*       tag value or its logical negation. An ordinary fixed-value notification
*       does not substitute these markers.
*       Self resolves to the source object. Application requires live global
*       info. Window requires an Area-compatible object with valid render-info
*       storage; delivery is skipped when that pointer is NULL. A record already
*       executing is skipped on recursive delivery; other records may still run.
*       Destination method results are ignored.
*
*   BUGS
*       MUIV_Notify_Parent is declared in the header but has no special handling
*       in this implementation. It falls through as a literal destination
*       pointer and must not be used as a selector. Application resolution does
*       not check global info for NULL. Do not remove active records or dispose
*       of the source from a notification callback.
*
*/
IPTR Notify__MUIM_Notify(struct IClass *cl, Object *obj,
    struct MUIP_Notify *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);
    struct NotifyNode *nnode;

    if (msg->FollowParams < 1)
        return FALSE;

    if (data->mnd_NotifyList == NULL)
    {
        if (!(data->mnd_NotifyList = mui_alloc_struct(struct MinList)))
              return FALSE;
        NewList((struct List *)data->mnd_NotifyList);
    }

    nnode = CreateNNode(data, msg);
    if (NULL == nnode)
        return FALSE;

    AddTail((struct List *)data->mnd_NotifyList, (struct Node *)nnode);
    return TRUE;
}


/****** Notify.mui/MUIM_Set ************
*
*   NAME
*       MUIM_Set
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Set, attr, val);
*
*   FUNCTION
*       Set one attribute on this object through the normal setter.
*
*   INPUTS
*       attr - Attribute identifier.
*       val - IPTR attribute value.
*
*   RESULT
*       The setter result. Notify's OM_SET returns zero even after storing
*       recognized attributes.
*
*   NOTES
*       This can deliver notifications synchronously. Ownership and copying of
*       data referenced by val depend on the target attribute.
*
*/
IPTR Notify__MUIM_Set(struct IClass *cl, Object *obj,
    struct MUIP_Set *msg)
{
    return set(obj, msg->attr, msg->val);
}

/****** Notify.mui/MUIM_SetAsString ************
*
*   NAME
*       MUIM_SetAsString
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_SetAsString, attr, (IPTR)format, val,
*       ...);
*
*   FUNCTION
*       Format a temporary string with RawDoFmt, set attr to its pointer, then
*       immediately free the string.
*
*   INPUTS
*       attr - String attribute whose setter consumes or copies the string
*       during the call.
*       format - Readable RawDoFmt format string.
*       val, ... - Arguments in the RawDoFmt data-stream layout required by the
*       format and target ABI; not a printf va_list.
*
*   RESULT
*       FALSE if temporary-string allocation fails; otherwise TRUE. The
*       attribute setter's result is ignored.
*
*   NOTES
*       The format is processed twice, first to count output bytes and then to
*       write them. Keep the format and arguments valid and consistent across
*       both passes. The final length must fit LONG and the allocation size.
*
*   BUGS
*       Do not use this method with an attribute that merely borrows the
*       supplied string, including Notify's MUIA_HelpNode: the stored pointer
*       would refer to freed memory after return.
*
*/
IPTR Notify__MUIM_SetAsString(struct IClass *cl, Object *obj,
    struct MUIP_SetAsString *msg)
{
    STRPTR txt;
    LONG txt_len;

    txt_len = 0;
    RawDoFmt(msg->format, (RAWARG)&msg->val,
        (VOID_FUNC) AROS_ASMSYMNAME(len_func), &txt_len);

/*  D(bug("Notify_SetAsString: fmt=%s, txtlen=%d\n", msg->format, txt_len)); */
    txt = AllocVec(txt_len + 1, 0);
    if (NULL == txt)
        return FALSE;

    {
        STRPTR txtptr = txt;
        RawDoFmt(msg->format, (RAWARG)&msg->val,
            (VOID_FUNC) AROS_ASMSYMNAME(cpy_func), &txtptr);
    }

    set(obj, msg->attr, (IPTR) txt);
    FreeVec(txt);

    return TRUE;
}


/****** Notify.mui/MUIM_SetUData ************
*
*   NAME
*       MUIM_SetUData
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_SetUData, udata, attr, val);
*
*   FUNCTION
*       Set attr to val if this object's MUIA_UserData matches udata.
*
*   INPUTS
*       udata - IPTR comparison value.
*       attr - Attribute identifier.
*       val - IPTR attribute value.
*
*   RESULT
*       TRUE on a UserData match, FALSE otherwise. The setter result is ignored.
*
*   NOTES
*       Notify examines only itself and uses the same implementation for
*       SetUData and SetUDataOnce. A container may override these methods to
*       visit all matches or stop after the first match. Normal setter
*       notifications can run synchronously.
*
*/
IPTR Notify__MUIM_SetUData(struct IClass *cl, Object *obj,
    struct MUIP_SetUData *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);

    if (data->mnd_UserData == msg->udata)
    {
        set(obj, msg->attr, msg->val);
        return TRUE;
    }
    return FALSE;
}


/****** Notify.mui/MUIM_WriteLong ************
*
*   NAME
*       MUIM_WriteLong
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_WriteLong, val, (IPTR)memory);
*
*   FUNCTION
*       Write val to the ULONG location addressed by memory.
*
*   INPUTS
*       val - ULONG value.
*       memory - Non-NULL, suitably aligned, writable ULONG pointer.
*
*   RESULT
*       Always TRUE after the write.
*
*   NOTES
*       This writes one ULONG, not an IPTR-sized value on 64-bit targets. No
*       bounds, alignment, or NULL checks are performed. Ownership of memory
*       does not change.
*
*/
IPTR Notify__MUIM_WriteLong(struct IClass *cl, Object *obj,
    struct MUIP_WriteLong *msg)
{
    *(msg->memory) = msg->val;
    return TRUE;
}


/****** Notify.mui/MUIM_WriteString ************
*
*   NAME
*       MUIM_WriteString
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_WriteString, (IPTR)str, (IPTR)memory);
*
*   FUNCTION
*       Copy the complete NUL-terminated string with strcpy.
*
*   INPUTS
*       str - Readable NUL-terminated source string.
*       memory - Writable destination with room for the string and its
*       terminating NUL; source and destination must not overlap.
*
*   RESULT
*       Always TRUE after the copy.
*
*   NOTES
*       No size or NULL checks are performed. The caller retains both buffers;
*       the method neither allocates nor frees storage.
*
*/
IPTR Notify__MUIM_WriteString(struct IClass *cl, Object *obj,
    struct MUIP_WriteString *msg)
{
    strcpy(msg->memory, msg->str);
    return TRUE;
}

/****** Notify.mui/MUIM_ConnectParent ************
*
*   NAME
*       MUIM_ConnectParent
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_ConnectParent, (IPTR)parent);
*
*   FUNCTION
*       Copy the parent's global-info pointer into this object.
*
*   INPUTS
*       parent - Non-NULL live object with Notify-compatible instance data.
*
*   RESULT
*       Always TRUE after copying the pointer.
*
*   NOTES
*       This borrows the global-info structure; it does not retain the parent or
*       application. Notify does not set MUIA_Parent, insert the object into a
*       container, or propagate the connection to children. Containers provide
*       those operations separately.
*
*/
IPTR Notify__MUIM_ConnectParent(struct IClass *cl, Object *obj,
    struct MUIP_ConnectParent *msg)
{
    //struct MUI_NotifyData *data = INST_DATA(cl, obj);

    /* Objects only have parents if they are inside a group or family object;
     ** no idea why MUIA_Parent belongs to the notify class then
     */
/*    data->mnd_ParentObject = msg->parent;*/
    muiGlobalInfo(obj) = muiGlobalInfo(msg->parent);
    return TRUE;
}

/****** Notify.mui/MUIM_DisconnectParent ************
*
*   NAME
*       MUIM_DisconnectParent
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_DisconnectParent);
*
*   FUNCTION
*       Return without changing this object's connection fields.
*
*   INPUTS
*       No arguments beyond MethodID.
*
*   RESULT
*       Always zero; no disconnection work is performed by Notify.
*
*   NOTES
*       AROS deliberately retains the global-info pointer here. This method also
*       leaves MUIA_Parent unchanged. Container methods may clear their parent
*       field separately. The retained pointer does not keep the application or
*       configuration object alive.
*
*/
IPTR Notify__MUIM_DisconnectParent(struct IClass *cl, Object *obj,
    struct MUIP_DisconnectParent *msg)
{
    //struct MUI_NotifyData *data = INST_DATA(cl, obj);
/*    data->mnd_ParentObject = NULL;*/
#if 0
    /* Some apps (YAM) seem to access this even after disconnection */
    muiGlobalInfo(obj) = NULL;
#endif
    return 0;
}

/****** Notify.mui/MUIM_GetConfigItem ************
*
*   NAME
*       MUIM_GetConfigItem
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_GetConfigItem, id, (IPTR)storage);
*
*   FUNCTION
*       Look up id with MUIM_Dataspace_Find on the configuration object
*       referenced by this object's global info.
*
*   INPUTS
*       id - ULONG record identifier.
*       storage - Writable IPTR location for the payload pointer.
*       obj - Object with live global info and a valid mgi_Configdata object.
*
*   RESULT
*       TRUE and a borrowed payload pointer in storage if found; FALSE with
*       storage unchanged otherwise.
*
*   NOTES
*       No payload length is returned. Do not free the pointer.
*       Configuration-record replacement, removal, or disposal can invalidate
*       it; coordinate its use with changes to the configuration dataspace.
*
*   BUGS
*       The method dereferences global info without checking for NULL. It is not
*       safe on an unconnected object merely because the object supports Notify
*       methods.
*
*/
IPTR Notify__MUIM_GetConfigItem(struct IClass *cl, Object *obj,
    struct MUIP_GetConfigItem *msg)
{
    IPTR found =
        DoMethod(muiGlobalInfo(obj)->mgi_Configdata, MUIM_Dataspace_Find,
        msg->id);

    if (found)
    {
        *msg->storage = found;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/****** Notify.mui/MUIM_Import ************
*
*   NAME
*       MUIM_Import
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Import, (IPTR)dataspace);
*
*   FUNCTION
*       Return TRUE without reading or writing any state. Notify implements this
*       method as a stub in its dispatcher.
*
*   INPUTS
*       dataspace - Declared Dataspace object argument; ignored by Notify.
*
*   RESULT
*       Always TRUE. This does not mean that state was transferred.
*
*   NOTES
*       Subclasses must implement their own persistence. Notify does not
*       traverse children or use MUIA_ObjectID here.
*
*/

/****** Notify.mui/MUIM_Export ************
*
*   NAME
*       MUIM_Export
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Export, (IPTR)dataspace);
*
*   FUNCTION
*       Return TRUE without reading or writing any state. Notify implements this
*       method as a stub in its dispatcher.
*
*   INPUTS
*       dataspace - Declared Dataspace object argument; ignored by Notify.
*
*   RESULT
*       Always TRUE. This does not mean that state was transferred.
*
*   NOTES
*       Subclasses must implement their own persistence. Notify does not
*       traverse children or use MUIA_ObjectID here.
*
*/

/****** Notify.mui/MUIM_SetUDataOnce ************
*
*   NAME
*       MUIM_SetUDataOnce
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_SetUDataOnce, udata, attr, val);
*
*   FUNCTION
*       Set attr to val if this object's MUIA_UserData matches udata.
*
*   INPUTS
*       udata - IPTR comparison value.
*       attr - Attribute identifier.
*       val - IPTR attribute value.
*
*   RESULT
*       TRUE on a UserData match, FALSE otherwise. The setter result is ignored.
*
*   NOTES
*       Notify examines only itself and uses the same implementation for
*       SetUData and SetUDataOnce. A container may override these methods to
*       visit all matches or stop after the first match. Normal setter
*       notifications can run synchronously.
*
*/

/* Dispatch supported Notify messages and return their handler result.
 * Import and Export return TRUE without work; SetUDataOnce shares SetUData.
 * Forward all other messages to rootclass. The caller supplies a live
 * object and a correctly sized message.
 */
BOOPSI_DISPATCHER(IPTR, Notify_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Notify__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Notify__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Notify__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Notify__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_CallHook:
        return Notify__MUIM_CallHook(cl, obj, (APTR) msg);
    case MUIM_Export:
        return TRUE;
    case MUIM_FindUData:
        return Notify__MUIM_FindUData(cl, obj, (APTR) msg);
    case MUIM_GetUData:
        return Notify__MUIM_GetUData(cl, obj, (APTR) msg);
    case MUIM_Import:
        return TRUE;
    case MUIM_KillNotify:
        return Notify__MUIM_KillNotify(cl, obj, (APTR) msg);
    case MUIM_KillNotifyObj:
        return Notify__MUIM_KillNotifyObj(cl, obj, (APTR) msg);
    case MUIM_MultiSet:
        return Notify__MUIM_MultiSet(cl, obj, (APTR) msg);
    case MUIM_NoNotifySet:
        return Notify__MUIM_NoNotifySet(cl, obj, (APTR) msg);
    case MUIM_Notify:
        return Notify__MUIM_Notify(cl, obj, (APTR) msg);
    case MUIM_Set:
        return Notify__MUIM_Set(cl, obj, (APTR) msg);
    case MUIM_SetAsString:
        return Notify__MUIM_SetAsString(cl, obj, (APTR) msg);
    case MUIM_SetUData:
        return Notify__MUIM_SetUData(cl, obj, (APTR) msg);
    case MUIM_SetUDataOnce:
        return Notify__MUIM_SetUData(cl, obj, (APTR) msg);
            /* use Notify_SetUData */
    case MUIM_WriteLong:
        return Notify__MUIM_WriteLong(cl, obj, (APTR) msg);
    case MUIM_WriteString:
        return Notify__MUIM_WriteString(cl, obj, (APTR) msg);
    case MUIM_ConnectParent:
        return Notify__MUIM_ConnectParent(cl, obj, (APTR) msg);
    case MUIM_DisconnectParent:
        return Notify__MUIM_DisconnectParent(cl, obj, (APTR) msg);
    case MUIM_GetConfigItem:
        return Notify__MUIM_GetConfigItem(cl, obj, (APTR) msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Notify_desc =
{
    MUIC_Notify,                /* Class name */
    ROOTCLASS,                  /* super class name */
    sizeof(struct MUI_NotifyData),      /* size of class's own data */
    (void *) Notify_Dispatcher   /* class dispatcher */
};
