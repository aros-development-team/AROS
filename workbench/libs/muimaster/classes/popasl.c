/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <string.h>
#include <stdio.h>

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
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "popasl_private.h"

/****** Popasl.mui/MUIC_Popasl ************
*
*   NAME
*       MUIC_Popasl
*
*   SUPERCLASS
*       Popstring.mui
*
*   DESCRIPTION
*       Run an ASL requester in a separate process and transfer accepted results
*       to the string. Type defaults ASL_FileRequest. AllocAslRequest receives
*       the constructor tag list; failure returns NULL. If Popstring
*       construction fails afterward, this path does not free the allocated
*       requester. On success the class owns the requester and installs internal
*       opening/closing hooks, forcing Toggle FALSE.
*       Open creates a process and waits synchronously for its startup-message
*       reply, with no timeout. The process then runs AslRequest and queues
*       Close(TRUE/FALSE) on the application. The close hook runs in a further
*       queued CallHook. Keep the application, object, string, requester and
*       referenced tag data alive through all these stages.
*
*   METHODS
*       Without StartHook, file opening derives InitialFile and InitialDrawer
*       from String_Contents; font opening derives a .font name and size from a
*       name/size string. Invalid numeric font size is not checked. Accepted
*       file results combine drawer/file; accepted font results remove the first
*       .font substring and append size. The default close hook sets
*       String_Contents and then String_Acknowledge from the retrieved stored
*       contents. Allocation failure can leave the string unchanged without a
*       separate error. Other requester types have no built-in string
*       conversion.
*       Cleanup requests AbortAslRequest if active and then delegates; it does
*       not wait for the process or queued close. Dispose frees the requester
*       without joining the worker. An explicit inherited Close merely queues
*       bookkeeping/result processing; it does not abort or wait for ASL. Do not
*       use it to force-close a running requester.
*
*   NOTES
*       Do not dispose while the worker or queued callbacks can still access
*       this object/requester. Active is based on a stored process pointer, not
*       a process-liveness query or join. That pointer is cleared only in the
*       deferred close hook. Queue failures are not checked and can leave state
*       stuck. The initial filename can point into the String's current buffer,
*       so keep it stable while the worker may read startup tags.
*       The open path leaks its temporary name/drawer buffer if message-port
*       allocation, startup allocation or process creation fails after buffer
*       creation. Startup reply handling frees the startup message without
*       explicitly dequeuing the reply. These limitations are documented, not
*       corrected here. Hook tags are borrowed, and asynchronous execution
*       provides no automatic ownership protection.
*
*/
/****** Popasl.mui/MUIA_Popasl_Active ************
*
*   NAME
*       MUIA_Popasl_Active
*
*   FUNCTION
*       Report whether the stored requester-process pointer is non-NULL.
*
*   INPUTS
*       Get only, BOOL. Initially FALSE; true after successful process creation
*       until the deferred close hook clears the pointer.
*
*   NOTES
*       No TRUE notification is explicitly emitted when starting. Close
*       processing emits Active FALSE using nfset, which disables Group
*       forwarding, not notifications. The getter can remain TRUE after the
*       worker has finished while close work is pending. It is not a process
*       join or proof that no instructions remain in the worker.
*
*/
/****** Popasl.mui/MUIA_Popasl_Type ************
*
*   NAME
*       MUIA_Popasl_Type
*
*   FUNCTION
*       Choose the ASL requester type at construction.
*
*   INPUTS
*       Initialize only in this implementation, ULONG; default ASL_FileRequest.
*       No local getter despite the header annotation.
*
*   NOTES
*       Passed to AllocAslRequest together with constructor tags. Built-in
*       string conversion exists only for file and font requesters. Changing
*       Type after construction is not implemented. Use suitable
*       StartHook/StopHook processing for other accepted ASL types.
*
*/
/****** Popasl.mui/MUIA_Popasl_StartHook ************
*
*   NAME
*       MUIA_Popasl_StartHook
*
*   FUNCTION
*       Customize per-open requester tags or reject an opening attempt.
*
*   INPUTS
*       Initialize and set, borrowed pointer to struct Hook; default NULL. No
*       local getter despite the header annotation.
*
*   NOTES
*       Called synchronously as CallHookPkt(hook, popaslObject, tagList). Return
*       nonzero to continue or zero to abort. The instance has 20 TagItems; the
*       first five are screen, private IDCMP, initial left/top/width, followed
*       by TAG_DONE. Keep edits and termination within this capacity. Providing
*       this hook replaces default file/font initialization. Referenced data
*       must survive asynchronous requester use; do not leave worker tags
*       pointing into callback-local storage.
*
*/
/****** Popasl.mui/MUIA_Popasl_StopHook ************
*
*   NAME
*       MUIA_Popasl_StopHook
*
*   FUNCTION
*       Handle an accepted requester result.
*
*   INPUTS
*       Initialize and set, borrowed pointer to struct Hook; default NULL. No
*       local getter despite the header annotation.
*
*   NOTES
*       Called as CallHookPkt(hook, popaslObject, aslRequester) only for a
*       nonzero deferred Close result. Return value is ignored. Providing the
*       hook replaces default string and Acknowledge updates; cancellation does
*       not call it. The requester is borrowed and remains class-owned. The
*       stored hook at close-processing time is used, so retain its data until
*       then.
*
*/

extern struct Library *MUIMasterBase;

struct Asl_Startup
{
    struct Message msg;
    APTR asl_req;
    struct TagItem *tags;
    Object *app;          /* the application */
    Object *pop;
    STRPTR buf;           /* asl_entry must FreeVec() this before exiting!!! */
};

/*
 * Worker waits for startup, copies borrowed request/tag/object pointers,
 * replies, runs AslRequest and queues Close with acceptance flag. Free
 * transferred temporary buffer and return zero. Requires all borrowed
 * resources live through requester and queued callbacks; queue failure is
 * ignored and no join signal is provided.
 */
SAVEDS static LONG Asl_Entry(void)
{
    struct Process *proc;
    struct Asl_Startup *msg;

    APTR asl_req;
    struct TagItem *tags;
    Object *app;
    Object *pop;
    STRPTR buf;

    proc = (struct Process *)FindTask(NULL);
    WaitPort(&proc->pr_MsgPort);        /* Wait for the startup message */
    msg = (struct Asl_Startup *)GetMsg(&proc->pr_MsgPort);

    asl_req = msg->asl_req;
    tags = msg->tags;
    app = msg->app;
    pop = msg->pop;
    buf = msg->buf;

    ReplyMsg(&msg->msg);

    if (AslRequest(asl_req, tags))
        DoMethod(app, MUIM_Application_PushMethod, (IPTR) pop, 2,
            MUIM_Popstring_Close, TRUE);
    else
        DoMethod(app, MUIM_Application_PushMethod, (IPTR) pop, 2,
            MUIM_Popstring_Close, FALSE);

    FreeVec(buf);

    return 0;
}

/*
 * Reject existing stored process, prepare 20-slot tag buffer via hook or
 * default string parsing, create process and wait for startup reply without
 * timeout. Return one on handshake or zero on failure. Temporary buffer
 * transfers to worker only on success; later allocation failures leak it.
 * Caller retains objects and tag targets.
 */
static ULONG Popasl_Open_Function(struct Hook *hook, Object *obj,
    void **msg)
{
    char *buf = NULL;
    struct Popasl_DATA *data = (struct Popasl_DATA *)hook->h_Data;
    struct Asl_Startup *startup;
    Object *string = (Object *) msg[0];
    struct MsgPort *msg_port;

    if (data->asl_proc)
        return 0;

    data->tag_list[0].ti_Tag = ASLFR_Screen;
    data->tag_list[0].ti_Data = (IPTR) _screen(obj);
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
        if (!(CallHookPkt(data->start_hook, obj, data->tag_list)))
            return 0;
    }
    else
    {
        if (data->type == ASL_FileRequest)
        {
            char *str = NULL;
            char *path_end;

            get(string, MUIA_String_Contents, &str);

            path_end = PathPart(str);
            buf = (char *)AllocVec(path_end - str + 10, MEMF_PUBLIC);
            if (!buf)
                return 0;

            strncpy(buf, str, path_end - str);
            buf[path_end - str] = 0;

            data->tag_list[5].ti_Tag = ASLFR_InitialFile;
            data->tag_list[5].ti_Data = (IPTR) FilePart(str);
            data->tag_list[6].ti_Tag = ASLFR_InitialDrawer;
            data->tag_list[6].ti_Data = (IPTR) buf;
            data->tag_list[7].ti_Tag = TAG_DONE;
            data->tag_list[7].ti_Data = 0;
        }

        if (data->type == ASL_FontRequest)
        {
            char *str = NULL;
            char *name_end;
            LONG size;

            get(string, MUIA_String_Contents, &str);

            if (str)
            {
                name_end = PathPart(str);
                buf = (char *)AllocVec(name_end - str + 10, MEMF_PUBLIC);
                if (!buf)
                    return 0;

                strncpy(buf, str, name_end - str);
                strcpy(buf + (name_end - str), ".font");
                StrToLong(FilePart(str), &size);

                data->tag_list[5].ti_Tag = ASLFO_InitialName;
                data->tag_list[5].ti_Data = (IPTR) buf;
                data->tag_list[6].ti_Tag = ASLFO_InitialSize;
                data->tag_list[6].ti_Data = size;
                data->tag_list[7].ti_Tag = TAG_DONE;
                data->tag_list[7].ti_Data = 0;
            }
        }
    }

    if (!(msg_port = CreateMsgPort()))
        return 0;
    if (!(startup =
            (struct Asl_Startup *)AllocVec(sizeof(struct Asl_Startup),
                MEMF_PUBLIC)))
    {
        DeleteMsgPort(msg_port);
        return 0;
    }

    {
        const struct TagItem processTags[2] = {
            {NP_Entry, (IPTR) Asl_Entry},
            {TAG_DONE, 0}
        };

        if (!(data->asl_proc = CreateNewProc(processTags)))
        {
            FreeVec(startup);
            DeleteMsgPort(msg_port);
            return 0;
        }
    }

    startup->msg.mn_ReplyPort = msg_port;
    startup->msg.mn_Length = sizeof(struct Asl_Startup);
    startup->tags = data->tag_list;
    startup->app = _app(obj);
    startup->asl_req = data->asl_req;
    startup->pop = obj;
    startup->buf = buf;
    PutMsg(&data->asl_proc->pr_MsgPort, &startup->msg);
    WaitPort(msg_port);

    FreeVec(startup);
    DeleteMsgPort(msg_port);

    return 1;
}


/*
 * On accepted result invoke StopHook or default file/font string conversion
 * and Acknowledge; then clear process pointer and notify Active FALSE via
 * nfset. Return zero. Requires completed requester and live objects; this
 * is not abort/join. Output allocation failures silently skip updates.
 */
static ULONG Popasl_Close_Function(struct Hook *hook, Object *obj,
    void **msg)
{
    struct Popasl_DATA *data = (struct Popasl_DATA *)hook->h_Data;
    Object *string = (Object *) msg[0];
    IPTR suc = (IPTR) msg[1];

    if (suc)
    {
        if (data->stop_hook)
        {
            CallHookPkt(data->stop_hook, obj, data->asl_req);
        }
        else
        {
            char *buf = NULL;

            if (data->type == ASL_FileRequest)
            {
                struct FileRequester *file_req =
                    (struct FileRequester *)data->asl_req;
                char *file =
                    (char *)file_req->fr_File ? (char *)file_req->
                    fr_File : (char *)"";
                char *drawer =
                    (char *)file_req->fr_Drawer ? (char *)file_req->
                    fr_Drawer : (char *)"";
                int len = strlen(file) + strlen(drawer) + 10;

                buf = (char *)AllocVec(len, MEMF_CLEAR);
                if (buf)
                {
                    strcpy(buf, drawer);
                    AddPart(buf, file, len);
                }
            }
            else if (data->type == ASL_FontRequest)
            {
                struct FontRequester *font_req =
                    (struct FontRequester *)data->asl_req;
                char *name = font_req->fo_Attr.ta_Name;

                if (name)
                {
                    LONG size = font_req->fo_Attr.ta_YSize;
                    int len = strlen(name) + 20;

                    buf = AllocVec(len, MEMF_CLEAR);
                    if (buf)
                    {
                        char num_buf[20];
                        char *font_ext;

                        strcpy(buf, name);

                        /* Remove the .font extension */
                        if ((font_ext = strstr(buf, ".font")))
                            *font_ext = 0;

                        snprintf(num_buf, 20, "%ld", (long)size);
                        AddPart(buf, num_buf, len);
                    }
                }
            }

            if (buf)
            {
                IPTR contents = 0;

                set(string, MUIA_String_Contents, (IPTR) buf);
                get(string, MUIA_String_Contents, &contents);

                /* trigger string notification */
                set(string, MUIA_String_Acknowledge, contents);

                FreeVec(buf);
            }
        }
    }

    data->asl_proc = NULL;
    nfset(obj, MUIA_Popasl_Active, FALSE);

    return 0;
}

/*
 * Allocate ASL request from Type/constructor tags, construct Popstring and
 * install embedded hooks with Toggle FALSE. Return object or zero on
 * failure. Superclass failure leaks already allocated requester. Borrow
 * hooks and retain requester until disposal; no getter for Type/hooks.
 */
IPTR Popasl__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Popasl_DATA *data;
    struct TagItem *tags;
    struct TagItem *tag;
    ULONG asl_type =
        GetTagData(MUIA_Popasl_Type, ASL_FileRequest, msg->ops_AttrList);
    APTR asl_req;

    if (!(asl_req = AllocAslRequest(asl_type, msg->ops_AttrList)))
        return 0;

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
        MUIA_Popstring_Toggle, FALSE, TAG_MORE, (IPTR) msg->ops_AttrList);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Popasl_StartHook:
            data->start_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Popasl_StopHook:
            data->stop_hook = (struct Hook *)tag->ti_Data;
            break;
        }
    }

    data->open_hook.h_Entry = HookEntry;
    data->open_hook.h_SubEntry = (HOOKFUNC) Popasl_Open_Function;
    data->open_hook.h_Data = data;
    data->close_hook.h_Entry = HookEntry;
    data->close_hook.h_SubEntry = (HOOKFUNC) Popasl_Close_Function;
    data->close_hook.h_Data = data;
    data->asl_req = asl_req;
    data->type = asl_type;

    SetAttrs(obj,
        MUIA_Popstring_OpenHook, (IPTR) & data->open_hook,
        MUIA_Popstring_CloseHook, (IPTR) & data->close_hook,
        MUIA_Popstring_Toggle, FALSE, TAG_DONE);

    return (IPTR) obj;
}

/*
 * Free class-owned ASL requester and return Popstring disposal result.
 * Requires worker and all deferred callbacks finished; does not abort, join
 * or cancel queued work here. Disposing an active requester is unsafe.
 */
IPTR Popasl__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Popasl_DATA *data = INST_DATA(cl, obj);
    if (data->asl_req)
        FreeAslRequest(data->asl_req);
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

#define STORE *(msg->opg_Storage)
/*
 * Store boolean presence of process pointer for Active and return one;
 * delegate other getters. Requires valid storage. Does not query process
 * liveness, wait, or expose Type/hooks.
 */
IPTR Popasl__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Popasl_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Popasl_Active:
        STORE = ! !data->asl_proc;
        return 1;
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
}
#undef STORE

/*
 * Replace borrowed StartHook/StopHook pointers and return Popstring setter
 * result. No Type or Active state setter. New StopHook is used by later
 * close processing; keep asynchronous resources valid.
 */
IPTR Popasl__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Popasl_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Popasl_StartHook:
            data->start_hook = (struct Hook *)tag->ti_Data;
            break;
        case MUIA_Popasl_StopHook:
            data->stop_hook = (struct Hook *)tag->ti_Data;
            break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * If process pointer is non-NULL request AbortAslRequest, then return
 * Popstring cleanup result. No wait for worker exit or deferred Close. Keep
 * requester, objects and hook data alive until asynchronous work finishes.
 */
IPTR Popasl__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Popasl_DATA *data = INST_DATA(cl, obj);
    if (data->asl_proc)
        AbortAslRequest(data->asl_req);
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

#if ZUNE_BUILTIN_POPASL
/*
 * Dispatch local overrides and return their results; delegate other
 * messages to superclass. Requires ABI-correct message and live object
 * except construction. Deferred callbacks and resource ownership follow the
 * class lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Popasl_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Popasl__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Popasl__OM_DISPOSE(cl, obj, msg);
    case OM_GET:
        return Popasl__OM_GET(cl, obj, (struct opGet *)msg);
    case OM_SET:
        return Popasl__OM_SET(cl, obj, (struct opSet *)msg);
    case MUIM_Cleanup:
        return Popasl__MUIM_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Popasl_desc =
{
    MUIC_Popasl,
    MUIC_Popstring,
    sizeof(struct Popasl_DATA),
    (void *) Popasl_Dispatcher
};
#endif /* ZUNE_BUILTIN_POPASL */
