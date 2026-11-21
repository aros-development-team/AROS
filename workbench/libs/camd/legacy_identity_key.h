#ifndef CAMD_LEGACY_IDENTITY_KEY_H
#define CAMD_LEGACY_IDENTITY_KEY_H

#include "identity_map.h"

#define CAMD_IDENTITY_NAMESPACE_LEGACY_PROVIDER 0x4c505256u
#define CAMD_IDENTITY_NAMESPACE_LEGACY_ENDPOINT 0x4c454e44u

/* The module leaf is an AROS DEVS:Midi filename, not a display name. ASCII
 * case is folded because the DOS path is case-insensitive. One physical port
 * gets one endpoint key; input/output direction is deliberately excluded. */
enum CAMDIdentityMapResult camd_legacy_provider_identity_key(
    const char *module_path, struct CAMDIdentityKeyV1 *key);
enum CAMDIdentityMapResult camd_legacy_endpoint_identity_key(
    const char *module_path, uint32_t port_index,
    struct CAMDIdentityKeyV1 *key);

#endif
