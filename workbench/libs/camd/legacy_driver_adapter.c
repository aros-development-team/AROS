#include "legacy_driver_adapter.h"

#include <limits.h>
#include <string.h>

#ifdef __AROS__
#include <exec/memory.h>
#include <proto/exec.h>
#else
#include <stdlib.h>
#endif

struct legacy_port {
    struct CAMDLegacyPortDescriptorV1 descriptor;
    struct CAMDHandleV1 owner_lease;
    int owner_lease_live;
};

struct CAMDLegacyDriverAdapter {
    struct CAMDEndpointRegistry *registry;
    struct CAMDHandleV1 provider;
    struct CAMDLegacyDriverDescriptorV1 descriptor;
    struct CAMDProviderOpsV1 backend_ops;
    struct CAMDProviderOpsV1 provider_ops;
    struct legacy_port *ports;
    size_t port_count;
    int retiring;
};

static void *adapter_alloc(size_t size, int clear)
{
#ifdef __AROS__
    ULONG flags = MEMF_ANY | (clear ? MEMF_CLEAR : 0);

    if (size > (size_t)ULONG_MAX)
        return NULL;
    return AllocVec((ULONG)size, flags);
#else
    return clear ? calloc(1, size) : malloc(size);
#endif
}

static void adapter_free(void *pointer)
{
#ifdef __AROS__
    if (pointer)
        FreeVec(pointer);
#else
    free(pointer);
#endif
}

static int id_is_zero(const struct CAMDEndpointIDV1 *id)
{
    return id->word[0] == 0 && id->word[1] == 0 &&
           id->word[2] == 0 && id->word[3] == 0;
}

static int id_equal(const struct CAMDEndpointIDV1 *left,
                    const struct CAMDEndpointIDV1 *right)
{
    return memcmp(left, right, sizeof(*left)) == 0;
}

static const struct CAMDLegacyPortDescriptorV1 *find_port(
    const struct CAMDLegacyDriverAdapter *adapter,
    const struct CAMDEndpointIDV1 *endpoint_id)
{
    size_t i;

    for (i = 0; i < adapter->port_count; ++i) {
        if (id_equal(&adapter->ports[i].descriptor.EndpointID, endpoint_id))
            return &adapter->ports[i].descriptor;
    }
    return NULL;
}

static enum CAMDProviderResult adapter_open(
    void *context, const struct CAMDProviderOpenRequestV1 *request,
    struct CAMDProviderOpenResultV1 *result)
{
    struct CAMDLegacyDriverAdapter *adapter = context;
    const struct CAMDLegacyPortDescriptorV1 *port;

    if (!adapter || !request || !result)
        return CAMD_PROVIDER_INVALID;
    port = find_port(adapter, &request->EndpointID);
    if (!port)
        return CAMD_PROVIDER_INVALID;
    if (request->DataFormat != CAMD_PROVIDER_FORMAT_MIDI1 ||
        request->Protocol != CAMD_PROVIDER_PROTOCOL_MIDI1 ||
        request->Direction == 0 ||
        (request->Direction & ~port->Directions) != 0)
        return CAMD_PROVIDER_UNSUPPORTED;
    return adapter->backend_ops.Open(adapter->descriptor.BackendContext,
                                     request, result);
}

static enum CAMDProviderResult adapter_close(void *context,
                                              void *session_context)
{
    struct CAMDLegacyDriverAdapter *adapter = context;

    return adapter->backend_ops.Close(adapter->descriptor.BackendContext,
                                      session_context);
}

static enum CAMDProviderResult adapter_send_midi1(
    void *context, void *session_context,
    const struct CAMDMIDI1EventV1 *events, size_t event_count)
{
    struct CAMDLegacyDriverAdapter *adapter = context;

    return adapter->backend_ops.SendMIDI1(adapter->descriptor.BackendContext,
                                          session_context, events,
                                          event_count);
}

static enum CAMDProviderResult adapter_send_midi1_sysex(
    void *context, void *session_context, const uint8_t *bytes,
    size_t byte_count, uint32_t time_high, uint32_t time_low,
    uint32_t clock_domain, uint32_t flags)
{
    struct CAMDLegacyDriverAdapter *adapter = context;

    return adapter->backend_ops.SendMIDI1SysEx(
        adapter->descriptor.BackendContext, session_context, bytes,
        byte_count, time_high, time_low, clock_domain, flags);
}

static enum CAMDProviderResult adapter_start_receive(
    void *context, void *session_context,
    const struct CAMDProviderReceiveSinkV1 *sink)
{
    struct CAMDLegacyDriverAdapter *adapter = context;

    return adapter->backend_ops.StartReceive(
        adapter->descriptor.BackendContext, session_context, sink);
}

static enum CAMDProviderResult adapter_stop_receive(void *context,
                                                     void *session_context)
{
    struct CAMDLegacyDriverAdapter *adapter = context;

    return adapter->backend_ops.StopReceive(adapter->descriptor.BackendContext,
                                            session_context);
}

static enum CAMDProviderResult adapter_drain(void *context,
                                             void *session_context)
{
    struct CAMDLegacyDriverAdapter *adapter = context;

    return adapter->backend_ops.Drain(adapter->descriptor.BackendContext,
                                      session_context);
}

static enum CAMDProviderResult adapter_cancel(void *context,
                                              void *session_context)
{
    struct CAMDLegacyDriverAdapter *adapter = context;

    return adapter->backend_ops.Cancel(adapter->descriptor.BackendContext,
                                       session_context);
}

static enum CAMDProviderResult adapter_begin_shutdown(void *context)
{
    struct CAMDLegacyDriverAdapter *adapter = context;

    if (!adapter->backend_ops.BeginShutdown)
        return CAMD_PROVIDER_OK;
    return adapter->backend_ops.BeginShutdown(
        adapter->descriptor.BackendContext);
}

static int adapter_shutdown_ready(void *context)
{
    struct CAMDLegacyDriverAdapter *adapter = context;

    if (!adapter->backend_ops.ShutdownReady)
        return 1;
    return adapter->backend_ops.ShutdownReady(
        adapter->descriptor.BackendContext);
}

static int validate_descriptor(
    const struct CAMDLegacyDriverDescriptorV1 *descriptor)
{
    const struct CAMDProviderOpsV1 *ops;
    uint32_t directions = 0;
    size_t i, j;

    if (!descriptor || descriptor->Size != sizeof(*descriptor) ||
        descriptor->Version != 1 || id_is_zero(&descriptor->ProviderID) ||
        !descriptor->BackendOps || !descriptor->Ports ||
        descriptor->PortCount == 0 ||
        descriptor->PortCount > SIZE_MAX / sizeof(struct legacy_port))
        return 0;
    ops = descriptor->BackendOps;
    if (ops->Size != sizeof(*ops) || ops->Version != 1 || !ops->Open ||
        !ops->Close)
        return 0;
    for (i = 0; i < descriptor->PortCount; ++i) {
        const struct CAMDLegacyPortDescriptorV1 *port = &descriptor->Ports[i];

        if (port->Size != sizeof(*port) || port->Version != 1 ||
            id_is_zero(&port->EndpointID) || port->Directions == 0 ||
            (port->Directions & ~CAMD_PROVIDER_DIRECTION_ALL) != 0 ||
            !memchr(port->Name, '\0', sizeof(port->Name)) ||
            !memchr(port->ProductInstance, '\0',
                    sizeof(port->ProductInstance)))
            return 0;
        directions |= port->Directions;
        for (j = 0; j < i; ++j) {
            if (id_equal(&port->EndpointID,
                         &descriptor->Ports[j].EndpointID))
                return 0;
        }
    }
    if ((directions & CAMD_PROVIDER_DIRECTION_OUTPUT) != 0 &&
        (!ops->SendMIDI1 || !ops->SendMIDI1SysEx || !ops->Drain ||
         !ops->Cancel))
        return 0;
    if ((directions & CAMD_PROVIDER_DIRECTION_INPUT) != 0 &&
        (!ops->StartReceive || !ops->StopReceive))
        return 0;
    return 1;
}

static void initialize_provider_ops(struct CAMDLegacyDriverAdapter *adapter)
{
    struct CAMDProviderOpsV1 *ops = &adapter->provider_ops;

    ops->Size = sizeof(*ops);
    ops->Version = 1;
    ops->Open = adapter_open;
    ops->Close = adapter_close;
    ops->SendMIDI1 = adapter_send_midi1;
    ops->SendMIDI1SysEx = adapter_send_midi1_sysex;
    ops->StartReceive = adapter_start_receive;
    ops->StopReceive = adapter_stop_receive;
    ops->Drain = adapter_drain;
    ops->Cancel = adapter_cancel;
    ops->BeginShutdown = adapter_begin_shutdown;
    ops->ShutdownReady = adapter_shutdown_ready;
}

static enum CAMDRegistryResult rollback_create(
    struct CAMDLegacyDriverAdapter *adapter)
{
    enum CAMDRegistryResult result, begin_result;
    size_t i;

    begin_result = camd_registry_provider_begin_retire(adapter->registry,
                                                        adapter->provider);
    if (begin_result != CAMD_REGISTRY_OK &&
        begin_result != CAMD_REGISTRY_CALLBACK_FAILED)
        return begin_result;
    adapter->retiring = 1;
    for (i = 0; i < adapter->port_count; ++i) {
        if (adapter->ports[i].owner_lease_live) {
            result = camd_registry_release(
                adapter->registry, adapter->ports[i].owner_lease);
            if (result != CAMD_REGISTRY_OK)
                return result;
            adapter->ports[i].owner_lease_live = 0;
        }
    }
    return camd_registry_provider_release(adapter->registry,
                                          adapter->provider);
}

enum CAMDRegistryResult camd_legacy_driver_adapter_create(
    struct CAMDEndpointRegistry *registry,
    const struct CAMDLegacyDriverDescriptorV1 *descriptor,
    struct CAMDLegacyDriverAdapter **adapter_out)
{
    struct CAMDLegacyDriverAdapter *adapter;
    struct CAMDProviderDescriptorV1 provider;
    enum CAMDRegistryResult result;
    uint32_t directions = 0;
    size_t i;

    if (!adapter_out)
        return CAMD_REGISTRY_INVALID;
    *adapter_out = NULL;
    if (!registry || !validate_descriptor(descriptor))
        return CAMD_REGISTRY_INVALID;
    adapter = adapter_alloc(sizeof(*adapter), 1);
    if (!adapter)
        return CAMD_REGISTRY_NOMEM;
    adapter->ports = adapter_alloc(
        descriptor->PortCount * sizeof(*adapter->ports), 1);
    if (!adapter->ports) {
        adapter_free(adapter);
        return CAMD_REGISTRY_NOMEM;
    }
    adapter->registry = registry;
    adapter->descriptor = *descriptor;
    adapter->backend_ops = *descriptor->BackendOps;
    adapter->descriptor.BackendOps = &adapter->backend_ops;
    adapter->descriptor.Ports = NULL;
    adapter->port_count = descriptor->PortCount;
    for (i = 0; i < adapter->port_count; ++i) {
        adapter->ports[i].descriptor = descriptor->Ports[i];
        directions |= descriptor->Ports[i].Directions;
    }
    initialize_provider_ops(adapter);
    memset(&provider, 0, sizeof(provider));
    provider.Size = sizeof(provider);
    provider.Version = 1;
    provider.ProviderID = descriptor->ProviderID;
    provider.NativePaths = CAMD_PROVIDER_PATH_MIDI1;
    provider.Directions = directions;
    provider.Context = adapter;
    provider.Ops = &adapter->provider_ops;
    result = camd_registry_provider_register(registry, &provider,
                                             &adapter->provider);
    if (result != CAMD_REGISTRY_OK)
        goto fail;

    for (i = 0; i < adapter->port_count; ++i) {
        struct CAMDEndpointInfoV1 endpoint;

        memset(&endpoint, 0, sizeof(endpoint));
        endpoint.Size = sizeof(endpoint);
        endpoint.Version = 1;
        endpoint.ID = adapter->ports[i].descriptor.EndpointID;
        endpoint.ProviderID = descriptor->ProviderID;
        endpoint.State = CAMD_ENDPOINT_AVAILABLE;
        endpoint.Flags = adapter->ports[i].descriptor.Flags;
        endpoint.IdentityKind = descriptor->IdentityKind;
        endpoint.NativeDataFormats = CAMD_DATA_FORMAT_MIDI1;
        endpoint.ProtocolCapabilities = descriptor->ProtocolCapabilities;
        endpoint.CurrentProtocol = CAMD_PROVIDER_PROTOCOL_MIDI1;
        memcpy(endpoint.Name, adapter->ports[i].descriptor.Name,
               sizeof(endpoint.Name));
        memcpy(endpoint.ProductInstance,
               adapter->ports[i].descriptor.ProductInstance,
               sizeof(endpoint.ProductInstance));
        memcpy(endpoint.Transport, "camd-legacy", sizeof("camd-legacy"));
        result = camd_registry_publish(registry, adapter->provider, &endpoint,
                                       NULL, 0, NULL, 0,
                                       &adapter->ports[i].owner_lease);
        if (result != CAMD_REGISTRY_OK) {
            enum CAMDRegistryResult rollback = rollback_create(adapter);

            if (rollback != CAMD_REGISTRY_OK) {
                *adapter_out = adapter;
                return rollback;
            }
            goto fail;
        }
        adapter->ports[i].owner_lease_live = 1;
    }
    *adapter_out = adapter;
    return CAMD_REGISTRY_OK;

fail:
    adapter_free(adapter->ports);
    adapter_free(adapter);
    return result;
}

enum CAMDRegistryResult camd_legacy_driver_adapter_begin_retire(
    struct CAMDLegacyDriverAdapter *adapter)
{
    enum CAMDRegistryResult result;

    if (!adapter)
        return CAMD_REGISTRY_INVALID;
    if (adapter->retiring)
        return CAMD_REGISTRY_RETIRED;
    result = camd_registry_provider_begin_retire(adapter->registry,
                                                  adapter->provider);
    if (result == CAMD_REGISTRY_OK ||
        result == CAMD_REGISTRY_CALLBACK_FAILED)
        adapter->retiring = 1;
    return result;
}

enum CAMDRegistryResult camd_legacy_driver_adapter_release(
    struct CAMDLegacyDriverAdapter *adapter)
{
    enum CAMDRegistryResult result;
    size_t i;

    if (!adapter)
        return CAMD_REGISTRY_INVALID;
    if (!adapter->retiring)
        return CAMD_REGISTRY_STATE;
    for (i = 0; i < adapter->port_count; ++i) {
        if (adapter->ports[i].owner_lease_live) {
            result = camd_registry_release(
                adapter->registry, adapter->ports[i].owner_lease);
            if (result != CAMD_REGISTRY_OK)
                return result;
            adapter->ports[i].owner_lease_live = 0;
        }
    }
    result = camd_registry_provider_release(adapter->registry,
                                            adapter->provider);
    if (result != CAMD_REGISTRY_OK)
        return result;
    adapter_free(adapter->ports);
    adapter_free(adapter);
    return CAMD_REGISTRY_OK;
}
