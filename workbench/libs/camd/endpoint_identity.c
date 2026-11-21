/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* The stored endpoint IDs as a program may see them: the store's state, a
   listing, and forgetting one. */

#include <proto/exec.h>

#include <string.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "identity_map.h"
#include "identity_store_aros.h"
#include "legacy_identity_key.h"

/* Whether an endpoint or what offers it has this ID at the moment. */
static BOOL identity_in_use(const struct CAMDEndpointIDV1 *id,
                            struct CamdBase *CamdBase)
{
    struct CAMDEndpointSnapshot *snapshot = NULL;
    const struct CAMDEndpointInfoV1 *endpoint;
    const struct CAMDGroupInfoV1 *groups;
    const struct CAMDFunctionBlockInfoV1 *blocks;
    size_t group_count, block_count, count, i;
    BOOL used = FALSE;

    if (camd_registry_snapshot(CB(CamdBase)->EndpointRegistry, &snapshot) !=
        CAMD_REGISTRY_OK)
        return TRUE;    /* cannot tell: do not let it be forgotten */
    count = camd_snapshot_endpoint_count(snapshot);
    for (i = 0; i < count && !used; ++i) {
        if (camd_snapshot_endpoint(snapshot, i, &endpoint, &groups,
                                   &group_count, &blocks, &block_count) !=
            CAMD_REGISTRY_OK)
            continue;
        used = memcmp(&endpoint->ID, id, sizeof(*id)) == 0 ||
               memcmp(&endpoint->ProviderID, id, sizeof(*id)) == 0;
    }
    camd_snapshot_destroy(snapshot);
    return used;
}

LONG ReadIdentityStore(struct CAMDIdentityStoreInfoV1 *info,
                       struct CamdBase *CamdBase)
{
    struct CAMDIdentityStoreAROS *store = CB(CamdBase)->EndpointIdentity;
    struct CAMDIdentityMap *map = camd_identity_store_aros_map(store);

    memset(info, 0, sizeof(*info));
    info->Size = sizeof(*info);
    info->Version = 1;
    info->State = camd_identity_store_aros_state(store);
    info->Stored = camd_identity_map_count(map);
    info->Capacity = camd_identity_map_capacity(map);
    return CAMD_REGISTRY_OK;
}

/* Keys are a format byte, a length, the name and, for a driver's port, its
 * number: see legacy_identity_key.c and resolve_name() in
 * endpoint_publish.c. */
LONG ReadStoredIdentity(ULONG index, struct CAMDStoredIdentityV1 *identity,
                        struct CamdBase *CamdBase)
{
    struct CAMDIdentityMap *map =
        camd_identity_store_aros_map(CB(CamdBase)->EndpointIdentity);
    struct CAMDIdentityRecordV1 record;
    uint32_t persistent = 0;
    ULONG length;

    /* Mappings that could not be written are not stored: skip them. */
    for (;;) {
        enum CAMDIdentityMapResult got;
        ULONG skipped = 0, at;

        if (map == NULL)
            return CAMD_REGISTRY_RANGE;
        for (at = 0;; ++at) {
            got = camd_identity_map_get(map, at, &record, &persistent);
            if (got != CAMD_IDENTITY_MAP_OK)
                return CAMD_REGISTRY_RANGE;
            if (!persistent) {
                ++skipped;
                continue;
            }
            if (at - skipped == index)
                break;
        }
        break;
    }

    memset(identity, 0, sizeof(*identity));
    identity->Size = sizeof(*identity);
    identity->Version = 1;
    identity->ID = record.ID;
    switch (record.Namespace) {
    case CAMD_IDENTITY_NAMESPACE_LEGACY_PROVIDER:
        identity->Flags |= CAMD_STORED_IDENTITY_PROVIDER;
        /* fall through */
    case CAMD_IDENTITY_NAMESPACE_LEGACY_ENDPOINT:
        identity->Kind = CAMD_IDENTITY_STORED_BY_PATH;
        break;
    case NAMESPACE_APP_PROVIDER:
        identity->Flags |= CAMD_STORED_IDENTITY_PROVIDER;
        /* fall through */
    case NAMESPACE_APP_ENDPOINT:
        identity->Kind = CAMD_IDENTITY_STORED_BY_NAME;
        break;
    }
    if (record.KeyByteCount >= 2 && record.Key[0] == 1 &&
        2u + record.Key[1] <= record.KeyByteCount) {
        length = record.Key[1];
        if (length > sizeof(identity->Name) - 1)
            length = sizeof(identity->Name) - 1;
        CopyMem(&record.Key[2], identity->Name, length);
        if (record.Namespace == CAMD_IDENTITY_NAMESPACE_LEGACY_ENDPOINT &&
            record.KeyByteCount == 2u + record.Key[1] + 4u) {
            const UBYTE *port = &record.Key[2 + record.Key[1]];

            identity->Port = ((ULONG)port[0] << 24) | ((ULONG)port[1] << 16) |
                             ((ULONG)port[2] << 8) | (ULONG)port[3];
        }
    }
    if (identity_in_use(&record.ID, CamdBase))
        identity->Flags |= CAMD_STORED_IDENTITY_IN_USE;
    return CAMD_REGISTRY_OK;
}

LONG ForgetStoredIdentity(const struct CAMDEndpointIDV1 *id,
                          struct CamdBase *CamdBase)
{
    struct CAMDIdentityStoreAROS *store = CB(CamdBase)->EndpointIdentity;
    struct CAMDIdentityMap *map = camd_identity_store_aros_map(store);
    LONG result;

    if (map == NULL ||
        camd_identity_store_aros_state(store) == CAMD_IDENTITY_STORE_UNREADABLE)
        return CAMD_REGISTRY_STATE;
    /* Publishing takes this semaphore too, so nothing starts to use the ID
     * between the look and the removal. */
    ObtainSemaphore(&CB(CamdBase)->EndpointClientSemaphore);
    if (identity_in_use(id, CamdBase)) {
        result = CAMD_REGISTRY_BUSY;
    } else {
        switch (camd_identity_map_remove(map, id)) {
        case CAMD_IDENTITY_MAP_OK:      result = CAMD_REGISTRY_OK; break;
        case CAMD_IDENTITY_MAP_MISSING: result = CAMD_REGISTRY_STALE; break;
        case CAMD_IDENTITY_MAP_STORAGE: result = CAMD_REGISTRY_CALLBACK_FAILED; break;
        default:                        result = CAMD_REGISTRY_INVALID; break;
        }
    }
    ReleaseSemaphore(&CB(CamdBase)->EndpointClientSemaphore);
    return result;
}
