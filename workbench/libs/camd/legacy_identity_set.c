#include "legacy_identity_set.h"

#include <limits.h>
#include <string.h>

static enum CAMDIdentityMapResult resolve(
    struct CAMDIdentityMap *map, const struct CAMDIdentityKeyV1 *key,
    struct CAMDIdentityResolutionV1 *resolution)
{
    memset(resolution, 0, sizeof(*resolution));
    resolution->Size = sizeof(*resolution);
    resolution->Version = 1;
    return camd_identity_map_resolve(map, key, resolution);
}

enum CAMDIdentityMapResult camd_legacy_identity_set_resolve(
    struct CAMDIdentityMap *map, const char *module_path, size_t port_count,
    struct CAMDEndpointIDV1 *provider_id,
    struct CAMDEndpointIDV1 *endpoint_ids, uint32_t *identity_kind)
{
    struct CAMDIdentityKeyV1 provider_key, endpoint_key;
    struct CAMDIdentityResolutionV1 resolution;
    enum CAMDIdentityMapResult result;
    size_t i;

    if (provider_id)
        memset(provider_id, 0, sizeof(*provider_id));
    if (identity_kind)
        *identity_kind = 0;
    if (!map || !module_path || port_count == 0 || port_count > UINT32_MAX ||
        port_count > SIZE_MAX / sizeof(*endpoint_ids) ||
        !provider_id || !endpoint_ids || !identity_kind)
        return CAMD_IDENTITY_MAP_INVALID;
    memset(endpoint_ids, 0, port_count * sizeof(*endpoint_ids));

    /* Validate both key shapes before adding any mapping. */
    result = camd_legacy_provider_identity_key(module_path, &provider_key);
    if (result != CAMD_IDENTITY_MAP_OK)
        return result;
    result = camd_legacy_endpoint_identity_key(
        module_path, (uint32_t)(port_count - 1), &endpoint_key);
    if (result != CAMD_IDENTITY_MAP_OK)
        return result;

    result = resolve(map, &provider_key, &resolution);
    if (result != CAMD_IDENTITY_MAP_OK)
        goto fail;
    *provider_id = resolution.ID;
    for (i = 0; i < port_count; ++i) {
        result = camd_legacy_endpoint_identity_key(
            module_path, (uint32_t)i, &endpoint_key);
        if (result != CAMD_IDENTITY_MAP_OK)
            goto fail;
        result = resolve(map, &endpoint_key, &resolution);
        if (result != CAMD_IDENTITY_MAP_OK)
            goto fail;
        endpoint_ids[i] = resolution.ID;
    }

    /* A later endpoint commit can promote the provider mapping. Refresh its
     * effective confidence before the set is published. */
    result = resolve(map, &provider_key, &resolution);
    if (result != CAMD_IDENTITY_MAP_OK)
        goto fail;
    *provider_id = resolution.ID;
    *identity_kind = resolution.EffectiveConfidence;
    return CAMD_IDENTITY_MAP_OK;

fail:
    memset(provider_id, 0, sizeof(*provider_id));
    memset(endpoint_ids, 0, port_count * sizeof(*endpoint_ids));
    return result;
}
