/* Zune -- a free Magic User Interface implementation
 * Copyright (C) 1999 David Le Corfec
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#define MYDEBUG 1
#include "debug.h"

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"

extern struct Library *MUIMasterBase;

/*
 * Application class is the master class for all
 * MUI applications. It serves as a kind of anchor
 * for all input, either coming from the user or
 * somewhere from the system, e.g. commodities
 * or ARexx messages. (hemm forget theses last 2 for Zune :)
 *
 * An application can have any number of sub windows,
 * these windows are the children of the application.
 * (FYI, it delegates child handling to a Family object).
 */

/*
MUIA_Application_Active [ISG]             needs Commodities
MUIA_Application_Author [I.G]             done
MUIA_Application_Base [I.G]               done
MUIA_Application_Broker [..G]             needs Commodities
MUIA_Application_BrokerHook [ISG]         needs Commodities
MUIA_Application_BrokerPort [..G]         needs Commodities
MUIA_Application_BrokerPri [I.G]          needs Commodities
MUIA_Application_Commands [ISG]           needs Arexx
MUIA_Application_Copyright [I.G]          done
MUIA_Application_Description [I.G]        done
MUIA_Application_DiskObject [ISG]         needs struct DiskObject
MUIA_Application_DoubleStart [..G]        not triggered yet (todo)
MUIA_Application_DropObject [IS.]         needs AppMessage
MUIA_Application_ForceQuit [..G]          not triggered yet
MUIA_Application_HelpFile [ISG]           unused/dummy
MUIA_Application_Iconified [.SG]          todo (wm interaction ?)
MUIA_Application_Menu [I.G]               unimplemented (OBSOLETE)
MUIA_Application_MenuAction [..G]         todo (will certainly be a hack)
MUIA_Application_MenuHelp [..G]           todo (ditto)
MUIA_Application_Menustrip [I..]          todo (ditto)
MUIA_Application_RexxHook [ISG]           needs Arexx
MUIA_Application_RexxMsg [..G]            needs Arexx
MUIA_Application_RexxString [.S.]         needs Arexx
MUIA_Application_SingleTask [I..]         todo
MUIA_Application_Sleep [.S.]              todo
MUIA_Application_Title [I.G]              done
MUIA_Application_UseCommodities [I..]     needs Commodities
MUIA_Application_UseRexx [I..]            needs Arexx
MUIA_Application_Version [I.G]            done
MUIA_Application_Window [I..]             done
MUIA_Application_WindowList [..G]         done

OM_ADDMEMBER                              done
OM_REMMEMBER                              done
MUIM_Application_AboutMUI                 todo
MUIM_Application_AddInputHandler          done ?
MUIM_Application_CheckRefresh             todo (implementable ?)
MUIM_Application_GetMenuCheck             OBSOLETE
MUIM_Application_GetMenuState             OBSOLETE
MUIM_Application_Iconify                  dummy (private)
MUIM_Application_Input                    OBSOLETE
MUIM_Application_InputBuffered            todo
MUIM_Application_Load
MUIM_Application_NewInput                 todo
MUIM_Application_OpenConfigWindow
MUIM_Application_PushMethod
MUIM_Application_RemInputHandler          done ?
MUIM_Application_ReturnID                 done
MUIM_Application_Save
MUIM_Application_SetConfigItem
MUIM_Application_SetMenuCheck
MUIM_Application_SetMenuState
MUIM_Application_ShowHelp

Notify.mui/MUIM_FindUData                 done
Notify.mui/MUIM_GetUData                  done
Notify.mui/MUIM_SetUData                  done
Notify.mui/MUIM_SetUDataOnce              done
 */

static const int __version = 1;
static const int __revision = 1;


/*
 * MethodQueueNode
 */
struct MQNode {
    struct MinNode    mq_Node;
    Object           *mq_Dest;
    LONG              mq_Count;
    ULONG            *mq_Msg;
};

/*
 * Allocates an MethodQueue Method
 */
static struct MQNode *CreateMQNode(LONG count)
{
    struct MQNode *mq;

    mq = (struct MQNode *)mui_alloc(sizeof(struct MQNode) + (count * sizeof(ULONG)));
    if (!mq)
	return NULL;

    mq->mq_Count = count;
    mq->mq_Msg   = (ULONG *)(((char *)mq)+sizeof(struct MQNode));
    return mq;
}

/*
 * Free an IQ Method got from CreateIQMethod()
 */
static void  DeleteMQNode(struct MQNode *mq)
{
    mui_free(mq);
}


/*
 * Queue of Return IDs
 */
struct RIDNode {
    struct MinNode  rid_Node;
    ULONG           rid_Value;
};


static struct RIDNode *CreateRIDNode(struct MUI_ApplicationData *data, ULONG retid)
{
    struct RIDNode *rid;
    if ((rid = mui_alloc_struct(struct RIDNode)))
    {
	rid->rid_Value = retid;
    }
    return rid;
}


static void  DeleteRIDNode (struct MUI_ApplicationData *data, struct RIDNode *rid)
{
    mui_free(rid);
}


/**************************************************************************
 Process a pushed method.
**************************************************************************/
static BOOL application_do_pushed_method (struct MUI_ApplicationData *data)
{
    struct MQNode  *mq;

    if ((mq = (struct MQNode *)RemHead((struct List *)&data->app_MethodQueue)))
    {
	DoMethodA(mq->mq_Dest, (Msg)mq->mq_Msg);
	DeleteMQNode(mq);
	return TRUE;
    }
    return FALSE;
}


/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Application_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ApplicationData *data;
    struct TagItem        *tags,*tag;
    BOOL   bad_childs = FALSE;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return FALSE;

    /* Initial local instance data */
    data = INST_DATA(cl, obj);

    /* init input handler list */
    NewList((struct List*)&(data->app_IHList));

    /* init input queue */
    NewList((struct List*)&(data->app_MethodQueue));

    /* init return ids queue */
    NewList((struct List*)&(data->app_ReturnIDQueue));

    /* window list */
    data->app_WindowFamily = MUI_NewObjectA(MUIC_Family, NULL);
    if (!data->app_WindowFamily)
    {
    	CoerceMethod(cl,obj,OM_DISPOSE);
	return 0;
    }

    data->app_GlobalInfo.mgi_ApplicationObject = obj;
    if (!(data->app_GlobalInfo.mgi_UserPort = CreateMsgPort()))
    {
    	CoerceMethod(cl,obj,OM_DISPOSE);
	return 0;
    }

    D(bug("muimaster.library/application.c: Message Port created at 0x%lx\n",data->app_GlobalInfo.mgi_UserPort));

    muiNotifyData(obj)->mnd_GlobalInfo = &data->app_GlobalInfo;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	case MUIA_Application_Author:
	    data->app_Author = (STRPTR)tag->ti_Data;
	    break;
	case MUIA_Application_Base:
	    data->app_Base = (STRPTR)tag->ti_Data;
	    break;
	case MUIA_Application_Copyright:
	    data->app_Copyright = (STRPTR)tag->ti_Data;
	    break;
	case MUIA_Application_Description:
	    data->app_Description = (STRPTR)tag->ti_Data;
	    break;
	case MUIA_Application_HelpFile:
	    data->app_HelpFile = (STRPTR)tag->ti_Data;
	    break;
	case MUIA_Application_SingleTask:
	    data->app_SingleTask = (BOOL)tag->ti_Data;
	    break;
	case MUIA_Application_Title:
	    data->app_Title = (STRPTR)tag->ti_Data;
	    break;
	case MUIA_Application_Version:
	    data->app_Version = (STRPTR)tag->ti_Data;
	    break;
	case MUIA_Application_Window:
	    if (tag->ti_Data) DoMethod(obj,OM_ADDMEMBER,tag->ti_Data);
	    else bad_childs = TRUE;
	    break;
	}
    }

    if (bad_childs)
    {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return 0;
    }

    DoMethod(obj, MUIM_Notify, MUIA_Application_Iconified, TRUE,
	     MUIV_Notify_Self, 1, MUIM_Application_Iconify);

#warning FIXME: prefs
#if 0
    __zune_prefs_sys_global_read(&__zprefs);
    __zune_prefs_user_global_read(&__zprefs);
    if (data->app_Title)
	__zune_prefs_user_app_read(&__zprefs, data->app_Title);
#endif

#warning FIXME: implement checking for prefs change

#if 0
#ifndef _AROS
    /* currently hardcoded range of 1-10 s for prefs files notifications */
    __zprefs.app_cfg_spy_delay = CLAMP(__zprefs.app_cfg_spy_delay, 1000, 10 * 1000);
    data->app_CfgSpyTimeout = g_timeout_add(__zprefs.app_cfg_spy_delay,
					    __zune_prefs_spy, data);
#endif
#endif

    return (ULONG)obj;
}



/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static ULONG Application_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (data->app_WindowFamily)
	MUI_DisposeObject(data->app_WindowFamily);

    if (data->app_GlobalInfo.mgi_UserPort)
    	DeleteMsgPort(data->app_GlobalInfo.mgi_UserPort);

#warning FIXME: prefs
#if 0
    __zune_prefs_release(&__zprefs);
#endif

    return DoSuperMethodA(cl, obj, msg);
}


/**************************************************************************
 OM_SET
**************************************************************************/
static ULONG mSet(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ApplicationData *data  = INST_DATA(cl, obj);
    struct TagItem             *tags  = msg->ops_AttrList;
    struct TagItem             *tag;

    /* There are many ways to find out what tag items provided by set()
    ** we do know. The best way should be using NextTagItem() and simply
    ** browsing through the list.
    */
    while ((tag = NextTagItem(&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	case MUIA_Application_HelpFile:
	    data->app_HelpFile = (STRPTR)tag->ti_Data;
	    break;
	case MUIA_Application_Iconified:
	    data->app_Iconified = (ULONG)tag->ti_Data;
	    break;
	case MUIA_Application_Sleep:
	    if (tag->ti_Data)
		data->app_SleepCount++;
	    else
		data->app_SleepCount--;

	    if (data->app_SleepCount < 0)
		data->app_SleepCount = 0;
	    else
	    {
		/*
		 * todo SC == 0 (wakeup), SC == 1 (sleep)
		 */
	    }
	    break;
	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/*
 * OM_GET
 */
static ULONG mGet(struct IClass *cl, Object *obj, struct opGet *msg)
{
/*----------------------------------------------------------------------------*/
/* small macro to simplify return value storage */
/*----------------------------------------------------------------------------*/
#define STORE *(msg->opg_Storage)

    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
    case MUIA_Version:
	STORE = __version;
	return(TRUE);
    case MUIA_Revision:
	STORE = __revision;
	return(TRUE);
    case MUIA_Application_Author:
	STORE = (ULONG)data->app_Author;
	return(TRUE);
    case MUIA_Application_Base:
	STORE = (ULONG)data->app_Base;
	return(TRUE);
    case MUIA_Application_Copyright:
	STORE = (ULONG)data->app_Copyright;
	return(TRUE);
    case MUIA_Application_Description:
	STORE = (ULONG)data->app_Description;
	return(TRUE);
    case MUIA_Application_DoubleStart:
	return(TRUE);
    case MUIA_Application_ForceQuit:
	STORE = (ULONG)data->app_ForceQuit;
	return(TRUE);
    case MUIA_Application_HelpFile:
	STORE = (ULONG)data->app_HelpFile;
	return(TRUE);
    case MUIA_Application_Iconified:
	STORE = (ULONG)data->app_Iconified;
	return(TRUE);
    case MUIA_Application_Title:
	STORE = (ULONG)data->app_Title;
	return(TRUE);
    case MUIA_Application_Version:
	STORE = (ULONG)data->app_Version;
	return(TRUE);
    case MUIA_Application_WindowList:
	return GetAttr(MUIA_Family_List, data->app_WindowFamily, msg->opg_Storage);
    }

    /* our handler didn't understand the attribute, we simply pass
    ** it to our superclass now
    */
    return(DoSuperMethodA(cl, obj, (Msg) msg));
#undef STORE
}


/*
 * OM_ADDMEMBER
 */
static ULONG mAddMember(struct IClass *cl, Object *obj, struct opMember *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    D(bug("muimaster.library/application.c: Adding 0x%lx to window member list\n",msg->opam_Object));

    DoMethodA(data->app_WindowFamily, (Msg)msg);
    /* Application knows its GlobalInfo, so we can inform window */
    DoMethod(msg->opam_Object, MUIM_ConnectParent, obj);
    return TRUE;
}


/*
 * OM_REMMEMBER
 */
static ULONG mRemMember(struct IClass *cl, Object *obj, struct opMember *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    static ULONG disconnect = MUIM_DisconnectParent;

    D(bug("muimaster.library/application.c: Removing 0x%lx to window member list\n",msg->opam_Object));

    DoMethodA(msg->opam_Object, (Msg)&disconnect);
    DoMethodA(data->app_WindowFamily, (APTR)msg);
    return TRUE;
}


/*
 * 
 */
static ULONG mAddInputHandler(struct IClass *cl, Object *obj,
		 struct MUIP_Application_AddInputHandler *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    AddTail((struct List *)&data->app_IHList, (struct Node *)msg->ihnode);
    return TRUE;
}


/*
 *
 */
static ULONG mIconify(struct IClass *cl, Object *obj, Msg *msg)
{
    /*struct MUI_ApplicationData *data = INST_DATA(cl, obj);*/

    return TRUE;
}


/*
 * MUIM_Application_InputBuffered : process all pending events
 */
static ULONG mInputBuffered(struct IClass *cl, Object *obj,
               struct MUIP_Application_InputBuffered *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct IntuiMessage *imsg;

    /* process all pushed methods */
    application_do_pushed_method(data);

    imsg = (struct IntuiMessage *)GetMsg(data->app_GlobalInfo.mgi_UserPort);
    if (imsg != NULL)
    {
        /*
         * Let window object process message
         */
        _zune_window_message(imsg);

        ReplyMsg((struct Message *)imsg);
    }
    return TRUE;
}

/* Forward reference */
static ULONG mNewInput(struct IClass *cl, Object *obj,
          struct MUIP_Application_NewInput *msg);

/*
 * MUIM_Application_Input : application main loop
 */
static ULONG mInput(struct IClass *cl, Object *obj,
       struct MUIP_Application_Input *msg)
{
    *msg->signal = 0;
    return mNewInput(cl, obj, (APTR)msg);
}


/*
 * MUIM_Application_NewInput : application main loop
 */
static ULONG mNewInput(struct IClass *cl, Object *obj,
          struct MUIP_Application_NewInput *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct RIDNode *rid;
    ULONG          retval = 0;
    ULONG          signal;

    /* if user didn't handle ctrl-c himself, quit */
    if (*msg->signal & SIGBREAKF_CTRL_C)
	return MUIV_Application_ReturnID_Quit;

    /* process all pushed methods */
    while (application_do_pushed_method(data))
        ;

    signal = 1L << data->app_GlobalInfo.mgi_UserPort->mp_SigBit;
    if (*msg->signal & signal)
    {
        while (!IsMsgPortEmpty(data->app_GlobalInfo.mgi_UserPort))
            mInputBuffered(cl, obj, NULL);
    }

    *msg->signal = signal | SIGBREAKF_CTRL_C;

    /* set return code */
    if ((rid = (struct RIDNode *)RemHead((struct List *)&data->app_ReturnIDQueue)))
    {
	retval = rid->rid_Value;
	DeleteRIDNode(data, rid);
	return retval;
    }
    return 0;
}


/*
 * Add a method in the method FIFO. Will be executed in the next
 * event loop.
 */
static ULONG mPushMethod(struct IClass *cl, Object *obj,
	    struct MUIP_Application_PushMethod *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct MQNode *mq;
    LONG          i;

    mq = CreateMQNode(msg->count);
    if (!mq)
	return FALSE;
    mq->mq_Dest = msg->dest;

    /* fill msg */
    for (i = 0; i < msg->count; i++)
	mq->mq_Msg[i] = (ULONG)*(&msg->count + 1 + i);

    /* enqueue method */
    AddTail((struct List *)&data->app_MethodQueue, (struct Node *)mq);
    return TRUE;
}


/*
 *
 */
static ULONG mRemInputHandler(struct IClass *cl, Object *obj,
		 struct MUIP_Application_RemInputHandler *msg)
{
    /*struct MUI_ApplicationData *data = INST_DATA(cl, obj);*/

    Remove((struct Node *)msg->ihnode);
    return TRUE;
}


/*
 * MUIM_Application_ReturnID : Tell MUI to return the given id with
 * the next call to MUIM_Application_NewInput. kinda obsolete :)
 */
static ULONG mReturnID(struct IClass *cl, Object *obj,
	  struct MUIP_Application_ReturnID *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct RIDNode *rid;

/*
    if (!data->app_RIDMemChunk)
    {
	data->app_RIDMemChunk =
	    g_mem_chunk_create(struct RIDNode, 10, G_ALLOC_AND_FREE);
    }
*/
    rid = CreateRIDNode(data, msg->retid);
    if (!rid)
	return FALSE;
    AddTail((struct List *)&data->app_ReturnIDQueue, (struct Node *)rid);
    return TRUE;
}


/*
 * MUIM_FindUData : tests if the MUIA_UserData of the object
 * contains the given <udata> and returns the object pointer in this case.
 */
static ULONG mFindUData(struct IClass *cl, Object *obj, struct MUIP_FindUData *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
	return (ULONG)obj;
    return DoMethodA(data->app_WindowFamily, (Msg)msg);
}


/*
 * MUIM_GetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and gets <attr> to <storage> for itself
 * in this case.
 */
static ULONG mGetUData(struct IClass *cl, Object *obj, struct MUIP_GetUData *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
    {
	get(obj, msg->attr, msg->storage);
	return TRUE;
    }
    return DoMethodA(data->app_WindowFamily, (Msg)msg);
}


/*
 * MUIM_SetUData : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and sets <attr> to <val> for itself in this case.
 */
static ULONG mSetUData(struct IClass *cl, Object *obj, struct MUIP_SetUData *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
	set(obj, msg->attr, msg->val);

    DoMethodA(data->app_WindowFamily, (Msg)msg);
    return TRUE;
}


/*
 * MUIM_SetUDataOnce : This method tests if the MUIA_UserData of the object
 * contains the given <udata> and sets <attr> to <val> for itself in this case.
 */
static ULONG mSetUDataOnce(struct IClass *cl, Object *obj, struct MUIP_SetUDataOnce *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
    {
	set(obj, msg->attr, msg->val);
	return TRUE;
    }
    return DoMethodA(data->app_WindowFamily, (Msg)msg);
}


/*
 * The class dispatcher
 */
#ifndef _AROS
__asm IPTR Application_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR, Application_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
/*----------------------------------------------------------------------------*/
/* watch out for methods we do understand                                     */
/*----------------------------------------------------------------------------*/
    switch (msg->MethodID)
    {
	/* Whenever an object shall be created using NewObject(), it will be
	** sent a OM_NEW method.
	*/
    case OM_NEW:
	return(Application_New(cl, obj, (struct opSet *) msg));
    case OM_DISPOSE:
	return(Application_Dispose(cl, obj, msg));
    case OM_SET:
	return(mSet(cl, obj, (struct opSet *)msg));
    case OM_GET:
	return(mGet(cl, obj, (struct opGet *)msg));
    case OM_ADDMEMBER:
	return(mAddMember(cl, obj, (APTR)msg));
    case OM_REMMEMBER:
	return(mRemMember(cl, obj, (APTR)msg));
    case MUIM_Application_AddInputHandler :
	return(mAddInputHandler(cl, obj, (APTR)msg));
    case MUIM_Application_Iconify :
	return(mIconify(cl, obj, (APTR)msg));
    case MUIM_Application_Input :
	return(mInput(cl, obj, (APTR)msg));
    case MUIM_Application_InputBuffered :
	return(mInputBuffered(cl, obj, (APTR)msg));
    case MUIM_Application_NewInput :
	return(mNewInput(cl, obj, (APTR)msg));
    case MUIM_Application_PushMethod :
	return(mPushMethod(cl, obj, (APTR)msg));
    case MUIM_Application_RemInputHandler :
	return(mRemInputHandler(cl, obj, (APTR)msg));
    case MUIM_Application_ReturnID :
	return(mReturnID(cl, obj, (APTR)msg));

    case MUIM_FindUData :
	return(mFindUData(cl, obj, (APTR)msg));
    case MUIM_GetUData :
	return(mGetUData(cl, obj, (APTR)msg));
    case MUIM_SetUData :
	return(mSetUData(cl, obj, (APTR)msg));
    case MUIM_SetUDataOnce :
	return(mSetUDataOnce(cl, obj, (APTR)msg));
    }

    return(DoSuperMethodA(cl, obj, msg));
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Application_desc = {
    MUIC_Application,
    MUIC_Notify,
    sizeof(struct MUI_ApplicationData),
    Application_Dispatcher
};
