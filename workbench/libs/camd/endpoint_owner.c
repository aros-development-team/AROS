/*
 * Whose endpoint objects are whose.
 *
 * A session, snapshot, watch or published endpoint belongs to the task that
 * made it. When that task closes camd.library for the last time, what it
 * left behind is released as if it had done so itself: its signals stop and
 * the library can go. A task that ends without closing the library is not
 * seen here; its objects stay and keep the library loaded.
 */

#include <proto/exec.h>
#include <exec/memory.h>

#include "camd_intern.h"
#include "endpoint_registry.h"

struct EndpointOpener {
    struct MinNode node;
    struct Task *task;
    ULONG count;
};

struct EndpointOwned {
    struct MinNode node;
    struct Task *task;
    ULONG kind;
    struct CAMDHandleV1 handle;
};

#define FOR_NODES(list, pointer, type) \
    for ((pointer) = (type *)(list)->mlh_Head; \
         (pointer)->node.mln_Succ != NULL; \
         (pointer) = (type *)(pointer)->node.mln_Succ)

/* Without memory the object simply has no owner and is not cleaned up. */
void OwnEndpointObject(ULONG kind, const struct CAMDHandleV1 *handle,
                       struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct EndpointOwned *owned;

    owned = AllocMem(sizeof(*owned), MEMF_ANY | MEMF_CLEAR);
    if (owned == NULL)
        return;
    owned->task = FindTask(NULL);
    owned->kind = kind;
    owned->handle = *handle;
    ObtainSemaphore(&base->EndpointOwnerSemaphore);
    AddTail((struct List *)&base->EndpointOwned, (struct Node *)owned);
    ReleaseSemaphore(&base->EndpointOwnerSemaphore);
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

void TaskOpenedCamd(struct CamdBase *CamdBase)
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

/* The calling task closes the library. On its last close, what it still
 * owns is released: sessions first, since they pin what it published. */
void TaskClosedCamd(struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct Task *task = FindTask(NULL);
    struct EndpointOpener *opener, *found = NULL;
    struct EndpointOwned *owned;
    ULONG kind;
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

    for (kind = ENDPOINT_OWNED_SNAPSHOT; kind <= ENDPOINT_OWNED_PUBLISHER;
         ++kind) {
        for (;;) {
            struct EndpointOwned taken;
            BOOL any = FALSE;

            /* One at a time and outside the semaphore: releasing takes
             * other locks and comes back through DisownEndpointObject(). */
            ObtainSemaphore(&base->EndpointOwnerSemaphore);
            FOR_NODES(&base->EndpointOwned, owned, struct EndpointOwned) {
                if (owned->task == task && owned->kind == kind) {
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
