#ifndef CAMD_UMP_DRIVER_BACKEND_H
#define CAMD_UMP_DRIVER_BACKEND_H

#include "native_worker_fanout.h"

typedef enum CAMDProviderResult (*CAMDUMPPortFnV1)(void *context,
                                                    uint32_t direction);
typedef enum CAMDProviderResult (*CAMDUMPSubmitFnV1)(
    void *context, const struct CAMDUMPEventV1 *events, size_t count);

struct CAMDUMPBackendCallbacksV1 {
    uint32_t Size;
    uint32_t Version;
    CAMDUMPPortFnV1 Acquire;
    CAMDUMPPortFnV1 Release;
    CAMDUMPSubmitFnV1 Submit;
    void (*Drained)(void *context);
};

struct CAMDUMPBackendPortV1 {
    uint32_t Size;
    uint32_t Version;
    struct CAMDEndpointIDV1 EndpointID;
    void *Context;
    struct CAMDNativeWorkerFanout *CapacityFanout;
};

struct CAMDUMPBackendConfigV1 {
    uint32_t Size;
    uint32_t Version;
    struct CAMDUMPBackendCallbacksV1 Callbacks;
    const struct CAMDUMPBackendPortV1 *Ports;
    size_t PortCount;
    uint32_t Protocol;
    uint32_t MaxQueueCapacity;
    uint32_t WorkerItemBudget;
};

struct CAMDUMPDriverBackend;

enum CAMDProviderResult camd_ump_driver_backend_create(
    const struct CAMDUMPBackendConfigV1 *config,
    struct CAMDUMPDriverBackend **backend);
const struct CAMDProviderOpsV1 *camd_ump_driver_backend_ops(
    struct CAMDUMPDriverBackend *backend);
void *camd_ump_driver_backend_context(struct CAMDUMPDriverBackend *backend);
enum CAMDProviderResult camd_ump_driver_backend_receive(
    struct CAMDUMPDriverBackend *backend, size_t port,
    const struct CAMDUMPEventV1 *events, size_t count, uint32_t *dropped);
enum CAMDProviderResult camd_ump_driver_backend_destroy(
    struct CAMDUMPDriverBackend *backend);

#endif
