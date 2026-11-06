#ifndef CAMD_LEGACY_DRIVER_ADAPTER_H
#define CAMD_LEGACY_DRIVER_ADAPTER_H

/*
 * Private adapter from fixed-port MIDI 1.0 backends to the endpoint core:
 * CAMD's legacy drivers and the endpoints programs publish.
 *
 * Existing CAMD 41/42 cluster traffic does not pass through this object.  The
 * adapter publishes the same physical ports for endpoint clients and forwards
 * only native MIDI 1.0 provider calls to a backend shim owned by camd.library.
 */

#include "provider_contract.h"

#include <stddef.h>
#include <stdint.h>

struct CAMDLegacyPortDescriptorV1 {
    uint32_t Size;
    uint32_t Version;
    struct CAMDEndpointIDV1 EndpointID;
    uint32_t Directions;
    uint32_t Flags;
    char Name[CAMD_ENDPOINT_NAME_BYTES];
    char ProductInstance[CAMD_ENDPOINT_PRODUCT_BYTES];
};

struct CAMDLegacyDriverDescriptorV1 {
    uint32_t Size;
    uint32_t Version;
    struct CAMDEndpointIDV1 ProviderID;
    uint32_t IdentityKind;
    uint32_t ProtocolCapabilities;
    void *BackendContext;
    const struct CAMDProviderOpsV1 *BackendOps;
    const struct CAMDLegacyPortDescriptorV1 *Ports;
    size_t PortCount;
    /* NULL for "camd-legacy"; at most CAMD_ENDPOINT_TRANSPORT_BYTES - 1
     * characters, copied. */
    const char *Transport;
    /* What every port carries. Both 0 for native MIDI 1.0. */
    uint32_t DataFormat;
    uint32_t Protocol;
};

struct CAMDLegacyDriverAdapter;

/* On an ordinary validation/publication failure, *adapter remains NULL.  If
 * concurrent users pin a partially published provider before rollback can
 * finish, create returns an error with a non-NULL adapter.  The caller must
 * retry begin_retire if needed, then release after those users drain. */
enum CAMDRegistryResult camd_legacy_driver_adapter_create(
    struct CAMDEndpointRegistry *registry,
    const struct CAMDLegacyDriverDescriptorV1 *descriptor,
    struct CAMDLegacyDriverAdapter **adapter);

/* CAMD_ENDPOINT_AVAILABLE or CAMD_ENDPOINT_OFFLINE for one published port. */
enum CAMDRegistryResult camd_legacy_driver_adapter_set_state(
    struct CAMDLegacyDriverAdapter *adapter, size_t port_index,
    uint32_t state);

/* Replaces one port's Groups and Function Blocks as a whole. EndpointID in
 * the records is filled in here. */
enum CAMDRegistryResult camd_legacy_driver_adapter_set_topology(
    struct CAMDLegacyDriverAdapter *adapter, size_t port_index,
    struct CAMDGroupInfoV1 *groups, size_t group_count,
    struct CAMDFunctionBlockInfoV1 *blocks, size_t block_count);

enum CAMDRegistryResult camd_legacy_driver_adapter_begin_retire(
    struct CAMDLegacyDriverAdapter *adapter);

/* Release is retryable.  It returns BUSY while endpoint clients or sessions
 * still pin the retiring provider, and frees the adapter only on success.
 * A failing BeginShutdown still puts the registry in retiring state, so the
 * caller may release after recording the callback failure. */
enum CAMDRegistryResult camd_legacy_driver_adapter_release(
    struct CAMDLegacyDriverAdapter *adapter);

#endif
