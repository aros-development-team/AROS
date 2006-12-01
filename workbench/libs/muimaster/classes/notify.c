/*
    Copyright © 2002-2006, The AROS Development Team. All rights reserved.
    $Id$
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
    IPTR       *nn_NewParams; /* For MUIV_EveryTime */
} *NNode;

static struct NotifyNode *CreateNNode (struct MUI_NotifyData *data, struct MUIP_Notify *msg)
{
    ULONG i, paramsize;

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
    
    paramsize = (msg->FollowParams + 1);
    if (msg->TrigVal == MUIV_EveryTime)
    {
    	paramsize *= 2;
    }
    
    if ((nnode->nn_Params = (IPTR *)mui_alloc(paramsize * sizeof(IPTR))))
    {
	for (i = 0; i < msg->FollowParams; i++)
	{
	    nnode->nn_Params[i] = *(&msg->FollowParams + i + 1);
	}

        if (msg->TrigVal == MUIV_EveryTime)
        {
    	    nnode->nn_NewParams = nnode->nn_Params + msg->FollowParams + 1;
	    for (i = 0; i < msg->FollowParams; i++)
	    {
		nnode->nn_NewParams[i] = *(&msg->FollowParams + i + 1);
	    }
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


IPTR Notify__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg);

/*
 * OM_NEW
 */
IPTR Notify__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_NotifyData *data;
    struct TagItem        *tags = msg->ops_AttrList;
    struct TagItem        *tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);

    while ((tag = NextTagItem((const struct TagItem **)&tags)) != NULL)
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
IPTR Notify__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
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
   IPTR       *params;
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


    if (tag->ti_Tag == MUIA_Window_InputEvent)
    {   
      struct InputXpression ix;
      
      ParseIX(nnode->nn_TrigVal,&ix); 
      if (tag->ti_Data == (ix.ix_Code  | (ix.ix_Qualifier << 16)))
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
                if (muiRenderInfo(obj))     
                destobj = _win(obj);
                else return;
                break;
                default:
                destobj = nnode->nn_DestObj;
            }     
               for (i = 0; i < nnode->nn_NumParams; i++)
               {
                   if (nnode->nn_Params[i] == MUIM_CallHook)
                   {   
                   nnode->nn_Active = TRUE;   
                  
CallHookPkt(nnode->nn_Params[i+1],destobj,&nnode->nn_Params[i+2]);
                   nnode->nn_Active = FALSE; 
                   return ;
               }          
                   }    
               
      }                  

    }

    if
    (
        nnode->nn_TrigVal == tag->ti_Data   ||
	nnode->nn_TrigVal == MUIV_EveryTime
    )
    {
	switch((IPTR)nnode->nn_DestObj)
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
	        switch(nnode->nn_Params[i])
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
	DoMethodA(destobj, (Msg)params);

	nnode->nn_Active = FALSE;
    }
}

/*
 * OM_SET
 */
IPTR Notify__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
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
    while ((tag = NextTagItem((const struct TagItem **)&tags)) != NULL)
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
    while ((tag = NextTagItem((const struct TagItem **)&tags)))
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
IPTR Notify__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
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
	return TRUE;

    case MUIA_AppMessage: /* struct AppMessage ? */
	STORE = 0;
	return TRUE;

    case MUIA_HelpLine:
	STORE = (IPTR)data->mnd_HelpLine;
	return TRUE;

    case MUIA_HelpNode:
	STORE = (IPTR)data->mnd_HelpNode;
	return TRUE;

    case MUIA_ObjectID:
	STORE = (IPTR)data->mnd_ObjectID;
	return TRUE;

    case MUIA_Parent:
	STORE = (IPTR)data->mnd_ParentObject;
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

    return DoSuperMethodA(cl,obj,(Msg)msg);
}


/*
 * MUIM_CallHook : Call a standard amiga callback hook, defined by a Hook
 * structure.
 */
IPTR Notify__MUIM_CallHook(struct IClass *cl, Object *obj, struct MUIP_CallHook *msg)
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
IPTR Notify__MUIM_FindUData(struct IClass *cl, Object *obj, struct MUIP_FindUData *msg)
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
IPTR Notify__MUIM_GetUData(struct IClass *cl, Object *obj, struct MUIP_GetUData *msg)
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
IPTR Notify__MUIM_KillNotify(struct IClass *cl, Object *obj, struct MUIP_KillNotify *msg)
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
IPTR Notify__MUIM_KillNotifyObj(struct IClass *cl, Object *obj, struct MUIP_KillNotifyObj *msg)
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
IPTR Notify__MUIM_MultiSet(struct IClass *cl, Object *obj, struct MUIP_MultiSet *msg)
{
    IPTR *destobj_p;
    for (destobj_p = (IPTR*)&msg->obj; (*destobj_p) != 0; destobj_p++)
    {
	set((APTR)*destobj_p, msg->attr, msg->val);
    }
    return TRUE;
}


/*
 * MUIM_NoNotifySet : Acts like MUIM_Set but doesn't trigger any notification.
 */
IPTR Notify__MUIM_NoNotifySet(struct IClass *cl, Object *obj, struct MUIP_NoNotifySet *msg)
{
    return SetAttrs(obj, MUIA_NoNotify, TRUE, msg->attr, msg->val, TAG_DONE);
}


/*
 * MUIM_Notify : Add a notification event handler to an object.
 */
IPTR Notify__MUIM_Notify(struct IClass *cl, Object *obj, struct MUIP_Notify *msg)
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
IPTR Notify__MUIM_Set(struct IClass *cl, Object *obj, struct MUIP_Set *msg)
{
    return set(obj, msg->attr, msg->val);
}

/*
 * MUIM_SetAsString : Set a (text kind) attribute to a string.
 */
IPTR Notify__MUIM_SetAsString(struct IClass *cl, Object *obj, struct MUIP_SetAsString *msg)
{
    STRPTR txt;
    LONG txt_len;

    txt_len = 0;
    RawDoFmt(msg->format, (ULONG *)&msg->val,
	     (VOID_FUNC)AROS_ASMSYMNAME(len_func), &txt_len);

/*      D(bug("Notify_SetAsString: fmt=%s, txtlen=%d\n", msg->format, txt_len)); */
    txt = AllocVec(txt_len + 1, 0);
    if (NULL == txt)
	return FALSE;

    {
    	STRPTR txtptr = txt;
	RawDoFmt(msg->format, (ULONG *)&msg->val,
		 (VOID_FUNC)AROS_ASMSYMNAME(cpy_func), &txtptr);
    }  

    set(obj, msg->attr, (IPTR)txt);
    FreeVec(txt);

    return TRUE;
}


/*
 * MUIM_SetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and sets <attr> to <val> for itself in this case.
 */
IPTR Notify__MUIM_SetUData(struct IClass *cl, Object *obj, struct MUIP_SetUData *msg)
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
IPTR Notify__MUIM_WriteLong(struct IClass *cl, Object *obj, struct MUIP_WriteLong *msg)
{
    *(msg->memory) = msg->val;
    return TRUE;
}


/*
 * MUIM_WriteString : This method simply copies a string somewhere to memory.
 */
IPTR Notify__MUIM_WriteString(struct IClass *cl, Object *obj, struct MUIP_WriteString *msg)
{
    strcpy(msg->memory, msg->str);
    return TRUE;
}

/**************************************************************************
 MUIM_ConnectParent
**************************************************************************/
IPTR Notify__MUIM_ConnectParent(struct IClass *cl, Object *obj, struct MUIP_ConnectParent *msg)
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
IPTR Notify__MUIM_DisconnectParent(struct IClass *cl, Object *obj, struct MUIP_DisconnectParent *msg)
{
    //struct MUI_NotifyData *data = INST_DATA(cl, obj);
/*    data->mnd_ParentObject = NULL;*/
#if 0 /* Some apps (YAM) seem to access this even after disconnection (Bernd Roesch) */
    muiGlobalInfo(obj) = NULL;
#endif
    return 0;
}

/**************************************************************************
 MUIM_GetConfigItem
**************************************************************************/
IPTR Notify__MUIM_GetConfigItem(struct IClass *cl, Object *obj, struct MUIP_GetConfigItem *msg)
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
	case OM_NEW:                return Notify__OM_NEW(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE:            return Notify__OM_DISPOSE(cl, obj, msg);
	case OM_SET:                return Notify__OM_SET(cl, obj, (struct opSet *)msg);
	case OM_GET:                return Notify__OM_GET(cl, obj, (struct opGet *)msg);

	case MUIM_CallHook:         return Notify__MUIM_CallHook(cl, obj, (APTR)msg);
	case MUIM_Export:           return TRUE;
	case MUIM_FindUData:        return Notify__MUIM_FindUData(cl, obj, (APTR)msg);
	case MUIM_GetUData:         return Notify__MUIM_GetUData(cl, obj, (APTR)msg);
	case MUIM_Import:           return TRUE;
	case MUIM_KillNotify:       return Notify__MUIM_KillNotify(cl, obj, (APTR)msg);
	case MUIM_KillNotifyObj:    return Notify__MUIM_KillNotifyObj(cl, obj, (APTR)msg);
	case MUIM_MultiSet:         return Notify__MUIM_MultiSet(cl, obj, (APTR)msg);
	case MUIM_NoNotifySet:      return Notify__MUIM_NoNotifySet(cl, obj, (APTR)msg);
	case MUIM_Notify:           return Notify__MUIM_Notify(cl, obj, (APTR)msg);
	case MUIM_Set:              return Notify__MUIM_Set(cl, obj, (APTR)msg);
	case MUIM_SetAsString:      return Notify__MUIM_SetAsString(cl, obj, (APTR)msg);
	case MUIM_SetUData:         return Notify__MUIM_SetUData(cl, obj, (APTR)msg);
	case MUIM_SetUDataOnce:	    return Notify__MUIM_SetUData(cl, obj, (APTR)msg);  /* use Notify_SetUData */
	case MUIM_WriteLong:        return Notify__MUIM_WriteLong(cl, obj, (APTR)msg);
	case MUIM_WriteString:      return Notify__MUIM_WriteString(cl, obj, (APTR)msg);
	case MUIM_ConnectParent:    return Notify__MUIM_ConnectParent(cl,obj,(APTR)msg);
	case MUIM_DisconnectParent: return Notify__MUIM_DisconnectParent(cl,obj,(APTR)msg);
	case MUIM_GetConfigItem:    return Notify__MUIM_GetConfigItem(cl,obj,(APTR)msg);
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
