#ifndef CAMD_LEGACY_OUTPUT_BACKEND_H
#define CAMD_LEGACY_OUTPUT_BACKEND_H

/*
 * Private bounded output backend for fixed-port MIDI 1.0 drivers.
 *
 * Each provider session owns one native MIDI 1.0 queue, pump and worker. The
 * physical port owns the fan-out used to retry blocked workers when driver
 * capacity is released. No MIDI 1.0 event is converted to UMP.
 */

#include "native_worker_fanout.h"

#include <stddef.h>
#include <stdint.h>

typedef enum CAMDProviderResult (*CAMDLegacyOutputPortFnV1)(void *context);
typedef enum CAMDProviderResult (*CAMDLegacyOutputMIDI1FnV1)(
    void *context, const struct CAMDMIDI1EventV1 *event);
typedef enum CAMDProviderResult (*CAMDLegacyOutputSysExFnV1)(
    void *context, const uint8_t *bytes, size_t byte_count,
    uint32_t time_high, uint32_t time_low, uint32_t clock_domain,
    uint32_t flags);

struct CAMDLegacyOutputCallbacksV1 {
    uint32_t Size;
    uint32_t Version;
    CAMDLegacyOutputPortFnV1 Acquire;
    CAMDLegacyOutputPortFnV1 Release;
    CAMDLegacyOutputMIDI1FnV1 SubmitMIDI1;
    CAMDLegacyOutputSysExFnV1 SubmitMIDI1SysEx;
};

struct CAMDLegacyOutputPortV1 {
    uint32_t Size;
    uint32_t Version;
    struct CAMDEndpointIDV1 EndpointID;
    void *Context;
    struct CAMDNativeWorkerFanout *CapacityFanout;
};

struct CAMDLegacyOutputBackendConfigV1 {
    uint32_t Size;
    uint32_t Version;
    struct CAMDLegacyOutputCallbacksV1 Callbacks;
    const struct CAMDLegacyOutputPortV1 *Ports;
    size_t PortCount;
    uint32_t MaxQueueCapacity;
    uint32_t MaxSysExBytes;
    uint32_t WorkerItemBudget;
};

struct CAMDLegacyOutputBackend;

enum CAMDProviderResult camd_legacy_output_backend_create(
    const struct CAMDLegacyOutputBackendConfigV1 *config,
    struct CAMDLegacyOutputBackend **backend);

/* The returned table and provider context remain owned by the backend. */
const struct CAMDProviderOpsV1 *camd_legacy_output_backend_ops(
    struct CAMDLegacyOutputBackend *backend);
void *camd_legacy_output_backend_context(
    struct CAMDLegacyOutputBackend *backend);

/* Fails with STATE while an open or live session still owns the backend. */
enum CAMDProviderResult camd_legacy_output_backend_destroy(
    struct CAMDLegacyOutputBackend *backend);

#endif
