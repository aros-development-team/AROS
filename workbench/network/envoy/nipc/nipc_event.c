/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library - timer events. Each pending event has an absolute
          due time; the supervisor delivers due events on every tick
          (ReplyMsg to the event port), in due-time order.
*/

#include <proto/exec.h>
#include <proto/timer.h>
#include <string.h>

#include <proto/nipc.h>

#include "nipc_intern.h"

#define TimerBase               ((struct Device *)NIPCBase->nipc_TimerBase)

static void TvAdd(struct timeval *tv, ULONG secs, ULONG micros)
{
    tv->tv_secs += secs + micros / 1000000;
    tv->tv_micro += micros % 1000000;
    if (tv->tv_micro >= 1000000)
    {
        tv->tv_micro -= 1000000;
        tv->tv_secs++;
    }
}

static LONG TvCmp(const struct timeval *a, const struct timeval *b)
{
    if (a->tv_secs != b->tv_secs)
        return a->tv_secs < b->tv_secs ? -1 : 1;
    if (a->tv_micro != b->tv_micro)
        return a->tv_micro < b->tv_micro ? -1 : 1;
    return 0;
}

/* Take an event off wherever it is (under Sem) */
static void UnlinkEvent(struct NIPCBase *NIPCBase, struct NIPCEvent *ev)
{
    if (ev->State == EVENT_PENDING)
        Remove((struct Node *)&ev->Node);
    else if (ev->State == EVENT_DELIVERED)
    {
        /* queued at its port: remove the message */
        Disable();
        if (ev->Msg.mn_Node.ln_Succ)
            Remove(&ev->Msg.mn_Node);
        Enable();
    }
    ev->State = EVENT_IDLE;
}

static void DeliverEvent(struct NIPCBase *NIPCBase, struct NIPCEvent *ev)
{
    ev->State = EVENT_DELIVERED;
    ReplyMsg(&ev->Msg);
}

/* Supervisor tick: deliver everything that is due */
void EventTick(struct NIPCBase *NIPCBase, const struct timeval *now)
{
    struct NIPCEvent *ev;

    ObtainSemaphore(&NIPCBase->Sem);
    while ((ev = (struct NIPCEvent *)GetHead(&NIPCBase->Events)))
    {
        struct NIPCEvent *e = (struct NIPCEvent *)((IPTR)ev - offsetof(struct NIPCEvent, Node));
        if (TvCmp(&e->Due, now) > 0)
            break;
        Remove((struct Node *)&e->Node);
        DeliverEvent(NIPCBase, e);
    }
    ReleaseSemaphore(&NIPCBase->Sem);
}

/*****************************************************************************

    NAME */
        AROS_LH0(struct MsgPort *, AllocNIPCEventPort,

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 67, NIPC)

/*  FUNCTION
        A message port for events (CreateMsgPort()).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return CreateMsgPort();

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreeNIPCEventPort,

/*  SYNOPSIS */
        AROS_LHA(struct MsgPort *, eventport, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 76, NIPC)

/*  FUNCTION
        Free an event port (DeleteMsgPort()). Free or abort its events first.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (eventport)
        DeleteMsgPort(eventport);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(struct NIPCEvent *, AllocNIPCEvent,

/*  SYNOPSIS */
        AROS_LHA(ULONG, eventcode, D0),
        AROS_LHA(APTR, eventdata, A0),
        AROS_LHA(struct MsgPort *, eventport, A1),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 68, NIPC)

/*  FUNCTION
        Allocate an event that will be delivered to eventport.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NIPCEvent *ev;

    if (!eventport || !(ev = AllocVec(sizeof(struct NIPCEvent), MEMF_CLEAR | MEMF_PUBLIC)))
        return NULL;
    ev->Msg.mn_Node.ln_Type = NT_FREEMSG;
    ev->Msg.mn_ReplyPort = eventport;
    ev->Msg.mn_Length = sizeof(struct NIPCEvent);
    ev->Code = eventcode;
    ev->Data = eventdata;
    return ev;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreeNIPCEvent,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCEvent *, event, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 75, NIPC)

/*  FUNCTION
        Free an event in any state; a pending one is cancelled silently.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!event)
        return;
    ObtainSemaphore(&NIPCBase->Sem);
    UnlinkEvent(NIPCBase, event);
    ReleaseSemaphore(&NIPCBase->Sem);
    FreeVec(event);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(void, ScheduleNIPCEvent,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCEvent *, event, A0),
        AROS_LHA(ULONG, seconds, D0),
        AROS_LHA(ULONG, micros, D1),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 69, NIPC)

/*  FUNCTION
        Deliver the event after the given time (immediately if both are 0).
        Rescheduling replaces an earlier schedule.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NIPCEvent *ev = event, *n;

    if (!ev)
        return;
    ObtainSemaphore(&NIPCBase->Sem);
    UnlinkEvent(NIPCBase, ev);
    if (!seconds && !micros)
    {
        DeliverEvent(NIPCBase, ev);
        ReleaseSemaphore(&NIPCBase->Sem);
        return;
    }
    GetNow(NIPCBase, &ev->Scheduled);
    ev->Due = ev->Scheduled;
    TvAdd(&ev->Due, seconds, micros);
    ev->State = EVENT_PENDING;
    /* insert in due order; ties after existing ones */
    ForeachNode(&NIPCBase->Events, n)
    {
        struct NIPCEvent *e = (struct NIPCEvent *)((IPTR)n - offsetof(struct NIPCEvent, Node));
        if (TvCmp(&e->Due, &ev->Due) > 0)
        {
            Insert((struct List *)&NIPCBase->Events, (struct Node *)&ev->Node, ((struct Node *)&e->Node)->ln_Pred);
            ReleaseSemaphore(&NIPCBase->Sem);
            return;
        }
    }
    AddTail((struct List *)&NIPCBase->Events, (struct Node *)&ev->Node);
    ReleaseSemaphore(&NIPCBase->Sem);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, AbortNIPCEvent,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCEvent *, event, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 74, NIPC)

/*  FUNCTION
        Deliver a pending event now instead of at its time.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!event)
        return;
    ObtainSemaphore(&NIPCBase->Sem);
    if (event->State == EVENT_PENDING)
    {
        Remove((struct Node *)&event->Node);
        DeliverEvent(NIPCBase, event);
    }
    ReleaseSemaphore(&NIPCBase->Sem);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, CancelNIPCEvent,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCEvent *, event, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 82, NIPC)

/*  FUNCTION
        Take a pending event out of the queue without delivering it.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!event)
        return;
    ObtainSemaphore(&NIPCBase->Sem);
    if (event->State == EVENT_PENDING)
        UnlinkEvent(NIPCBase, event);
    ReleaseSemaphore(&NIPCBase->Sem);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, CheckNIPCEvent,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCEvent *, event, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 70, NIPC)

/*  FUNCTION
        1 while the event is pending, 0 otherwise.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (event && event->State == EVENT_PENDING) ? 1 : 0;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(struct NIPCEvent *, GetNIPCEvent,

/*  SYNOPSIS */
        AROS_LHA(struct MsgPort *, eventport, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 71, NIPC)

/*  FUNCTION
        The next delivered event at the port, or NULL.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NIPCEvent *ev;

    if (!eventport)
        return NULL;
    ObtainSemaphore(&NIPCBase->Sem);
    if ((ev = (struct NIPCEvent *)GetMsg(eventport)))
        ev->State = EVENT_IDLE;
    ReleaseSemaphore(&NIPCBase->Sem);
    return ev;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, GetNIPCEventCode,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCEvent *, event, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 72, NIPC)

/*  FUNCTION
        Field of an event.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return event ? event->Code : 0;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(APTR, GetNIPCEventData,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCEvent *, event, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 73, NIPC)

/*  FUNCTION
        Field of an event.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return event ? event->Data : NULL;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, GetNIPCEventTime,

/*  SYNOPSIS */
        AROS_LHA(struct NIPCEvent *, event, A0),
        AROS_LHA(struct timeval *, tv, A1),
        AROS_LHA(BOOL, remaining, D0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 83, NIPC)

/*  FUNCTION
        For a pending event: the time it was scheduled, or (remaining)
        the time left until it is due.

    RESULT
        TRUE if the event was pending and tv was filled in.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct timeval now;
    BOOL ok = FALSE;

    if (!event || !tv)
        return FALSE;
    ObtainSemaphore(&NIPCBase->Sem);
    if (event->State == EVENT_PENDING)
    {
        if (remaining)
        {
            GetNow(NIPCBase, &now);
            if (TvCmp(&event->Due, &now) > 0)
            {
                *tv = event->Due;
                SubTime(tv, &now);
            }
            else
                tv->tv_secs = tv->tv_micro = 0;
        }
        else
            *tv = event->Scheduled;
        ok = TRUE;
    }
    ReleaseSemaphore(&NIPCBase->Sem);
    return ok;

    AROS_LIBFUNC_EXIT
}
