/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

#ifdef _AROS
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#endif

#include <zunepriv.h>
#include <builtin.h>
#include <renderinfo.h>
#include <Notify.h>
#include <notifydata.h>
#include <stdlib.h>
#include <string.h>

/*
 * Notify class is superclass of all other MUI classes.
 */

/*
MUIA_ApplicationObject [..G]   done
MUIA_AppMessage [..G]          dummy, no struct AppMessage
MUIA_HelpLine [ISG]            done
MUIA_HelpNode [ISG]            done
MUIA_NoNotify [.S.]            done
MUIA_ObjectID [ISG]            done
MUIA_Parent [..G]              done
MUIA_Revision [..G]            done
MUIA_UserData [ISG]            done
MUIA_Version [..G]             done

MUIM_CallHook                  done
MUIM_Export                    dummy & redefine in subclasses w/ childs
MUIM_FindUData                 redefine in subclasses w/ childs
MUIM_GetConfigItem
MUIM_GetUData                  redefine in subclasses w/ childs
MUIM_Import                    dummy & redefine in subclasses w/ childs
MUIM_KillNotify                done
MUIM_KillNotifyObj             done (semantic ?)
MUIM_MultiSet                  done
MUIM_NoNotifySet               done
MUIM_Notify                    done
MUIM_Set                       done
MUIM_SetAsString               done
MUIM_SetUData                  redefine in subclasses w/ childs
MUIM_SetUDataOnce              redefine in subclasses w/ childs
MUIM_WriteLong                 done
MUIM_WriteString               done
*/

static const int __version = 1;
static const int __revision = 1;

static GMemChunk *nodeMemChunk = NULL;
static GMemChunk *listMemChunk = NULL;

/*
 * Notification handler
 */
typedef struct NotifyNode {
    struct MinNode nn_Node;
    IPTR        nn_OldValue; /* save old attrib value to detect endless loop */
    ULONG       nn_TrigAttr;
    ULONG       nn_TrigVal;
    APTR        nn_DestObj;
    ULONG       nn_NumParams;
    ULONG       *nn_Params; /* FIXME: use nn_Params[1] and tweak stuff below */
} *NNode;

static void
destroy_memchunk (void)
{
kprintf("*** %s:%ld: destroy_memchunk()\n", __FILE__, __LINE__);
    if (nodeMemChunk)
    {
	g_mem_chunk_destroy(nodeMemChunk);
	nodeMemChunk = NULL;
    }

    if (listMemChunk)
    {
	g_mem_chunk_destroy(listMemChunk);
	listMemChunk = NULL;
    }
}

static struct NotifyNode *
CreateNNode (struct MUI_NotifyData *data, struct MUIP_Notify *msg, ULONG oldval)
{
    ULONG i;

    struct NotifyNode *nnode =
	g_chunk_new(struct NotifyNode, nodeMemChunk);
    nnode->nn_OldValue = oldval;
    nnode->nn_TrigAttr = msg->TrigAttr;
    nnode->nn_TrigVal  = msg->TrigVal;
    nnode->nn_DestObj  = msg->DestObj;
    nnode->nn_NumParams = msg->FollowParams;
    nnode->nn_Params = (ULONG *)g_malloc(msg->FollowParams * sizeof(ULONG));

    for (i = 0; i < msg->FollowParams; i++)
    {
	nnode->nn_Params[i] = *(&msg->FollowParams + i + 1);
	/*g_print("param %ld = %ld\n", i, *(nnode->nn_Params + i));*/
    }
    return nnode;
}

static void
DeleteNNode (struct MUI_NotifyData *data, struct NotifyNode *nnode)
{
    g_free(nnode->nn_Params);
    g_chunk_free(nnode, nodeMemChunk);
}


static ULONG mSet(struct IClass *cl, Object *obj, struct opSet *msg);

/*
 * OM_NEW
 */
static ULONG 
mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_NotifyData *data;
    struct TagItem        *tags = msg->ops_AttrList;
    struct TagItem        *tag;

//kprintf("*** Notify->NEW\n");
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
    {
//kprintf("*** Notify->NEW: DoSuperMethodA() failed\n");
	return FALSE;
    }

    data = INST_DATA(cl, obj);

    while ((tag = NextTagItem(&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	case MUIA_HelpLine:
	    data->mnd_HelpLine = (LONG)tag->ti_Data;
	    break;
	case MUIA_HelpNode:
	    data->mnd_HelpNode = (STRPTR)tag->ti_Data;
	    break;
	case MUIA_ObjectID:
	    data->mnd_ObjectID = (STRPTR)tag->ti_Data;
	    break;
	case MUIA_UserData:
	    data->mnd_UserData = (ULONG)tag->ti_Data;
	    break;
	}
    }


//kprintf("*** Notify->NEW: OK\n");
    return (ULONG)obj;
}


/*
 * OM_DISPOSE
 */
static ULONG 
mDispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MinNode *node, *tmp;
    struct MUI_NotifyData *data = INST_DATA(cl, obj);

    if (data->mnd_NotifyList)
    {
	for (node = data->mnd_NotifyList->mlh_Head; node->mln_Succ ; node = tmp)
	{
	    tmp = node->mln_Succ;
	    DeleteNNode(data, (struct NotifyNode *)node);
	}
	g_chunk_free(data->mnd_NotifyList, listMemChunk);
    }

    return DoSuperMethodA(cl, obj, msg);
}

static void
check_notify (NNode nnode, Object *obj, struct TagItem *tag, BOOL no_notify)
{
    APTR    destobj;
    ULONG   backup[8];
    int     i;

    /* is it the good attribute ? */
    if (tag->ti_Tag != nnode->nn_TrigAttr)
	return;
      
    /* value changed since last set ? */
    if (tag->ti_Data == nnode->nn_OldValue)
	return;

    nnode->nn_OldValue = tag->ti_Data;

    /* trigger notification for the new value ? */

    if (no_notify)
	return;

    if ((nnode->nn_TrigVal == tag->ti_Data)
	|| (nnode->nn_TrigVal == MUIV_EveryTime))
    {
	switch((ULONG)nnode->nn_DestObj)
	{ 
	    case MUIV_Notify_Application:
		destobj = _app(obj);
		break;
	    case MUIV_Notify_Self:
		destobj = obj;
		break;
	    case MUIV_Notify_Window:
		destobj = _win(obj);
		break;
	    default:
		destobj = nnode->nn_DestObj;
	}
	memcpy(backup, nnode->nn_Params,
	       nnode->nn_NumParams * sizeof(ULONG));
	for (i = 1; i < nnode->nn_NumParams; i++)
	{
	    switch(nnode->nn_Params[i])
	    {
		case MUIV_TriggerValue:
		    nnode->nn_Params[i] = tag->ti_Data;
		    break;
		case MUIV_NotTriggerValue:
		    nnode->nn_Params[i] = !tag->ti_Data;
		    break;
	    }
	}
	/* call method */
	DoMethodA(destobj, (Msg)nnode->nn_Params);
	/* restore params in handler */
	memcpy(nnode->nn_Params, backup,
	       nnode->nn_NumParams * sizeof(ULONG));
    }
}

/*
 * OM_SET 
 */
static ULONG 
mSet(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);
    struct TagItem        *tags = msg->ops_AttrList;
    BOOL                   no_notify = FALSE;
    struct TagItem        *tag;
    struct MinNode        *node;

    /* There are many ways to find out what tag items provided by set()
    ** we do know. The best way should be using NextTagItem() and simply
    ** browsing through the list.
    */
    while ((tag = NextTagItem(&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	case MUIA_HelpLine:
	    data->mnd_HelpLine = (LONG)tag->ti_Data;
	    break;
	case MUIA_HelpNode:
	    data->mnd_HelpNode = (STRPTR)tag->ti_Data;
	    break;
	case MUIA_NoNotify:
	    if (tag->ti_Data == TRUE)
		no_notify = TRUE;
	    break;
	case MUIA_ObjectID:
	    data->mnd_ObjectID = (STRPTR)tag->ti_Data;
	    break;
	case MUIA_UserData:
	    data->mnd_UserData = tag->ti_Data;
	    break;
	}
    }

    /*
     * check for notifications
     */
    if (data->mnd_NotifyList == NULL)
	return TRUE;
    tags = msg->ops_AttrList;
    while ((tag = NextTagItem(&tags)) != NULL)
    {
	for (node = data->mnd_NotifyList->mlh_Head; node->mln_Succ;
	     node = node->mln_Succ)
	{
	    check_notify((NNode)node, obj, tag, no_notify);
	}
   }

    return TRUE;
}


/*
 * OM_GET
 */
static ULONG
mGet(struct IClass *cl, Object *obj, struct opGet *msg)
{
/* small macro to simplify return value storage */
#define STORE *(msg->opg_Storage)

    struct MUI_NotifyData *data = INST_DATA(cl, obj);

//kprintf("*** Notify->GET\n");
    switch (msg->opg_AttrID)
    {
    case MUIA_ApplicationObject:
	if (data->mnd_GlobalInfo)
	    STORE = (ULONG)data->mnd_GlobalInfo->mgi_ApplicationObject;
	else
	    STORE = 0;
	return(TRUE);
    case MUIA_AppMessage: /* struct AppMessage ? */
	STORE = 0;
	return(TRUE);
    case MUIA_HelpLine:
	STORE = (ULONG)data->mnd_HelpLine;
	break;
    case MUIA_HelpNode:
	STORE = (ULONG)data->mnd_HelpNode;
	break;
    case MUIA_ObjectID:
	STORE = (ULONG)data->mnd_ObjectID;
	return(TRUE);
    case MUIA_Parent:
	STORE = (ULONG)data->mnd_ParentObject;
	return(TRUE);
    case MUIA_Revision:
	STORE = __revision;
	return(TRUE);
    case MUIA_UserData:
//kprintf("*** Notify->GET MUIA_UserData ($%lx)\n", data->mnd_UserData);
	STORE = data->mnd_UserData;
//kprintf("*** Notify->GET MUIA_UserData OK\n");
	return(TRUE);
    case MUIA_Version:
	STORE = __version;
	return(TRUE);
    }

    /* Getting unkown attr.
     */
//kprintf("*** Notify->GET: unknown attribute $%lx\n", msg->opg_AttrID);
    return FALSE;
#undef STORE
}


/*
 * MUIM_CallHook : Call a standard amiga callback hook, defined by a Hook
 * structure.
 */
static ULONG
mCallHook(struct IClass *cl, Object *obj, struct MUIP_CallHook *msg)
{
    if (msg->Hook->h_Entry)
	return (ULONG)(*(msg->Hook->h_Entry))(msg->Hook, obj, &msg->param1);
    else return FALSE;
}


/*
 * MUIM_Export : to export an objects "contents" to a dataspace object.
 */
/* nothing to export */

/*
 * MUIM_FindUData : tests if the MUIA_UserData of the object
 * contains the given <udata> and returns the object pointer in this case.
 */
static ULONG
mFindUData(struct IClass *cl, Object *obj, struct MUIP_FindUData *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);

    if (data->mnd_UserData == msg->udata)
    {
	return (ULONG)obj;
    }
    return 0L;
}


/*
 * MUIM_GetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and gets <attr> to <storage> for itself
 * in this case.
 */
static ULONG
mGetUData(struct IClass *cl, Object *obj, struct MUIP_GetUData *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);

    if (data->mnd_UserData == msg->udata)
    {
	get(obj, msg->attr, msg->storage);
	return TRUE;
    }
    return FALSE;
}


/*
 * MUIM_Import : to import an objects "contents" from a dataspace object.
 */
/* nothing to import */

/*
 * MUIM_KillNotify : kills previously given notifications on specific attributes.
 */
static ULONG
mKillNotify(struct IClass *cl, Object *obj, struct MUIP_KillNotify *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);
    struct MinNode        *node;
    struct NotifyNode     *nnode;

    g_return_val_if_fail(data->mnd_NotifyList != NULL, FALSE);

    for (node = data->mnd_NotifyList->mlh_Head; node->mln_Succ; node = node->mln_Succ)
    {
	nnode = (NNode)node;
	if (msg->TrigAttr == nnode->nn_TrigAttr)
	{
	    Remove((struct Node *)node);
	    DeleteNNode(data, nnode);
	    return TRUE;
	}
    }
    return FALSE;
}


/*
 * MUIM_KillNotifyObj : originally undocumented !
 * Supposed to kill a notification with a given attr and a given dest.
 */
static ULONG
mKillNotifyObj(struct IClass *cl, Object *obj, struct MUIP_KillNotifyObj *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);
    struct MinNode        *node;
    struct NotifyNode     *nnode;

    g_return_val_if_fail(data->mnd_NotifyList != NULL, FALSE);

    for (node = data->mnd_NotifyList->mlh_Head; node->mln_Succ; node = node->mln_Succ)
    {
	nnode = (NNode)node;
	if ((msg->TrigAttr == nnode->nn_TrigAttr)
	    && (msg->dest == nnode->nn_DestObj))
	{
	    Remove((struct Node *)node);
	    DeleteNNode(data, nnode);
	    return TRUE;
	}
    }
    return FALSE;
}


/*
 * MUIM_MultiSet : Set an attribute for multiple objects.
 */
static ULONG 
mMultiSet(struct IClass *cl, Object *obj, struct MUIP_MultiSet *msg)
{
    ULONG *destobj_p;

    for (destobj_p = (ULONG*)&msg->obj; destobj_p != NULL; destobj_p++)
    {
	set((APTR)*destobj_p, msg->attr, msg->val);
    }
    return TRUE;
}


/*
 * MUIM_NoNotifySet : Acts like MUIM_Set but doesn't trigger any notification.
 */
static ULONG 
mNoNotifySet(struct IClass *cl, Object *obj, struct MUIP_NoNotifySet *msg)
{
    return SetAttrs(obj, MUIA_NoNotify, TRUE, msg->attr, msg->val, TAG_DONE);
}


/*
 * MUIM_Notify : Add a notification event handler to an object.
 */
static ULONG
mNotify(struct IClass *cl, Object *obj, struct MUIP_Notify *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);
    struct NotifyNode     *nnode;
    ULONG                 oldval;

//kprintf("*** Notify->MUIM_Notify\n");
    if ((msg->FollowParams < 1) || (msg->FollowParams > 8))
	return FALSE;

//kprintf("*** Notify->MUIM_Notify: get old value\n");
    if (!get(obj, msg->TrigAttr, &oldval))
	return FALSE;

    if (data->mnd_NotifyList == NULL)
    {
	if (!nodeMemChunk)
	{
//kprintf("*** Notify->MUIM_Notify: creating memory chunks\n");
	    nodeMemChunk =
		g_mem_chunk_create(struct NotifyNode, 20, G_ALLOC_AND_FREE);
	    ASSERT(nodeMemChunk != NULL);
	    listMemChunk =
		g_mem_chunk_create(struct MinList, 20, G_ALLOC_AND_FREE);
	    ASSERT(listMemChunk != NULL);
	    g_atexit(destroy_memchunk);
	}

//kprintf("*** Notify->MUIM_Notify: creating notify list for object\n");
	data->mnd_NotifyList = g_chunk_new(struct MinList, listMemChunk);
	InitMinList(data->mnd_NotifyList);
    }

//kprintf("*** Notify->MUIM_Notify: creating NotifyNode (oldval=$%lx)\n", oldval);
    nnode = CreateNNode(data, msg, oldval);
    AddTail((struct List *)data->mnd_NotifyList, (struct Node *)nnode);
    return TRUE;
}


/*
 * MUIM_Set : Set an attribute to a value, useful within a MUIM_Notify method.
 */
static ULONG 
mNotifySet(struct IClass *cl, Object *obj, struct MUIP_Set *msg)
{
    return set(obj, msg->attr, msg->val);
}


/*
 * MUIM_SetAsString : Set a (text kind) attribute to a string.
 */
static ULONG 
mSetAsString(struct IClass *cl, Object *obj, struct MUIP_SetAsString *msg)
{
#warning FIXME: MUIM_SetAsString not yet implemented
    char buf[2048];

    g_error("MUIM_SetAsString : not yet implemented, please complain !\n");
/*      g_vsnprintf(buf, 2048, msg->format, (va_list)&msg->val); */
/*      set(obj, msg->attr, (ULONG)buf); */
    return TRUE;
}


/*
 * MUIM_SetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and sets <attr> to <val> for itself in this case.
 */
static ULONG 
mSetUData(struct IClass *cl, Object *obj, struct MUIP_SetUData *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);

    if (data->mnd_UserData == msg->udata)
    {
	set(obj, msg->attr, msg->val);
	return TRUE;
    }
    return FALSE;
}


/*
 * MUIM_WriteLong : This method simply writes a longword somewhere to memory.
 */
static ULONG 
mWriteLong(struct IClass *cl, Object *obj, struct MUIP_WriteLong *msg)
{
    *(msg->memory) = msg->val;
    return TRUE;
}


/*
 * MUIM_WriteString : This method simply copies a string somewhere to memory.
 */
static ULONG 
mWriteString(struct IClass *cl, Object *obj, struct MUIP_WriteString *msg)
{
    strcpy(msg->memory, msg->str);
    return TRUE;
}


/*
 * The class dispatcher
 */
AROS_UFH3S(IPTR, MyDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
    switch (msg->MethodID)
    {
	/* Whenever an object shall be created using NewObject(), it will be
	** sent a OM_NEW method.
	*/
	case OM_NEW:
	    return(mNew(cl, obj, (struct opSet *) msg));
	case OM_DISPOSE:
	    return(mDispose(cl, obj, msg));
	case OM_SET:
	    return(mSet(cl, obj, (struct opSet *)msg));
	case OM_GET:
	    return(mGet(cl, obj, (struct opGet *)msg));
	case MUIM_CallHook :
	    return(mCallHook(cl, obj, (APTR)msg));
	case MUIM_Export :
	    return TRUE;
	case MUIM_FindUData :
	    return(mFindUData(cl, obj, (APTR)msg));
	case MUIM_GetUData :
	    return(mGetUData(cl, obj, (APTR)msg));
	case MUIM_Import :
	    return TRUE;
	case MUIM_KillNotify :
	    return(mKillNotify(cl, obj, (APTR)msg));
	case MUIM_KillNotifyObj :
	    return(mKillNotifyObj(cl, obj, (APTR)msg));
	case MUIM_MultiSet :
	    return(mMultiSet(cl, obj, (APTR)msg));
	case MUIM_NoNotifySet :
	    return(mNoNotifySet(cl, obj, (APTR)msg));
	case MUIM_Notify :
	    return(mNotify(cl, obj, (APTR)msg));
	case MUIM_Set :
	    return(mNotifySet(cl, obj, (APTR)msg));
	case MUIM_SetAsString :
	    return(mSetAsString(cl, obj, (APTR)msg));
	case MUIM_SetUData :
	    return(mSetUData(cl, obj, (APTR)msg));
	case MUIM_SetUDataOnce : /* use Notify_SetUData */
	    return(mSetUData(cl, obj, (APTR)msg));
	case MUIM_WriteLong :
	    return(mWriteLong(cl, obj, (APTR)msg));
	case MUIM_WriteString :
	    return(mWriteString(cl, obj, (APTR)msg));
    }

    return(DoSuperMethodA(cl, obj, msg));
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Notify_desc = {
    MUIC_Notify,                        /* Class name */
    ROOTCLASS,                          /* super class name */
    sizeof(struct MUI_NotifyData),      /* size of class own datas */
    MyDispatcher                        /* class dispatcher */
};
