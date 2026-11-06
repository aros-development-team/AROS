#ifndef CAMD_LEGACY_DRIVER_OUTPUT_BACKEND_H
#define CAMD_LEGACY_DRIVER_OUTPUT_BACKEND_H

/* Private AROS binding from the MIDI 1.0 driver backend to DriverData. */

#include "legacy_output_backend.h"

struct CamdBase;
struct DriverData;

struct CAMDLegacyDriverOutputPortV1 {
    uint32_t Size;
    uint32_t Version;
    struct CAMDEndpointIDV1 EndpointID;
    struct DriverData *DriverData;
};

struct CAMDLegacyDriverOutputConfigV1 {
    uint32_t Size;
    uint32_t Version;
    struct CamdBase *CamdBase;
    const struct CAMDLegacyDriverOutputPortV1 *Ports;
    size_t PortCount;
    uint32_t MaxQueueCapacity;
    uint32_t MaxSysExBytes;
    uint32_t WorkerItemBudget;
    uint32_t LimitSysExBytes;
};

struct CAMDLegacyDriverOutputBackend;

enum CAMDProviderResult camd_legacy_driver_output_backend_create(
    const struct CAMDLegacyDriverOutputConfigV1 *config,
    struct CAMDLegacyDriverOutputBackend **backend);

const struct CAMDProviderOpsV1 *camd_legacy_driver_output_backend_ops(
    struct CAMDLegacyDriverOutputBackend *backend);
void *camd_legacy_driver_output_backend_context(
    struct CAMDLegacyDriverOutputBackend *backend);

/* What port port_index received, from its receiver process. */
enum CAMDProviderResult camd_legacy_driver_output_backend_receive_midi1(
    struct CAMDLegacyDriverOutputBackend *backend, size_t port_index,
    const struct CAMDMIDI1EventV1 *event, uint32_t *dropped);
enum CAMDProviderResult camd_legacy_driver_output_backend_receive_sysex(
    struct CAMDLegacyDriverOutputBackend *backend, size_t port_index,
    const uint8_t *bytes, size_t byte_count, uint32_t *dropped);

/* Retryable: returns STATE while provider sessions still own the backend. */
enum CAMDProviderResult camd_legacy_driver_output_backend_destroy(
    struct CAMDLegacyDriverOutputBackend *backend);

#endif
