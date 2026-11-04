#include "legacy_port_refs.h"

#include <limits.h>
#include <string.h>

static int valid_direction(uint32_t direction)
{
    return direction == CAMD_PROVIDER_DIRECTION_INPUT ||
           direction == CAMD_PROVIDER_DIRECTION_OUTPUT;
}

void camd_legacy_port_refs_init(struct CAMDLegacyPortRefs *refs)
{
    if (refs)
        memset(refs, 0, sizeof(*refs));
}

int camd_legacy_port_refs_in_use(const struct CAMDLegacyPortRefs *refs)
{
    return refs && (refs->LegacyDirections != 0 ||
                    refs->EndpointInputUsers != 0 ||
                    refs->EndpointOutputUsers != 0);
}

enum CAMDLegacyPortRefResult camd_legacy_port_refs_set_legacy(
    struct CAMDLegacyPortRefs *refs,
    uint32_t direction,
    int present,
    enum CAMDLegacyPortTransition *transition)
{
    int was_in_use;

    if (!refs || !transition || !valid_direction(direction))
        return CAMD_LEGACY_PORT_REF_INVALID;
    *transition = CAMD_LEGACY_PORT_TRANSITION_NONE;
    present = present != 0;
    if (((refs->LegacyDirections & direction) != 0) == present)
        return CAMD_LEGACY_PORT_REF_OK;
    was_in_use = camd_legacy_port_refs_in_use(refs);
    if (present)
        refs->LegacyDirections |= direction;
    else
        refs->LegacyDirections &= ~direction;
    if (!was_in_use)
        *transition = CAMD_LEGACY_PORT_TRANSITION_OPEN;
    else if (!camd_legacy_port_refs_in_use(refs))
        *transition = CAMD_LEGACY_PORT_TRANSITION_CLOSE;
    return CAMD_LEGACY_PORT_REF_OK;
}

static uint32_t *endpoint_counter(struct CAMDLegacyPortRefs *refs,
                                  uint32_t direction)
{
    return direction == CAMD_PROVIDER_DIRECTION_INPUT
               ? &refs->EndpointInputUsers
               : &refs->EndpointOutputUsers;
}

enum CAMDLegacyPortRefResult camd_legacy_port_refs_acquire_endpoint(
    struct CAMDLegacyPortRefs *refs,
    uint32_t direction,
    enum CAMDLegacyPortTransition *transition)
{
    uint32_t *counter;
    int was_in_use;

    if (!refs || !transition || !valid_direction(direction))
        return CAMD_LEGACY_PORT_REF_INVALID;
    *transition = CAMD_LEGACY_PORT_TRANSITION_NONE;
    counter = endpoint_counter(refs, direction);
    if (*counter == UINT32_MAX)
        return CAMD_LEGACY_PORT_REF_OVERFLOW;
    was_in_use = camd_legacy_port_refs_in_use(refs);
    ++*counter;
    if (!was_in_use)
        *transition = CAMD_LEGACY_PORT_TRANSITION_OPEN;
    return CAMD_LEGACY_PORT_REF_OK;
}

enum CAMDLegacyPortRefResult camd_legacy_port_refs_release_endpoint(
    struct CAMDLegacyPortRefs *refs,
    uint32_t direction,
    enum CAMDLegacyPortTransition *transition)
{
    uint32_t *counter;

    if (!refs || !transition || !valid_direction(direction))
        return CAMD_LEGACY_PORT_REF_INVALID;
    *transition = CAMD_LEGACY_PORT_TRANSITION_NONE;
    counter = endpoint_counter(refs, direction);
    if (*counter == 0)
        return CAMD_LEGACY_PORT_REF_STATE;
    --*counter;
    if (!camd_legacy_port_refs_in_use(refs))
        *transition = CAMD_LEGACY_PORT_TRANSITION_CLOSE;
    return CAMD_LEGACY_PORT_REF_OK;
}
