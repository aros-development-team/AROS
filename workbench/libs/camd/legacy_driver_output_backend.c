#include "camd_intern.h"
#include "legacy_driver_output_backend.h"

#include <limits.h>
#include <string.h>

#include <exec/memory.h>
#include <proto/exec.h>

struct driver_output_port {
    struct CamdBase *camd_base;
    struct DriverData *driver_data;
};

struct CAMDLegacyDriverOutputBackend {
    struct CAMDLegacyOutputBackend *backend;
    struct driver_output_port *ports;
    size_t port_count;
};

static void *binding_alloc(size_t size)
{
    if (size > (size_t)ULONG_MAX)
        return NULL;
    return AllocVec((ULONG)size, MEMF_ANY | MEMF_CLEAR);
}

static void binding_free(void *pointer)
{
    if (pointer)
        FreeVec(pointer);
}

static enum CAMDProviderResult acquire_output(void *context)
{
    struct driver_output_port *port = context;
    enum CAMDProviderResult result;

    ObtainSemaphore(CB(port->camd_base)->CLSemaphore);
    result = AcquireDriverEndpointPort(
        port->driver_data, CAMD_PROVIDER_DIRECTION_OUTPUT, port->camd_base);
    ReleaseSemaphore(CB(port->camd_base)->CLSemaphore);
    return result;
}

static enum CAMDProviderResult release_output(void *context)
{
    struct driver_output_port *port = context;
    enum CAMDProviderResult result;

    ObtainSemaphore(CB(port->camd_base)->CLSemaphore);
    result = ReleaseDriverEndpointPort(
        port->driver_data, CAMD_PROVIDER_DIRECTION_OUTPUT, port->camd_base);
    ReleaseSemaphore(CB(port->camd_base)->CLSemaphore);
    return result;
}

static enum CAMDProviderResult acquire_input(void *context)
{
    struct driver_output_port *port = context;
    enum CAMDProviderResult result;

    ObtainSemaphore(CB(port->camd_base)->CLSemaphore);
    result = AcquireDriverEndpointPort(
        port->driver_data, CAMD_PROVIDER_DIRECTION_INPUT, port->camd_base);
    ReleaseSemaphore(CB(port->camd_base)->CLSemaphore);
    return result;
}

static enum CAMDProviderResult release_input(void *context)
{
    struct driver_output_port *port = context;
    enum CAMDProviderResult result;

    ObtainSemaphore(CB(port->camd_base)->CLSemaphore);
    result = ReleaseDriverEndpointPort(
        port->driver_data, CAMD_PROVIDER_DIRECTION_INPUT, port->camd_base);
    ReleaseSemaphore(CB(port->camd_base)->CLSemaphore);
    return result;
}

static enum CAMDProviderResult submit_midi1(
    void *context, const struct CAMDMIDI1EventV1 *event)
{
    struct driver_output_port *port = context;
    ULONG message;

    if (!event || event->Length == 0 || event->Length > 3)
        return CAMD_PROVIDER_INVALID;
    message = (ULONG)event->Bytes[0] << 24;
    if (event->Length > 1)
        message |= (ULONG)event->Bytes[1] << 16;
    if (event->Length > 2)
        message |= (ULONG)event->Bytes[2] << 8;
    return Midi2Driver_internal(port->driver_data, message,
                                OUTBUFFERSIZE - 1)
               ? CAMD_PROVIDER_OK
               : CAMD_PROVIDER_QUEUE_FULL;
}

static enum CAMDProviderResult submit_sysex(
    void *context, const uint8_t *bytes, size_t byte_count,
    uint32_t time_high, uint32_t time_low, uint32_t clock_domain,
    uint32_t flags)
{
    struct driver_output_port *port = context;
    struct DriverData *driver_data = port->driver_data;

    (void)time_high;
    (void)time_low;
    (void)clock_domain;
    (void)flags;
    if (!bytes || byte_count < 2 || byte_count > (size_t)ULONG_MAX)
        return CAMD_PROVIDER_INVALID;
    /* The legacy driver borrows buffer_sx until the final F7 is transmitted.
     * Keep the pump's checkout buffer stable by holding this callback and the
     * existing SysEx semaphore for exactly that interval. AttemptSemaphore()
     * turns another in-flight SysEx into bounded backpressure instead of
     * blocking a session worker behind it. */
    if (!AttemptSemaphore(&driver_data->sysexsemaphore))
        return CAMD_PROVIDER_QUEUE_FULL;
    driver_data->buffer_sx = (UBYTE *)bytes;
    driver_data->buffercurrsend_sx = 0;
    driver_data->issending_sx = 1;
    if (!SysEx2Driver(driver_data, (UBYTE *)bytes)) {
        driver_data->issending_sx = 0;
        driver_data->buffer_sx = NULL;
        ReleaseSemaphore(&driver_data->sysexsemaphore);
        return CAMD_PROVIDER_QUEUE_FULL;
    }
    while (driver_data->issending_sx != 0)
        CamdWait();
    driver_data->buffer_sx = NULL;
    ReleaseSemaphore(&driver_data->sysexsemaphore);
    return CAMD_PROVIDER_OK;
}

static int valid_config(
    const struct CAMDLegacyDriverOutputConfigV1 *config)
{
    size_t i;

    if (!config || config->Size != sizeof(*config) ||
        config->Version != 1 || !config->CamdBase || !config->Ports ||
        config->PortCount == 0 ||
        config->PortCount > SIZE_MAX / sizeof(struct driver_output_port) ||
        config->PortCount > SIZE_MAX / sizeof(struct CAMDLegacyOutputPortV1) ||
        config->MaxQueueCapacity == 0 || config->MaxSysExBytes < 2 ||
        config->WorkerItemBudget == 0)
        return 0;
    for (i = 0; i < config->PortCount; ++i) {
        if (config->Ports[i].Size != sizeof(config->Ports[i]) ||
            config->Ports[i].Version != 1 || !config->Ports[i].DriverData ||
            !config->Ports[i].DriverData->EndpointOutputFanout)
            return 0;
    }
    return 1;
}

enum CAMDProviderResult camd_legacy_driver_output_backend_create(
    const struct CAMDLegacyDriverOutputConfigV1 *config,
    struct CAMDLegacyDriverOutputBackend **backend_out)
{
    struct CAMDLegacyDriverOutputBackend *binding;
    struct CAMDLegacyOutputBackendConfigV1 backend_config;
    struct CAMDLegacyOutputPortV1 *ports;
    enum CAMDProviderResult result;
    size_t i;

    if (!backend_out)
        return CAMD_PROVIDER_INVALID;
    *backend_out = NULL;
    if (!valid_config(config))
        return CAMD_PROVIDER_INVALID;
    binding = binding_alloc(sizeof(*binding));
    if (!binding)
        return CAMD_PROVIDER_CALLBACK_FAILED;
    binding->ports = binding_alloc(config->PortCount * sizeof(*binding->ports));
    ports = binding_alloc(config->PortCount * sizeof(*ports));
    if (!binding->ports || !ports) {
        binding_free(ports);
        binding_free(binding->ports);
        binding_free(binding);
        return CAMD_PROVIDER_CALLBACK_FAILED;
    }
    binding->port_count = config->PortCount;
    for (i = 0; i < config->PortCount; ++i) {
        binding->ports[i].camd_base = config->CamdBase;
        binding->ports[i].driver_data = config->Ports[i].DriverData;
        ports[i].Size = sizeof(ports[i]);
        ports[i].Version = 1;
        ports[i].EndpointID = config->Ports[i].EndpointID;
        ports[i].Context = &binding->ports[i];
        ports[i].CapacityFanout =
            config->Ports[i].DriverData->EndpointOutputFanout;
    }
    memset(&backend_config, 0, sizeof(backend_config));
    backend_config.Size = sizeof(backend_config);
    backend_config.Version = 1;
    backend_config.Callbacks.Size = sizeof(backend_config.Callbacks);
    backend_config.Callbacks.Version = 1;
    backend_config.Callbacks.Acquire = acquire_output;
    backend_config.Callbacks.Release = release_output;
    backend_config.Callbacks.SubmitMIDI1 = submit_midi1;
    backend_config.Callbacks.SubmitMIDI1SysEx = submit_sysex;
    backend_config.Callbacks.AcquireInput = acquire_input;
    backend_config.Callbacks.ReleaseInput = release_input;
    backend_config.Ports = ports;
    backend_config.PortCount = config->PortCount;
    backend_config.MaxQueueCapacity = config->MaxQueueCapacity;
    backend_config.MaxSysExBytes = config->MaxSysExBytes;
    backend_config.WorkerItemBudget = config->WorkerItemBudget;
    result = camd_legacy_output_backend_create(&backend_config,
                                                &binding->backend);
    binding_free(ports);
    if (result != CAMD_PROVIDER_OK) {
        binding_free(binding->ports);
        binding_free(binding);
        return result;
    }
    *backend_out = binding;
    return CAMD_PROVIDER_OK;
}

const struct CAMDProviderOpsV1 *camd_legacy_driver_output_backend_ops(
    struct CAMDLegacyDriverOutputBackend *backend)
{
    return backend ? camd_legacy_output_backend_ops(backend->backend) : NULL;
}

void *camd_legacy_driver_output_backend_context(
    struct CAMDLegacyDriverOutputBackend *backend)
{
    return backend ? camd_legacy_output_backend_context(backend->backend)
                   : NULL;
}

enum CAMDProviderResult camd_legacy_driver_output_backend_receive_midi1(
    struct CAMDLegacyDriverOutputBackend *backend, size_t port_index,
    const struct CAMDMIDI1EventV1 *event, uint32_t *dropped)
{
    if (!backend)
        return CAMD_PROVIDER_INVALID;
    return camd_legacy_output_backend_receive_midi1(backend->backend,
                                                    port_index, event,
                                                    dropped);
}

enum CAMDProviderResult camd_legacy_driver_output_backend_receive_sysex(
    struct CAMDLegacyDriverOutputBackend *backend, size_t port_index,
    const uint8_t *bytes, size_t byte_count, uint32_t *dropped)
{
    if (!backend)
        return CAMD_PROVIDER_INVALID;
    return camd_legacy_output_backend_receive_sysex(backend->backend,
                                                    port_index, bytes,
                                                    byte_count, dropped);
}

enum CAMDProviderResult camd_legacy_driver_output_backend_destroy(
    struct CAMDLegacyDriverOutputBackend *backend)
{
    enum CAMDProviderResult result;

    if (!backend)
        return CAMD_PROVIDER_INVALID;
    result = camd_legacy_output_backend_destroy(backend->backend);
    if (result != CAMD_PROVIDER_OK)
        return result;
    binding_free(backend->ports);
    binding_free(backend);
    return CAMD_PROVIDER_OK;
}
