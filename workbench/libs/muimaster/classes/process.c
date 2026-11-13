/*
    Copyright (C) 2012-2026, The AROS Development Team. All rights reserved.
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

//#define MYDEBUG 1
#include "debug.h"

#define DELAYTICKS (10)


/****** Process.mui/MUIC_Process ************
*
*   NAME
*       MUIC_Process
*
*   SUPERCLASS
*       Semaphore.mui
*
*   DESCRIPTION
*       Start a DOS process that invokes MUIM_Process_Process on a supplied
*       source object. With SourceClass set, the worker uses CoerceMethod
*       through that class; otherwise it uses DoMethod on SourceObject. Supply a
*       valid source object implementing the worker method before launch.
*       Neither source object nor source class is owned, copied or kept alive by
*       a reference.
*       The superclass is Semaphore, which derives from rootclass, not Notify.
*       The inherited semaphore methods are available, but Process does not
*       acquire that semaphore around its own state. This class does not
*       implement Notify-style attribute notifications, automatic UI marshalling
*       or thread-safe object access.
*       Construction defaults AutoLaunch to TRUE, stack size to 40000 bytes and
*       priority to the creating task's priority. With automatic launch, the
*       worker can execute before construction returns. Constructor success does
*       not prove successful process creation. Use AutoLaunch FALSE when the
*       source needs additional initialization before starting.
*       Disposal calls Kill with maxdelay zero and waits without a timeout
*       before delegating disposal. The source implementation must cooperate
*       with cancellation and return. Do not dispose this object or call its
*       waiting Kill from its own worker, or while holding resources that worker
*       needs to finish. Keep the source, class and process object alive until
*       worker use has ended.
*
*   BUGS
*       The worker clears the task field after its source method returns, before
*       the DOS process has completely exited. NULL is therefore not a native
*       process-termination acknowledgement. The class supplies no synchronized
*       startup/exit handshake or task-lifetime reference.
*       Launch assigns CreateNewProc's result to the task field and only then
*       clears kill. A worker can run before this assignment and, if it returns
*       quickly, clear the field before Launch overwrites it with the returned
*       process pointer. On relaunch it can also observe the previous kill value
*       before Launch resets it. A non-NULL field can consequently be stale;
*       Task, Signal and Kill are not race-free lifetime operations. This
*       implementation does not provide a general safe join guarantee.
*
*/
/****** Process.mui/MUIA_Process_AutoLaunch ************
*
*   NAME
*       MUIA_Process_AutoLaunch
*
*   FUNCTION
*       Choose whether construction calls Launch.
*
*   INPUTS
*       Initialize only. BOOL stored from the tag value. Default TRUE. No local
*       setter or getter.
*
*   NOTES
*       Any nonzero value launches. Launch status is ignored, and the callback
*       may run before construction returns. Set FALSE to defer launch until
*       required initialization is complete.
*
*/
/****** Process.mui/MUIA_Process_Name ************
*
*   NAME
*       MUIA_Process_Name
*
*   FUNCTION
*       Supply the name used for each CreateNewProc call.
*
*   INPUTS
*       Initialize only. Borrowed terminated STRPTR, despite the header ULONG
*       annotation. Default NULL. No local setter or getter.
*
*   NOTES
*       The wrapper retains the pointer without copying or freeing it. Keep it
*       valid for future launches. DOS copies the name for a successfully
*       created process; NULL omits NP_Name and uses the DOS default.
*
*/
/****** Process.mui/MUIA_Process_Priority ************
*
*   NAME
*       MUIA_Process_Priority
*
*   FUNCTION
*       Supply NP_Priority when launching the worker.
*
*   INPUTS
*       Initialize only. LONG stored value, despite the header ULONG annotation.
*       Default creating task priority. No local setter or getter.
*
*   NOTES
*       Process performs no range validation or adjustment of an already running
*       worker. A higher-priority worker may run before Launch returns; the
*       startup limitations still apply.
*
*/
/****** Process.mui/MUIA_Process_SourceClass ************
*
*   NAME
*       MUIA_Process_SourceClass
*
*   FUNCTION
*       Choose explicit class dispatch for the worker callback.
*
*   INPUTS
*       Initialize only. Borrowed struct IClass pointer, despite the header
*       ULONG annotation. Default NULL. No local setter or getter.
*
*   NOTES
*       Non-NULL selects CoerceMethod with SourceObject; NULL selects normal
*       DoMethod dispatch on SourceObject. It does not create a source object.
*       Keep the class and source object valid throughout the call.
*
*/
/****** Process.mui/MUIA_Process_SourceObject ************
*
*   NAME
*       MUIA_Process_SourceObject
*
*   FUNCTION
*       Choose the object receiving MUIM_Process_Process in the worker task.
*
*   INPUTS
*       Initialize only. Borrowed Object pointer, despite the header ULONG
*       annotation. Default NULL. No local setter or getter.
*
*   NOTES
*       A valid compatible source is required before launch; NULL is not
*       replaced with the Process object and is not checked before dispatch. The
*       object is not disposed by Process. Callback code runs in the DOS worker,
*       not the application task.
*
*/
/****** Process.mui/MUIA_Process_StackSize ************
*
*   NAME
*       MUIA_Process_StackSize
*
*   FUNCTION
*       Supply NP_StackSize for worker creation.
*
*   INPUTS
*       Initialize only. ULONG byte count. Default 40000. No local setter or
*       getter.
*
*   NOTES
*       The wrapper passes the value to DOS without validating sufficiency for
*       the callback. It does not resize a running worker. Creation failure is
*       not reported by Launch return status.
*
*/
/****** Process.mui/MUIA_Process_Task ************
*
*   NAME
*       MUIA_Process_Task
*
*   FUNCTION
*       Read the currently stored DOS process pointer.
*
*   INPUTS
*       Get only. Borrowed struct Process pointer, despite the header ULONG
*       annotation; NULL initially and after the worker clears the field.
*
*   RESULT
*       The local getter stores the pointer and returns TRUE. It does not retain
*       or lock the process.
*
*   NOTES
*       The pointer is only a snapshot and can become invalid immediately. NULL
*       does not prove complete DOS process exit; a fast worker can leave a
*       stale non-NULL value because of the launch race described by the class.
*       Do not dispose the process or infer safe external task access from this
*       getter. Unknown attribute queries first zero caller storage before
*       delegating to Semaphore.
*
*/
/****** Process.mui/MUIM_Process_Launch ************
*
*   NAME
*       MUIM_Process_Launch
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Process_Launch);
*
*   FUNCTION
*       Create a worker when the stored task pointer is NULL.
*
*   INPUTS
*       obj - Live Process object with valid source object/class and borrowed
*       launch data.
*
*   RESULT
*       Always TRUE, including CreateNewProc failure and an already non-NULL
*       task field.
*
*   NOTES
*       CreateNewProc receives entry, stack size, optional name, priority and
*       instance data as NP_UserData. After the call, Launch stores the returned
*       pointer and sets kill to zero. No startup acknowledgement is awaited. If
*       the pointer was already non-NULL, the method does nothing and does not
*       reset kill. See the class startup race; return TRUE is not proof that a
*       worker is running.
*
*/
/****** Process.mui/MUIM_Process_Kill ************
*
*   NAME
*       MUIM_Process_Kill
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Process_Kill, maxdelay);
*
*   FUNCTION
*       Request cooperative termination and poll the stored task pointer.
*
*   INPUTS
*       maxdelay - LONG nominal delay limit in DOS ticks, each 1/50 second. Zero
*       means wait without a limit. Use a nonnegative value; negative values are
*       not validated and interact with the unsigned delay counter.
*
*   RESULT
*       TRUE if polling finishes without taking the timeout branch; FALSE if
*       accumulated delay exceeds a nonzero maxdelay. No forced task removal
*       occurs.
*
*   NOTES
*       Set kill to one even when there is no worker. While the task field is
*       non-NULL, send SIGBREAKF_CTRL_C, Delay(10), add ten to the delay
*       counter, then compare using strictly greater than. A limit of ten
*       therefore allows two delays before timeout if the field stays non-NULL.
*       Scheduler delays may extend elapsed wall time. Timeout is checked before
*       the next task-field check, so FALSE can occur even if the worker cleared
*       the field during the final delay.
*       On timeout, kill remains one and the worker may still run. Keep all
*       referenced objects alive. This call does not pump application input,
*       protect task lifetime or forcibly interrupt arbitrary blocking work. A
*       stale task pointer can invalidate the operation; see class limitations.
*       Never call a waiting Kill from the worker it is waiting for.
*
*/
/****** Process.mui/MUIM_Process_Process ************
*
*   NAME
*       MUIM_Process_Process
*
*   SYNOPSIS
*       IPTR result = DoMethod(source, MUIM_Process_Process, kill, proc);
*
*   FUNCTION
*       Provide the source object worker entry point invoked by Process.
*
*   INPUTS
*       kill - Borrowed pointer to the Process instance ULONG cancellation flag.
*       proc - Borrowed Process MUI object, not a DOS task pointer. Both must
*       remain valid during the callback.
*
*   RESULT
*       The worker ignores the callback result. The base Process implementation
*       only delegates this message to Semaphore and implements no work loop.
*
*   NOTES
*       Implement this method in the chosen source object/class. It runs in the
*       newly created DOS process. Cooperate with the shared kill flag and
*       SIGBREAKF_CTRL_C, release acquired resources and return to allow the
*       wrapper to clear its task field. Wait for suitable events rather than
*       continuously polling. The wrapper provides neither synchronization of
*       shared application data nor automatic dispatch to the UI task; use the
*       appropriate AROS ownership and messaging rules. Do not free or retain
*       the supplied instance pointers beyond their lifetime.
*
*/
/****** Process.mui/MUIM_Process_Signal ************
*
*   NAME
*       MUIM_Process_Signal
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Process_Signal, sigs);
*
*   FUNCTION
*       Send an Exec signal mask to the stored worker task if non-NULL.
*
*   INPUTS
*       sigs - ULONG signal-bit mask, not a signal number. obj must remain live.
*
*   RESULT
*       Always zero; no delivery acknowledgement or error status.
*
*   NOTES
*       The method does not set the kill flag and does nothing for a NULL task
*       field. Checking the field and calling Signal are not protected against
*       concurrent worker exit or the launch race. The caller must not treat
*       this operation as a task-lifetime guard.
*
*/

extern struct Library *MUIMasterBase;


/*
 * Use current task UserData as live Process instance and synchronously
 * invoke source worker method through SourceClass or normal dispatch. Pass
 * borrowed kill flag and self; ignore callback result, then clear task
 * field before returning to DOS. No startup/exit handshake or locking;
 * source and instance must outlive the call.
 */
static void my_process(void)
{
    // invokes MUIM_Process_Process  for the the class/object specified by
    // MUIA_Process_SourceClass/Object (source class may be NULL)

    D(bug("[Process.mui] my_process called\n"));

    struct Task *thistask = FindTask(NULL);
    struct Process_DATA *data = thistask->tc_UserData;

    D(bug("[Process.mui::my_process] data %p sourceclass %p sourceobject %p\n",
        data, data->sourceclass, data->sourceobject));

    if (data->sourceclass)
    {
        CoerceMethod(data->sourceclass, data->sourceobject,
            MUIM_Process_Process, &data->kill, data->self);
    }
    else
    {
        DoMethod(data->sourceobject, MUIM_Process_Process, &data->kill,
            data->self);
    }

    data->task = NULL;          // show MUIM_Process_Kill that we're done

    D(bug("[Process.mui] my_process terminated\n"));
}


/*
 * Construct Semaphore object, set defaults, store borrowed launch/source
 * data and optionally call Launch before returning. Return object or zero
 * on superclass failure; ignore launch status. Source must already be valid
 * when AutoLaunch is nonzero; no ownership transfer for source/class/name.
 */
IPTR Process__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Process_DATA *data;
    struct TagItem *tag, *tags;
    struct Task *thistask;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    data->self = obj;

    // defaults
    data->autolaunch = TRUE;
    data->stacksize = 40000;
    thistask = FindTask(NULL);
    data->priority = thistask->tc_Node.ln_Pri;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Process_AutoLaunch:
            data->autolaunch = tag->ti_Data;
            break;

        case MUIA_Process_Name:
            data->name = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Process_Priority:
            data->priority = tag->ti_Data;
            break;

        case MUIA_Process_SourceClass:
            data->sourceclass = (struct IClass *)tag->ti_Data;
            break;

        case MUIA_Process_SourceObject:
            data->sourceobject = (Object *) tag->ti_Data;
            break;

        case MUIA_Process_StackSize:
            data->stacksize = tag->ti_Data;
            break;
        }
    }

    D(bug("muimaster.library/process.c: Process Object created at 0x%p\n",
            obj));

    if (data->autolaunch)
    {
        DoMethod(obj, MUIM_Process_Launch);
    }

    return (IPTR) obj;
}


/*
 * Zero writable caller storage, then return borrowed stored Process pointer
 * with TRUE for Task; delegate other queries to Semaphore. No
 * synchronization or process-lifetime reference. Pointer can be stale and
 * NULL does not certify final DOS exit.
 */
IPTR Process__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct Process_DATA *data = INST_DATA(cl, obj);

    STORE = 0;

    switch (msg->opg_AttrID)
    {
    case MUIA_Process_Task:
        STORE = (IPTR) data->task;
        return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}


/*
 * Call cooperative Kill with unlimited delay, then return Semaphore
 * disposal result. Requires caller outside worker and no resource deadlock;
 * can block indefinitely. Does not own source object/class/name or
 * independently force native task removal.
 */
IPTR Process__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    D(bug("[Process.mui/OM_DISPOSE]\n"));

    // struct Process_DATA *data = INST_DATA(cl, obj);

    DoMethod(obj, MUIM_Process_Kill, 0);

    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Set kill to one, repeatedly signal CTRL_C and Delay ten DOS ticks while
 * stored task is non-NULL, returning FALSE after strictly exceeding a
 * nonzero nominal limit, otherwise TRUE. No forced termination, UI pumping,
 * pointer protection or completion handshake. Caller must preserve
 * source/instance lifetime even on timeout.
 */
IPTR Process__MUIM_Process_Kill(struct IClass *cl, Object *obj,
    struct MUIP_Process_Kill *msg)
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

    data->kill = 1;             // stops the loop in Class4.c demo

    // the spawned task sets data->task to NULL on exit
    while (data->task != NULL)
    {
        Signal((struct Task *)data->task, SIGBREAKF_CTRL_C);
        Delay(DELAYTICKS);
        delay += DELAYTICKS;
        D(bug("[MUIM_Process_Kill] delay %d maxdelay %d\n", delay,
                msg->maxdelay));
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


/*
 * When task is NULL, CreateNewProc with borrowed instance UserData, store
 * result and then clear kill. Always return TRUE, including creation
 * failure; do nothing for non-NULL task. Worker can execute/finish before
 * assignment and observe prior kill on relaunch; no startup
 * synchronization.
 */
IPTR Process__MUIM_Process_Launch(struct IClass *cl, Object *obj,
    struct MUIP_Process_Launch *msg)
{
    D(bug("[MUIM_Process_Launch]\n"));

    struct Process_DATA *data = INST_DATA(cl, obj);

    // Starts process's loop (MUIM_Process_Process). If the loop
    // is already running does nothing.

    if (data->task == NULL)
    {
        struct TagItem tags[] = {
            {NP_Entry, (IPTR) my_process},
            {NP_StackSize, data->stacksize},
            {data->name ? NP_Name : TAG_IGNORE,
                (IPTR) data->name},
            {NP_Priority, data->priority},
            {NP_UserData, (IPTR) data},
            {TAG_DONE}
        };

        data->task = CreateNewProc(tags);

        data->kill = 0;
    }

    return (IPTR) TRUE;
}


/*
 * Base worker-method stub delegates to Semaphore and returns its result
 * without processing kill/proc. Override on the chosen source object/class
 * for useful work; worker ignores result. Message pointers remain borrowed
 * from the live Process object.
 */
IPTR Process__MUIM_Process_Process(struct IClass *cl, Object *obj,
    struct MUIP_Process_Process *msg)
{
    D(bug("[MUIM_Process_Process] kill %p proc %p\n", msg->kill,
            msg->proc));

    // struct Process_DATA *data = INST_DATA(cl, obj);

    // Main process method. Terminating condition is passed in message struct.
    // Proper implementation should wait for a signal to not use 100% cpu.
    // This is some kind of a virtual function. Sub-class implementators
    // must overwrite it.

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


/*
 * If stored task is non-NULL, pass ULONG signal mask to Exec Signal, then
 * return zero. No kill-flag update or acknowledgement. Task check and send
 * are unprotected against worker exit or stale startup state.
 */
IPTR Process__MUIM_Process_Signal(struct IClass *cl, Object *obj,
    struct MUIP_Process_Signal *msg)
{
    D(bug("[MUIM_Process_Signal] sigs %u\n", msg->sigs));

    struct Process_DATA *data = INST_DATA(cl, obj);

    // just send an arbitrary signal to the spawned process.
    if (data->task)
    {
        Signal((struct Task *)data->task, msg->sigs);
    }

    return 0;
}


#if ZUNE_BUILTIN_PROCESS
/*
 * Dispatch local overrides and return handler results; delegate other
 * messages to the declared superclass. Requires correct method ABI and live
 * object except construction. No implicit additional ownership,
 * synchronization or conversion is supplied by this dispatcher.
 */
BOOPSI_DISPATCHER(IPTR, Process_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Process__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Process__OM_GET(cl, obj, (struct opGet *)msg);
    case OM_DISPOSE:
        return Process__OM_DISPOSE(cl, obj, msg);
    case MUIM_Process_Kill:
        return Process__MUIM_Process_Kill(cl, obj,
            (struct MUIP_Process_Kill *)msg);
    case MUIM_Process_Launch:
        return Process__MUIM_Process_Launch(cl, obj,
            (struct MUIP_Process_Launch *)msg);
    case MUIM_Process_Process:
        return Process__MUIM_Process_Process(cl, obj,
            (struct MUIP_Process_Process *)msg);
    case MUIM_Process_Signal:
        return Process__MUIM_Process_Signal(cl, obj,
            (struct MUIP_Process_Signal *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Process_desc =
{
    MUIC_Process,
    MUIC_Semaphore,
    sizeof(struct Process_DATA),
    (void *) Process_Dispatcher
};
#endif /* ZUNE_BUILTIN_PROCESS */
