/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <dos/dostags.h>
#include <libraries/asl.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/asl.h>
#include <proto/dos.h>

#include <string.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "compiler.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_PopaslData
{
    int type;
    APTR asl_req;

    struct Hook open_hook;
    struct Hook close_hook;
    struct Hook *start_hook, *stop_hook;

    struct TagItem tag_list[20]; /* According to docs we need at least 16 */

    struct Process *asl_proc;
};

struct Asl_Startup
{
    struct Message msg;
    APTR asl_req;
    struct TagItem *tags;
    Object *app; /* the application */
    Object *pop;
};

#ifndef _AROS
__saveds static LONG Asl_Entry(void)
#else
static LONG Asl_Entry(void)
#endif
{
    struct Process *proc;
    struct Asl_Startup *msg;

    APTR asl_req;
    struct TagItem *tags;
    Object *app;
    Object *pop;

    proc = (struct Process *) FindTask(NULL);
    WaitPort(&proc->pr_MsgPort); /* Wait for the startup message */
    msg = (struct Asl_Startup*)GetMsg(&proc->pr_MsgPort);

    asl_req = msg->asl_req;
    tags = msg->tags;
    app = msg->app;
    pop = msg->pop;

    ReplyMsg(&msg->msg);

    if (AslRequest(asl_req, tags))
	DoMethod(app,MUIM_Application_PushMethod, pop, 2, MUIM_Popstring_Close, TRUE);
    else DoMethod(app,MUIM_Application_PushMethod, pop, 2, MUIM_Popstring_Close, FALSE);

    return 0;
}

#ifndef _AROS
static __asm ULONG Popasl_Open_Function(register __a0 struct Hook *hook, register __a2 Object *obj, register __a1 void **msg)
#else
AROS_UFH3(ULONG,Popasl_Open_Function,
	AROS_UFHA(struct Hook *, hook,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(void **, msg,  A1))
#endif
{
    char *buf = NULL;
    struct MUI_PopaslData *data = (struct MUI_PopaslData *)hook->h_Data;
    struct Asl_Startup *startup;
    Object *string = (Object*)msg[0];
    struct MsgPort *msg_port;

    if (data->asl_proc) return 0;

    data->tag_list[0].ti_Tag = ASLFR_Screen;
    data->tag_list[0].ti_Data = (IPTR)_screen(obj);
    data->tag_list[1].ti_Tag = ASLFR_PrivateIDCMP;
    data->tag_list[1].ti_Data = 1;
    data->tag_list[2].ti_Tag = ASLFR_InitialLeftEdge;
    data->tag_list[2].ti_Data = _left(obj);
    data->tag_list[3].ti_Tag = ASLFR_InitialTopEdge;
    data->tag_list[3].ti_Data = _top(obj);
    data->tag_list[4].ti_Tag = ASLFR_InitialWidth;
    data->tag_list[4].ti_Data = _width(obj);
    data->tag_list[5].ti_Tag = TAG_DONE;
    data->tag_list[5].ti_Data = 0;

    if (data->start_hook)
    {
	if (!(CallHookPkt(data->start_hook,obj,data->tag_list)))
	    return 0;
    } else
    {
	if (data->type == ASL_FileRequest)
	{
	    char *str, *path_end;
	    get(string,MUIA_String_Contents,&str);

	    path_end = PathPart(str);
	    buf = (char*)AllocVec(path_end - str + 2, MEMF_PUBLIC);
	    if (!buf) return 0;

	    strncpy(buf,str,path_end - str);
	    buf[path_end - str] = 0;

	    data->tag_list[5].ti_Tag = ASLFR_InitialFile;
	    data->tag_list[5].ti_Data = (IPTR)FilePart(str);
	    data->tag_list[6].ti_Tag = ASLFR_InitialDrawer;
	    data->tag_list[6].ti_Data = (IPTR)buf;
	    data->tag_list[7].ti_Tag = TAG_DONE;
	    data->tag_list[7].ti_Data = 0;
	}
    }

    if (!(msg_port = CreateMsgPort())) return 0;
    if (!(startup = (struct Asl_Startup*)AllocVec(sizeof(struct Asl_Startup),MEMF_PUBLIC)))
    {
    	DeleteMsgPort(msg_port);
    	return 0;
    }
    if (!(data->asl_proc = CreateNewProcTags(NP_Entry, Asl_Entry, TAG_DONE)))
    {
    	FreeVec(startup);
    	DeleteMsgPort(msg_port);
    	return 0;
    }

    startup->msg.mn_ReplyPort = msg_port;
    startup->msg.mn_Length = sizeof(struct Asl_Startup);
    startup->tags = data->tag_list;
    startup->app = _app(obj);
    startup->asl_req = data->asl_req;
    startup->pop = obj;
    PutMsg(&data->asl_proc->pr_MsgPort,&startup->msg);
    WaitPort(msg_port);

    FreeVec(startup);
    DeleteMsgPort(msg_port);
    if (buf) FreeVec(buf);

    return 1;
}


#ifndef _AROS
static __asm ULONG Popasl_Close_Function(register __a0 struct Hook *hook, register __a2 Object *obj, register __a1 void **msg)
#else
AROS_UFH3(ULONG,Popasl_Close_Function,
	AROS_UFHA(struct Hook *, hook,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(void **, msg,  A1))
#endif
{
    struct MUI_PopaslData *data= (struct MUI_PopaslData *)hook->h_Data;
    Object *string = (Object*)msg[0];
    LONG suc = (LONG)msg[1];

    if (suc)
    {
	if (data->stop_hook)
	{
	    CallHookPkt(data->stop_hook,obj,data->asl_req);
	} else
	{
	    if (data->type == ASL_FileRequest)
	    {
		struct FileRequester *file_req = (struct FileRequester*)data->asl_req;
		char *file = (char*)file_req->fr_File?(char*)file_req->fr_File:(char*)"";
		char *drawer = (char*)file_req->fr_Drawer?(char*)file_req->fr_Drawer:(char*)"";
		int len = strlen(file)+strlen(drawer)+10;
		char *buf = (char*)AllocVec(len,MEMF_CLEAR);
		if (buf)
		{
		    strcpy(buf,drawer);
		    AddPart(buf,file,len);
		    set(string,MUIA_String_Contents,buf);
		    FreeVec(buf);
		}
	    }
	}
    }

    data->asl_proc = NULL;

    return 0;
}



/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Popasl_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PopaslData   *data;
    struct TagItem  	    *tag, *tags;
    ULONG asl_type = GetTagData(MUIA_Popasl_Type,ASL_FileRequest,msg->ops_AttrList);
    APTR asl_req;

    if (!(asl_req = AllocAslRequest(asl_type,msg->ops_AttrList))) return 0;
 
    obj = (Object *)DoSuperNew(cl, obj,
		MUIA_Popstring_Toggle, FALSE,
		TAG_MORE, msg->ops_AttrList);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Popasl_StartHook:
		    data->start_hook = (struct Hook*)tag->ti_Data;
		    break;

	    case    MUIA_Popasl_StopHook:
		    data->stop_hook = (struct Hook*)tag->ti_Data;
		    break;
    	}
    }

    data->open_hook.h_Entry = (HOOKFUNC)Popasl_Open_Function;
    data->open_hook.h_Data = data;
    data->close_hook.h_Entry = (HOOKFUNC)Popasl_Close_Function;
    data->close_hook.h_Data = data;
    data->asl_req = asl_req;
    
    SetAttrs(obj,
	MUIA_Popstring_OpenHook, &data->open_hook,
	MUIA_Popstring_CloseHook, &data->close_hook,
	MUIA_Popstring_Toggle, FALSE,
	TAG_DONE);

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Popasl_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_PopaslData   *data = INST_DATA(cl, obj);
    if (data->asl_req) FreeAslRequest(data->asl_req);
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG Popasl_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_PopaslData *data = INST_DATA(cl, obj);

#define STORE *(msg->opg_Storage)
    switch(msg->opg_AttrID)
    {
    	case MUIA_Popasl_Active: STORE = !!data->asl_proc; return 1;
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static ULONG Popasl_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_PopaslData *data = INST_DATA(cl, obj);
    if (data->asl_proc) AbortAslRequest(data->asl_req);
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

#ifndef _AROS
__asm IPTR Popasl_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Popasl_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Popasl_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Popasl_Dispose(cl, obj, msg);
	case OM_GET: return Popasl_Get(cl, obj, (struct opGet *)msg);
	case MUIM_Cleanup: return Popasl_Cleanup(cl, obj, (struct MUIP_Cleanup*)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Popasl_desc = { 
    MUIC_Popasl,
    MUIC_Popstring, 
    sizeof(struct MUI_PopaslData), 
    (void*)Popasl_Dispatcher 
};

