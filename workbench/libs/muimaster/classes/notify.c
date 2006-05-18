/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <clib/alib_protos.h>
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

#ifdef __AROS__
AROS_UFH2S(void, cpy_func,
    AROS_UFHA(UBYTE, chr, D0),
    AROS_UFHA(STRPTR *, strPtrPtr, A3))
{
    AROS_USERFUNC_INIT
    
    *(*strPtrPtr)++ = chr;
    
    AROS_USERFUNC_EXIT
}

AROS_UFH2S(void, len_func,
    AROS_UFHA(UBYTE, chr, D0),
    AROS_UFHA(LONG *, lenPtr, A3))
{
    AROS_USERFUNC_INIT
    
    (*lenPtr)++;
    
    AROS_USERFUNC_EXIT
}
#endif


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

/*
 * Notification handler
 */
typedef struct NotifyNode {
    struct MinNode nn_Node;
    BOOL        nn_Active;   /* When TRUE, it means that the notification is currently being handled */
                             /* It's used to prevent loops */
    ULONG       nn_TrigAttr;
    ULONG       nn_TrigVal;
    APTR        nn_DestObj;
    ULONG       nn_NumParams;
    IPTR       *nn_Params; /* FIXME: use nn_Params[1] and tweak stuff below */
} *NNode;

static struct NotifyNode *CreateNNode (struct MUI_NotifyData *data, struct MUIP_Notify *msg)
{
    ULONG i;

    struct NotifyNode *nnode = mui_alloc_struct(struct NotifyNode);
    if (!nnode) return NULL;

    nnode->nn_Active    = FALSE;
    nnode->nn_TrigAttr  = msg->TrigAttr;
    nnode->nn_TrigVal   = msg->TrigVal;
    nnode->nn_DestObj   = msg->DestObj;
    nnode->nn_NumParams = msg->FollowParams;

    /* Allocate one more IPTR (FollowParams + 1) as some ext apps/classes
       forget trailing NULLs in methods like MUIM_MultiSet and MUI seems
       like it can live with that (without crashing) */
    
    if ((nnode->nn_Params = (IPTR *)mui_alloc((msg->FollowParams + 1) * sizeof(IPTR))))
    {
	for (i = 0; i < msg->FollowParams; i++)
	{
	    nnode->nn_Params[i] = *(&msg->FollowParams + i + 1);
	}
	return nnode;
    }

    mui_free(nnode);

    return NULL;
}

static void DeleteNNode (struct MUI_NotifyData *data, struct NotifyNode *nnode)
{
    mui_free(nnode->nn_Params);
    mui_free(nnode);
}


static ULONG Notify_OMSET(struct IClass *cl, Object *obj, struct opSet *msg);

/*
 * OM_NEW
 */
static IPTR Notify_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_NotifyData *data;
    struct TagItem        *tags = msg->ops_AttrList;
    struct TagItem        *tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);

    while ((tag = NextTagItem((struct TagItem **)&tags)) != NULL)
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
	        data->mnd_ObjectID = (ULONG)tag->ti_Data;
	        break;

	    case MUIA_UserData:
	        data->mnd_UserData = (IPTR)tag->ti_Data;
	        break;
	}
    }

    return (IPTR)obj;
}


/*
 * OM_DISPOSE
 */
static ULONG Notify_Dispose(struct IClass *cl, Object *obj, Msg msg)
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
	mui_free(data->mnd_NotifyList);
    }

    return DoSuperMethodA(cl, obj, msg);
}

static void check_notify (NNode nnode, Object *obj, struct TagItem *tag)
{
    STACKULONG newparams[8];
    STACKULONG *params;
    APTR       destobj;
    int        i;

    /* is it the good attribute ? */
    if (tag->ti_Tag != nnode->nn_TrigAttr)
	return;

    /* Is the notification already being performed? */
    if (nnode->nn_Active)
    {
	D(bug("Notifyloop detected!\n"));
        return;
    }

    if
    (
        nnode->nn_TrigVal == tag->ti_Data   ||
	nnode->nn_TrigVal == MUIV_EveryTime
    )
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


	params = nnode->nn_Params;
	if (nnode->nn_TrigVal == MUIV_EveryTime)
	{
	    newparams[0] = nnode->nn_Params[0];

  	    for (i = 1; i < nnode->nn_NumParams; i++)
	    {
	        newparams[i] = nnode->nn_Params[i];

	        switch(newparams[i])
	        {
		    case MUIV_TriggerValue:
		        newparams[i] = tag->ti_Data;
		        break;

		    case MUIV_NotTriggerValue:
		        newparams[i] = !tag->ti_Data;
		        break;
	        }
	    }

	    params = newparams;
	}

	nnode->nn_Active = TRUE;

	/* call method */
	DoMethodA(destobj, (Msg)params);

	nnode->nn_Active = FALSE;
    }
}

/*
 * OM_SET
 */
static ULONG Notify_OMSET(struct IClass *cl, Object *obj, struct opSet *msg)
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
    while ((tag = NextTagItem((struct TagItem **)&tags)) != NULL)
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
	        data->mnd_ObjectID = (ULONG)tag->ti_Data;
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
	for
	(
	    node = data->mnd_NotifyList->mlh_Head;
	    node->mln_Succ;
	    node = node->mln_Succ
	)
	{
	    check_notify((NNode)node, obj, tag);
	}
   }

    return 0;
}


/*
 * OM_GET
 */
static ULONG Notify_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
/* small macro to simplify return value storage */
#define STORE *(msg->opg_Storage)

    struct MUI_NotifyData *data = INST_DATA(cl, obj);

//kprintf("*** Notify->GET\n");
    switch (msg->opg_AttrID)
    {
    case MUIA_ApplicationObject:
	if (data->mnd_GlobalInfo) STORE = (IPTR)data->mnd_GlobalInfo->mgi_ApplicationObject;
	else  STORE = 0;
	return 1;

    case MUIA_AppMessage: /* struct AppMessage ? */
	STORE = 0;
	return 1;

    case MUIA_HelpLine:
	STORE = (IPTR)data->mnd_HelpLine;
	return 1;

    case MUIA_HelpNode:
	STORE = (IPTR)data->mnd_HelpNode;
	return 1;

    case MUIA_ObjectID:
	STORE = (IPTR)data->mnd_ObjectID;
	return 1;

    case MUIA_Parent:
	STORE = (IPTR)data->mnd_ParentObject;
	return 1;

    case MUIA_Revision:
	STORE = __revision;
	return 1;

    case MUIA_UserData:
	STORE = data->mnd_UserData;
	return 1;

    case MUIA_Version:
	STORE = __version;
	return 1;
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}


/*
 * MUIM_CallHook : Call a standard amiga callback hook, defined by a Hook
 * structure.
 */
static ULONG Notify_CallHook(struct IClass *cl, Object *obj, struct MUIP_CallHook *msg)
{
    if (msg->Hook->h_Entry)
	return CallHookPkt(msg->Hook,obj, &msg->param1);
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
static IPTR Notify_FindUData(struct IClass *cl, Object *obj, struct MUIP_FindUData *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);

    if (data->mnd_UserData == msg->udata)
    {
	return (IPTR)obj;
    }
    return 0L;
}


/*
 * MUIM_GetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and gets <attr> to <storage> for itself
 * in this case.
 */
static ULONG Notify_GetUData(struct IClass *cl, Object *obj, struct MUIP_GetUData *msg)
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
static ULONG Notify_KillNotify(struct IClass *cl, Object *obj, struct MUIP_KillNotify *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);
    struct MinNode        *node;
    struct NotifyNode     *nnode;

    if (!data->mnd_NotifyList) return 0;

    for (node = data->mnd_NotifyList->mlh_Head; node->mln_Succ; node = node->mln_Succ)
    {
	nnode = (NNode)node;
	if (msg->TrigAttr == nnode->nn_TrigAttr)
	{
	    Remove((struct Node *)node);
	    DeleteNNode(data, nnode);
	    return 1;
	}
    }
    return 0;
}


/*
 * MUIM_KillNotifyObj : originally undocumented !
 * Supposed to kill a notification with a given attr and a given dest.
 */
static ULONG Notify_KillNotifyObj(struct IClass *cl, Object *obj, struct MUIP_KillNotifyObj *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);
    struct MinNode        *node;
    struct NotifyNode     *nnode;

    if (!data->mnd_NotifyList) return 0;

    for (node = data->mnd_NotifyList->mlh_Head; node->mln_Succ; node = node->mln_Succ)
    {
	nnode = (NNode)node;
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


/*
 * MUIM_MultiSet : Set an attribute for multiple objects.
 */
static ULONG Notify_MultiSet(struct IClass *cl, Object *obj, struct MUIP_MultiSet *msg)
{
    IPTR *destobj_p;
    for (destobj_p = (IPTR*)&msg->obj; (*destobj_p) != NULL; destobj_p++)
    {
	set((APTR)*destobj_p, msg->attr, msg->val);
    }
    return TRUE;
}


/*
 * MUIM_NoNotifySet : Acts like MUIM_Set but doesn't trigger any notification.
 */
static ULONG Notify_NoNotifySet(struct IClass *cl, Object *obj, struct MUIP_NoNotifySet *msg)
{
    return SetAttrs(obj, MUIA_NoNotify, TRUE, msg->attr, msg->val, TAG_DONE);
}


/*
 * MUIM_Notify : Add a notification event handler to an object.
 */
static ULONG Notify_Notify(struct IClass *cl, Object *obj, struct MUIP_Notify *msg)
{
    struct MUI_NotifyData *data = INST_DATA(cl, obj);
    struct NotifyNode     *nnode;

    if ((msg->FollowParams < 1) || (msg->FollowParams > 8))
	return FALSE;

    if (data->mnd_NotifyList == NULL)
    {
	if (!(data->mnd_NotifyList = mui_alloc_struct(struct MinList)))
	    return FALSE;
	NewList((struct List*)data->mnd_NotifyList);
    }

    nnode = CreateNNode(data, msg);
    if (NULL == nnode)
	return FALSE;

    AddTail((struct List *)data->mnd_NotifyList, (struct Node *)nnode);
    return TRUE;
}


/*
 * MUIM_Set : Set an attribute to a value, useful within a MUIM_Notify method.
 */
static ULONG Notify_Set(struct IClass *cl, Object *obj, struct MUIP_Set *msg)
{
    return set(obj, msg->attr, msg->val);
}

/*
 * MUIM_SetAsString : Set a (text kind) attribute to a string.
 */
static ULONG Notify_SetAsString(struct IClass *cl, Object *obj, struct MUIP_SetAsString *msg)
{
    STRPTR txt;
    LONG txt_len;

#ifndef __AROS__
    static const ULONG len_func = 0x52934e75; /* addq.l  #1,(A3) ; rts */
    static const ULONG cpy_func = 0x16c04e75; /* move.b d0,(a3)+ ; rts */
#endif

    txt_len = 0;
    RawDoFmt(msg->format, (ULONG *)&msg->val,
	     (VOID_FUNC)AROS_ASMSYMNAME(len_func), &txt_len);

/*      D(bug("Notify_SetAsString: fmt=%s, txtlen=%d\n", msg->format, txt_len)); */
    txt = AllocVec(txt_len + 1, 0);
    if (NULL == txt)
	return FALSE;

#ifdef __AROS__
    {
    	STRPTR txtptr = txt;
	RawDoFmt(msg->format, (ULONG *)&msg->val,
		 (VOID_FUNC)AROS_ASMSYMNAME(cpy_func), &txtptr);
    }  
#else
    RawDoFmt(msg->format, (ULONG *)&msg->val,
	     (VOID_FUNC)AROS_ASMSYMNAME(cpy_func), txt);
#endif

    set(obj, msg->attr, (IPTR)txt);
    FreeVec(txt);

    return TRUE;
}


/*
 * MUIM_SetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and sets <attr> to <val> for itself in this case.
 */
static ULONG Notify_SetUData(struct IClass *cl, Object *obj, struct MUIP_SetUData *msg)
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
static ULONG Notify_WriteLong(struct IClass *cl, Object *obj, struct MUIP_WriteLong *msg)
{
    *(msg->memory) = msg->val;
    return TRUE;
}


/*
 * MUIM_WriteString : This method simply copies a string somewhere to memory.
 */
static ULONG Notify_WriteString(struct IClass *cl, Object *obj, struct MUIP_WriteString *msg)
{
    strcpy(msg->memory, msg->str);
    return TRUE;
}

/**************************************************************************
 MUIM_ConnectParent
**************************************************************************/
static ULONG Notify_ConnectParent(struct IClass *cl, Object *obj, struct MUIP_ConnectParent *msg)
{
    //struct MUI_NotifyData *data = INST_DATA(cl, obj);

    /* Objects only have parents if they are inside a group or family object, no idea
    ** why MUIA_Parent belongs to the notify class then
    */
/*    data->mnd_ParentObject = msg->parent;*/
    muiGlobalInfo(obj) = muiGlobalInfo(msg->parent);
    return TRUE;
}

/**************************************************************************
 MUIM_DisconnectParent
**************************************************************************/
static ULONG Notify_DisconnectParent(struct IClass *cl, Object *obj, struct MUIP_DisconnectParent *msg)
{
    //struct MUI_NotifyData *data = INST_DATA(cl, obj);
/*    data->mnd_ParentObject = NULL;*/
    muiGlobalInfo(obj) = NULL;
    return 0;
}

/**************************************************************************
 MUIM_GetConfigItem
**************************************************************************/
static ULONG Notify_GetConfigItem(struct IClass *cl, Object *obj, struct MUIP_GetConfigItem *msg)
{
    IPTR found = DoMethod(muiGlobalInfo(obj)->mgi_Configdata,MUIM_Dataspace_Find,msg->id);
    
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


BOOPSI_DISPATCHER(IPTR, Notify_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:
	    return Notify_New(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE:
	    return Notify_Dispose(cl, obj, msg);
	case OM_SET:
	    return Notify_OMSET(cl, obj, (struct opSet *)msg);
	case OM_GET:
	    return Notify_Get(cl, obj, (struct opGet *)msg);
	case MUIM_CallHook :
	    return Notify_CallHook(cl, obj, (APTR)msg);
	case MUIM_Export :
	    return TRUE;
	case MUIM_FindUData :
	    return Notify_FindUData(cl, obj, (APTR)msg);
	case MUIM_GetUData :
	    return Notify_GetUData(cl, obj, (APTR)msg);
	case MUIM_Import :
	    return TRUE;
	case MUIM_KillNotify :
	    return Notify_KillNotify(cl, obj, (APTR)msg);
	case MUIM_KillNotifyObj :
	    return Notify_KillNotifyObj(cl, obj, (APTR)msg);
	case MUIM_MultiSet :
	    return Notify_MultiSet(cl, obj, (APTR)msg);
	case MUIM_NoNotifySet :
	    return Notify_NoNotifySet(cl, obj, (APTR)msg);
	case MUIM_Notify :
	    return Notify_Notify(cl, obj, (APTR)msg);
	case MUIM_Set :
	    return Notify_Set(cl, obj, (APTR)msg);
	case MUIM_SetAsString:
	    return Notify_SetAsString(cl, obj, (APTR)msg);
	case MUIM_SetUData :
	    return Notify_SetUData(cl, obj, (APTR)msg);
	case MUIM_SetUDataOnce : /* use Notify_SetUData */
	    return Notify_SetUData(cl, obj, (APTR)msg);
	case MUIM_WriteLong :
	    return Notify_WriteLong(cl, obj, (APTR)msg);
	case MUIM_WriteString :
	    return Notify_WriteString(cl, obj, (APTR)msg);
	case MUIM_ConnectParent:
	    return Notify_ConnectParent(cl,obj,(APTR)msg);
	case MUIM_DisconnectParent:
	    return Notify_DisconnectParent(cl,obj,(APTR)msg);
	case MUIM_GetConfigItem:
	    return Notify_GetConfigItem(cl,obj,(APTR)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Notify_desc = {
    MUIC_Notify,                        /* Class name */
    ROOTCLASS,                          /* super class name */
    sizeof(struct MUI_NotifyData),      /* size of class own datas */
    (void*)Notify_Dispatcher            /* class dispatcher */
};
