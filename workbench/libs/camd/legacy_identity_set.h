#ifndef CAMD_LEGACY_IDENTITY_SET_H
#define CAMD_LEGACY_IDENTITY_SET_H

#include "legacy_identity_key.h"

/* Resolve one provider ID and one direction-independent ID per fixed port.
 * For a valid output span, outputs are cleared on failure. IdentityKind
 * reflects the final provider state after all mappings can persist. A failure
 * can leave harmless unpublished mappings in the append-only identity map. */
enum CAMDIdentityMapResult camd_legacy_identity_set_resolve(
    struct CAMDIdentityMap *map, const char *module_path, size_t port_count,
    struct CAMDEndpointIDV1 *provider_id,
    struct CAMDEndpointIDV1 *endpoint_ids, uint32_t *identity_kind);

#endif
