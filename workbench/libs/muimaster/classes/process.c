/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "process_private.h"

#define MYDEBUG 1
#include "debug.h"

#define DELAYTICKS (10)


extern struct Library *MUIMasterBase;


static void my_process(void)
{
    // invokes MUIM_Process_Process  for the the class/object specified by
    // MUIA_Process_SourceClass/Object (source class may be NULL)

    D(bug("[Process.mui] my_process called\n"));

    struct Task *thistask = FindTask(NULL);
    struct Process_DATA *data = thistask->tc_UserData;

    if (data->sourceclass)
    {   
        CoerceMethod(data->sourceclass, data->sourceobject, MUIM_Process_Process, &data->kill, data->self);
    }
    else
    {
        DoMethod(data->sourceobject, MUIM_Process_Process, &data->kill, data->self);
    }

    data->task = NULL; // show MUIM_Process_Kill that we're done

    D(bug("[Process.mui] my_process terminated\n"));
}


IPTR Process__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Process_DATA *data;
    const struct TagItem *tags;
    struct TagItem *tag;
    struct Task *thistask;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    data->self = obj;

    // defaults
    data->autolaunch = TRUE;
    data->stacksize = 40000;
    thistask = FindTask(NULL);
    data->priority = thistask->tc_Node.ln_Pri;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Process_AutoLaunch:
                data->autolaunch = tag->ti_Data;
                break;

            case MUIA_Process_Name:
                data->name = (STRPTR)tag->ti_Data;
                break;

            case MUIA_Process_Priority:
                data->priority = tag->ti_Data;
                break;

            case MUIA_Process_SourceClass:
                data->sourceclass = (struct IClass *)tag->ti_Data;
                break;

            case MUIA_Process_SourceObject:
                data->sourceobject = (Object *)tag->ti_Data;
                break;

            case MUIA_Process_StackSize:
                data->stacksize = tag->ti_Data;
                break;
        }
    }

    D(bug("muimaster.library/process.c: Process Object created at 0x%lx\n",obj));

    if (data->autolaunch)
    {
        DoMethod(obj, MUIM_Process_Launch);
    }

    return (IPTR)obj;
}


IPTR Process__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct Process_DATA *data = INST_DATA(cl, obj);

    STORE = 0;

    switch(msg->opg_AttrID)
    {
        case MUIA_Process_Task:
            STORE = (IPTR)data->task;
            return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}


IPTR Process__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    D(bug("[Process.mui/OM_DISPOSE]\n"));

    // struct Process_DATA *data = INST_DATA(cl, obj);

    DoMethod(obj, MUIM_Process_Kill, 0);

    return DoSuperMethodA(cl, obj, msg);
}


IPTR Process__MUIM_Process_Kill(struct IClass *cl, Object *obj, struct MUIP_Process_Kill *msg)
{
    D(bug("[MUIM_Process_Kill] maxdelay %d\n", msg->maxdelay));

    struct Process_DATA *data = INST_DATA(cl, obj);
    ULONG delay = 0;
    BOOL retval = TRUE;

    // send SIGBREAKF_CTRL_C
    // wait until  it has terminated

    // Stops process' loop (MUIM_Process_Process). If the loop
    // is not running does nothing.

    // msg->maxdelay == 0 means "wait forever"

    data->kill = 1; // stops the loop in Class4.c demo

    // the spawned task sets data->task to NULL on exit
    while (data->task != NULL)
    {
        Signal((struct Task *)data->task, SIGBREAKF_CTRL_C);
        Delay(DELAYTICKS);
        delay += DELAYTICKS;
        D(bug("[MUIM_Process_Kill] delay %d maxdelay %d\n", delay, msg->maxdelay));
        if ((msg->maxdelay != 0) && (delay > msg->maxdelay))
        {
            D(bug("[MUIM_Process_Kill] timeout\n"));
            retval = FALSE;
            break;
        }
    }

    D(bug("[MUIM_Process_Kill] retval %d\n", retval));

    return retval;
}


IPTR Process__MUIM_Process_Launch(struct IClass *cl, Object *obj, struct MUIP_Process_Launch *msg)
{
    D(bug("[MUIM_Process_Launch]\n"));

    struct Process_DATA *data = INST_DATA(cl, obj);

    // Starts process' loop (MUIM_Process_Process). If the loop
    // is already running does nothing.

    if (data->task == NULL)
    {
        struct TagItem tags[] =
        {
            {NP_Entry,      (IPTR)my_process},
            {NP_StackSize,  data->stacksize},
            {data->name ? NP_Name : TAG_IGNORE,
                            (IPTR)data->name},
            {NP_Priority,   data->priority},
            {NP_UserData,   (IPTR)data},
            {TAG_DONE}
        };

        data->task = CreateNewProc(tags);
    }

    return (IPTR)TRUE;
}


IPTR Process__MUIM_Process_Process(struct IClass *cl, Object *obj, struct MUIP_Process_Process *msg)
{
    D(bug("[MUIM_Process_Process] kill %p proc %p\n", msg->kill, msg->proc));

    // struct Process_DATA *data = INST_DATA(cl, obj);

    // Main process method. Terminating condition is passed in message struct.
    // Proper implementation should wait for a signal to not use 100% cpu.
    // This is some kind of a virtual function. Sub-class implementators
    // must overwrite it.

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


IPTR Process__MUIM_Process_Signal(struct IClass *cl, Object *obj, struct MUIP_Process_Signal *msg)
{
    D(bug("[MUIM_Process_Signal] sigs %u\n", msg->sigs));

    struct Process_DATA *data = INST_DATA(cl, obj);

    // MUIM_Process_Signal just sends an arbitrary signal to the spawned process.

    if (data->task)
    {
        Signal((struct Task *)data->task, msg->sigs);
    }

    return 0;
}


#if ZUNE_BUILTIN_PROCESS
BOOPSI_DISPATCHER(IPTR, Process_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:                    return Process__OM_NEW(cl, obj, (struct opSet *)msg);
        case OM_GET:                    return Process__OM_SET(cl, obj, (struct opSet *)msg);
        case OM_DISPOSE:                return Process__OM_DISPOSE(cl, obj, msg);
        case MUIM_Process_Kill:         return Process__MUIM_Process_Kill(cl, obj, (struct MUIP_Process_Kill *)msg);
        case MUIM_Process_Launch:       return Process__MUIM_Process_Launch(cl, obj, (struct MUIP_Process_Launch *)msg);
        case MUIM_Process_Process:      return Process__MUIM_Process_Process(cl, obj, (struct MUIP_Process_Process *)msg);
        case MUIM_Process_Signal:       return Process__MUIM_Process_Signal(cl, obj, (struct MUIP_Process_Signal *)msg);
        default:                        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Process_desc =
{
    MUIC_Process,
    MUIC_Semaphore,
    sizeof(struct Process_DATA),
    (void*)Process_Dispatcher
};
#endif /* ZUNE_BUILTIN_PROCESS */
