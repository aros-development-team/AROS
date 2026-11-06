#ifndef CAMD_LEGACY_PORT_REFS_H
#define CAMD_LEGACY_PORT_REFS_H

/*
 * Private logical ownership state for one fixed CAMD driver port.
 *
 * Callers serialize mutations with the legacy graph lock.  The returned
 * transition tells the AROS shim when to call the physical OpenPort/ClosePort;
 * legacy links and endpoint sessions therefore share one physical open.
 */

#include "provider_contract.h"

#include <stdint.h>

enum CAMDLegacyPortRefResult {
    CAMD_LEGACY_PORT_REF_OK = 0,
    CAMD_LEGACY_PORT_REF_INVALID,
    CAMD_LEGACY_PORT_REF_STATE,
    CAMD_LEGACY_PORT_REF_OVERFLOW
};

enum CAMDLegacyPortTransition {
    CAMD_LEGACY_PORT_TRANSITION_NONE = 0,
    CAMD_LEGACY_PORT_TRANSITION_OPEN,
    CAMD_LEGACY_PORT_TRANSITION_CLOSE
};

struct CAMDLegacyPortRefs {
    uint32_t LegacyDirections;
    uint32_t EndpointInputUsers;
    uint32_t EndpointOutputUsers;
};

void camd_legacy_port_refs_init(struct CAMDLegacyPortRefs *refs);
int camd_legacy_port_refs_in_use(const struct CAMDLegacyPortRefs *refs);

enum CAMDLegacyPortRefResult camd_legacy_port_refs_set_legacy(
    struct CAMDLegacyPortRefs *refs,
    uint32_t direction,
    int present,
    enum CAMDLegacyPortTransition *transition);

enum CAMDLegacyPortRefResult camd_legacy_port_refs_acquire_endpoint(
    struct CAMDLegacyPortRefs *refs,
    uint32_t direction,
    enum CAMDLegacyPortTransition *transition);

enum CAMDLegacyPortRefResult camd_legacy_port_refs_release_endpoint(
    struct CAMDLegacyPortRefs *refs,
    uint32_t direction,
    enum CAMDLegacyPortTransition *transition);

#endif
