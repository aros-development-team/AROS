/*
 * Snapshot handles of the endpoint client functions.
 *
 * A client holds a slot and a generation, never the snapshot's address, so a
 * stale or foreign handle is refused instead of followed.
 */

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"

LONG AddEndpointSnapshot(struct CAMDEndpointSnapshot *snapshot,
                         struct CAMDHandleV1 *handle,
                         struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    LONG result = CAMD_REGISTRY_NOMEM;
    ULONG i;

    ObtainSemaphore(&base->EndpointClientSemaphore);
    for (i = 0; i < ENDPOINT_SNAPSHOTS; ++i) {
        if (base->EndpointSnapshots[i] == NULL) {
            if (++base->EndpointSnapshotGenerations[i] == 0)
                ++base->EndpointSnapshotGenerations[i];
            base->EndpointSnapshots[i] = snapshot;
            handle->slot = i;
            handle->generation = base->EndpointSnapshotGenerations[i];
            result = CAMD_REGISTRY_OK;
            break;
        }
    }
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    return result;
}

/* EndpointClientSemaphore must be held; the snapshot is good while it is. */
struct CAMDEndpointSnapshot *FindEndpointSnapshot(
    const struct CAMDHandleV1 *handle, struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);

    if (handle->slot >= ENDPOINT_SNAPSHOTS || handle->generation == 0 ||
        base->EndpointSnapshotGenerations[handle->slot] != handle->generation)
        return NULL;
    return base->EndpointSnapshots[handle->slot];
}

void RemoveEndpointSnapshot(const struct CAMDHandleV1 *handle,
                            struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    struct CAMDEndpointSnapshot *snapshot;

    ObtainSemaphore(&base->EndpointClientSemaphore);
    snapshot = FindEndpointSnapshot(handle, CamdBase);
    if (snapshot != NULL)
        base->EndpointSnapshots[handle->slot] = NULL;
    ReleaseSemaphore(&base->EndpointClientSemaphore);
    if (snapshot != NULL)
        camd_snapshot_destroy(snapshot);
}

/* Called from expunge with the library closed, so no client is inside CAMD
 * and the snapshot table needs no lock. */
BOOL EndpointClientsRemain(struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    ULONG i;

    for (i = 0; i < ENDPOINT_SNAPSHOTS; ++i) {
        if (base->EndpointSnapshots[i] != NULL)
            return TRUE;
    }
    return camd_registry_session_count(base->EndpointRegistry) != 0;
}

/* Snapshots a client did not release. CAMD must be expunging. */
void FreeEndpointSnapshots(struct CamdBase *CamdBase)
{
    struct CamdBase_intern *base = CB(CamdBase);
    ULONG i;

    for (i = 0; i < ENDPOINT_SNAPSHOTS; ++i) {
        if (base->EndpointSnapshots[i] != NULL) {
            camd_snapshot_destroy(base->EndpointSnapshots[i]);
            base->EndpointSnapshots[i] = NULL;
        }
    }
}
