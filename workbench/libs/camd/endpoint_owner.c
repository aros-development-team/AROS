/*
 * Whose endpoint objects are whose.
 *
 * A session, snapshot, watch or published endpoint belongs to the task that
 * made it. When that task closes camd.library for the last time, what it
 * left behind is released as if it had done so itself: its signals stop and
 * the library can go. task.resource marks objects whose task is being
 * removed; a private CAMD task releases them outside the notification hook.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#ifndef __amigaos4__
#include <proto/task.h>
#include <resources/task.h>
#include <aros/asmcall.h>
#endif

#include "camd_intern.h"
#include "endpoint_registry.h"

struct EndpointOpener {
    struct MinNode node;
    struct Task *task;
    ULONG count;
    BOOL dead;
};

struct EndpointOwned {
    struct MinNode node;
    struct Task *task;
    ULONG kind;
    struct CAMDHandleV1 handle;
    BOOL dead;
};

#define FOR_NODES(list, pointer, type) \
    for ((pointer) = (type *)(list)->mlh_Head; \
         (pointer)->node.mln_Succ != NULL; \
         (pointer) = (type *)(pointer)->node.mln_Succ)

BOOL OwnEndpointObject(ULONG kind, const struct CAMDHandleV1 *handle,
                       struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointOwned *owned;

    ReapDeadEndpointOwners(CamdBase);
    owned = AllocMem(sizeof(*owned), MEMF_ANY | MEMF_CLEAR);
    if (owned == NULL)
        return FALSE;
    owned->task = FindTask(NULL);
    owned->kind = kind;
    owned->handle = *handle;
    ObtainSemaphore(&base->EndpointOwnerSemaphore);
    AddTail((struct List *)&base->EndpointOwned, (struct Node *)owned);
    ReleaseSemaphore(&base->EndpointOwnerSemaphore);
    return TRUE;
}

/* The object is gone, whoever released it. */
void DisownEndpointObject(ULONG kind, const struct CAMDHandleV1 *handle,
                          struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointOwned *owned, *found = NULL;

    ObtainSemaphore(&base->EndpointOwnerSemaphore);
    FOR_NODES(&base->EndpointOwned, owned, struct EndpointOwned) {
        if (owned->kind == kind && owned->handle.slot == handle->slot &&
            owned->handle.generation == handle->generation) {
            found = owned;
            break;
        }
    }
    if (found != NULL)
        Remove((struct Node *)found);
    ReleaseSemaphore(&base->EndpointOwnerSemaphore);
    if (found != NULL)
        FreeMem(found, sizeof(*found));
}

BOOL TaskOpenedCamd(struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct Task *task = FindTask(NULL);
    struct EndpointOpener *opener, *found = NULL;

    ObtainSemaphore(&base->EndpointOwnerSemaphore);
    FOR_NODES(&base->EndpointOpeners, opener, struct EndpointOpener) {
        if (opener->task == task) {
            found = opener;
            break;
        }
    }
    if (found == NULL) {
        found = AllocMem(sizeof(*found), MEMF_ANY | MEMF_CLEAR);
        if (found != NULL) {
            found->task = task;
            AddTail((struct List *)&base->EndpointOpeners,
                    (struct Node *)found);
        }
    }
    if (found != NULL)
        ++found->count;
    ReleaseSemaphore(&base->EndpointOwnerSemaphore);
    return found != NULL;
}

static void release_owned(const struct EndpointOwned *owned,
                          struct CamdBase *CamdBase)
{
    switch (owned->kind) {
    case ENDPOINT_OWNED_SNAPSHOT:
        RemoveEndpointSnapshot(&owned->handle, CamdBase);
        break;
    case ENDPOINT_OWNED_SESSION:
        StopEndpointReceiver(&owned->handle, CamdBase);
        camd_registry_session_close(CB(CamdBase)->EndpointRegistry,
                                    owned->handle);
        break;
    case ENDPOINT_OWNED_WATCH:
        EndEndpointWatcher(&owned->handle, CamdBase);
        break;
    case ENDPOINT_OWNED_PUBLISHER:
        WithdrawAppEndpoint(&owned->handle, CamdBase);
        break;
    }
}

static void release_matching(struct Task *task, BOOL dead_only,
                             struct CamdBase *CamdBase)
{
    static const ULONG order[] = {
        ENDPOINT_OWNED_SESSION,
        ENDPOINT_OWNED_SNAPSHOT,
        ENDPOINT_OWNED_WATCH,
        ENDPOINT_OWNED_PUBLISHER
    };
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointOwned *owned;
    ULONG index;

    for (index = 0; index < sizeof(order) / sizeof(order[0]); ++index) {
        for (;;) {
            struct EndpointOwned taken;
            BOOL any = FALSE;

            ObtainSemaphore(&base->EndpointOwnerSemaphore);
            FOR_NODES(&base->EndpointOwned, owned, struct EndpointOwned) {
                if ((task == NULL || owned->task == task) &&
                    owned->kind == order[index] &&
                    (!dead_only || owned->dead)) {
                    taken = *owned;
                    Remove((struct Node *)owned);
                    FreeMem(owned, sizeof(*owned));
                    any = TRUE;
                    break;
                }
            }
            ReleaseSemaphore(&base->EndpointOwnerSemaphore);
            if (!any)
                break;
            release_owned(&taken, CamdBase);
        }
    }
}

void ReapDeadEndpointOwners(struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointOpener *opener;

    for (;;) {
        struct EndpointOpener *dead = NULL;

        ObtainSemaphore(&base->EndpointOwnerSemaphore);
        FOR_NODES(&base->EndpointOpeners, opener, struct EndpointOpener) {
            if (opener->dead) {
                dead = opener;
                Remove((struct Node *)dead);
                break;
            }
        }
        ReleaseSemaphore(&base->EndpointOwnerSemaphore);
        if (dead == NULL)
            break;
        FreeMem(dead, sizeof(*dead));
    }
    release_matching(NULL, TRUE, CamdBase);
}

/* The calling task closes the library. On its last close, what it still
 * owns is released: sessions first, since they pin what it published. */
void TaskClosedCamd(struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct Task *task = FindTask(NULL);
    struct EndpointOpener *opener, *found = NULL;
    BOOL last = FALSE;

    ObtainSemaphore(&base->EndpointOwnerSemaphore);
    FOR_NODES(&base->EndpointOpeners, opener, struct EndpointOpener) {
        if (opener->task == task) {
            found = opener;
            break;
        }
    }
    if (found != NULL && --found->count == 0) {
        Remove((struct Node *)found);
        FreeMem(found, sizeof(*found));
        last = TRUE;
    }
    ReleaseSemaphore(&base->EndpointOwnerSemaphore);
    if (!last)
        return;
    release_matching(task, FALSE, CamdBase);
}

#ifndef __amigaos4__
AROS_UFH3(static IPTR, endpoint_task_notify,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct Task *, caller, A2),
    AROS_UFHA(struct TaskNotifyMsg *, message, A1))
{
    AROS_USERFUNC_INIT

    struct CamdBase *CamdBase = hook->h_Data;
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointOpener *opener;
    struct EndpointOwned *owned;

    (void)caller;
    if (message->tnm_Action != TNA_REMOVED)
        return 0;

    ObtainSemaphore(&base->EndpointOwnerSemaphore);
    FOR_NODES(&base->EndpointOpeners, opener, struct EndpointOpener)
        if (opener->task == message->tnm_Task)
            opener->dead = TRUE;
    FOR_NODES(&base->EndpointOwned, owned, struct EndpointOwned)
        if (owned->task == message->tnm_Task)
            owned->dead = TRUE;
    ReleaseSemaphore(&base->EndpointOwnerSemaphore);

    DisableEndpointTaskSignals(message->tnm_Task, CamdBase);
    if (base->EndpointOwnerReaper != NULL)
        Signal(base->EndpointOwnerReaper, SIGBREAKF_CTRL_E);
    return 0;

    AROS_USERFUNC_EXIT
}

static void endpoint_owner_reaper(void)
{
    struct CamdBase *CamdBase = FindTask(NULL)->tc_UserData;
    struct CamdBase_intern *base = CB(CamdBase);
    ULONG signals;

    do {
        signals = Wait(SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F);
        ReapDeadEndpointOwners(CamdBase);
    } while ((signals & SIGBREAKF_CTRL_F) == 0);

    Forbid();
    base->EndpointOwnerReaper = NULL;
    Permit();
}
#endif

BOOL InitEndpointOwners(struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);

    InitSemaphore(&base->EndpointOwnerSemaphore);
    InitSemaphore(&base->EndpointSignalSemaphore);
    NEWLIST((struct List *)&base->EndpointOpeners);
    NEWLIST((struct List *)&base->EndpointOwned);
#ifndef __amigaos4__
    base->EndpointOwnerReaper = NewCreateTask(
        TASKTAG_PC, (IPTR)endpoint_owner_reaper,
        TASKTAG_NAME, (IPTR)"camd endpoint owner reaper",
        TASKTAG_STACKSIZE, 32768,
        TASKTAG_USERDATA, (IPTR)CamdBase,
        TAG_DONE);
    if (base->EndpointOwnerReaper == NULL)
        return FALSE;
    base->EndpointTaskResource = OpenResource("task.resource");
    if (base->EndpointTaskResource != NULL) {
        struct Library *TaskResBase = base->EndpointTaskResource;
        base->EndpointTaskHook.h_Entry = (HOOKFUNC)endpoint_task_notify;
        base->EndpointTaskHook.h_SubEntry = NULL;
        base->EndpointTaskHook.h_Data = CamdBase;
        if (AddTaskNotifyHook(&base->EndpointTaskHook))
            return TRUE;
        base->EndpointTaskResource = NULL;
    }
    Signal(base->EndpointOwnerReaper, SIGBREAKF_CTRL_F);
    while (base->EndpointOwnerReaper != NULL)
        Delay(1);
    return FALSE;
#else
    return TRUE;
#endif
}

void UninitEndpointOwners(struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);

#ifndef __amigaos4__
    if (base->EndpointTaskResource != NULL) {
        struct Library *TaskResBase = base->EndpointTaskResource;
        RemTaskNotifyHook(&base->EndpointTaskHook);
        base->EndpointTaskResource = NULL;
    }
    if (base->EndpointOwnerReaper != NULL) {
        Signal(base->EndpointOwnerReaper, SIGBREAKF_CTRL_F);
        while (base->EndpointOwnerReaper != NULL)
            Delay(1);
    }
#endif
    ReapDeadEndpointOwners(CamdBase);
    FreeEndpointOwners(CamdBase);
}

/* CAMD must be expunging. */
void FreeEndpointOwners(struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct Node *node;

    while ((node = RemHead((struct List *)&base->EndpointOpeners)) != NULL)
        FreeMem(node, sizeof(struct EndpointOpener));
    while ((node = RemHead((struct List *)&base->EndpointOwned)) != NULL)
        FreeMem(node, sizeof(struct EndpointOwned));
}

/*
 * A record a caller hands in may be of another size than today's: a program
 * built against an earlier header has a shorter one, one built against a
 * later header a longer one. `given` starts with Size and Version. It is
 * accepted when it has at least `least` bytes, which is the record as it
 * first was, and when whatever it has beyond `size`, today's record, is zero:
 * a later field left at zero asks for nothing this library lacks. `record`
 * gets `size` bytes: the caller's, and zero for what its record did not
 * have, so a field added later means its default at zero.
 */
BOOL TakeEndpointRecord(const APTR given, APTR record, ULONG size, ULONG least)
{
    const UBYTE *from = given;
    ULONG have = *(const ULONG *)given;
    ULONG i;

    if (have < least)
        return FALSE;
    for (i = size; i < have; ++i) {
        if (from[i] != 0)
            return FALSE;
    }
    if (have > size)
        have = size;
    CopyMem(from, record, have);
    for (i = have; i < size; ++i)
        ((UBYTE *)record)[i] = 0;
    *(ULONG *)record = size;
    return TRUE;
}
