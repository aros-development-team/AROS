/*
 * Publishes a loaded driver's fixed ports in the private endpoint registry.
 *
 * Legacy cluster traffic does not pass through these objects, and a failure
 * here leaves the driver's clusters working. Output sessions have their own
 * queue and worker; input sessions get each complete message the port's
 * receiver process parses, beside the port's legacy cluster.
 */

#include <exec/memory.h>
#include <proto/exec.h>

#include <limits.h>
#include <string.h>

#include "camd_intern.h"
#include "identity_store_aros.h"
#include "legacy_driver_adapter.h"
#include "legacy_driver_output_backend.h"
#include "legacy_identity_set.h"
#include "ump_driver_backend.h"

#  undef DEBUG
#  define DEBUG 1
#  include AROS_DEBUG_H_FILE

static void show_id(const char *label, long index,
                    const struct CAMDEndpointIDV1 *id)
{
    D(bug("camd.library: %s %ld id %08lx%08lx%08lx%08lx\n", label, index,
          (unsigned long)id->word[0], (unsigned long)id->word[1],
          (unsigned long)id->word[2], (unsigned long)id->word[3]));
}

struct ump_driver_host;
struct ump_driver_port {
    struct ump_driver_host *host;
    struct DriverData *driver_data;
    ULONG index;
};

struct ump_driver_host {
    struct MidiUMPDriverExtensionV1 *extension;
    struct CAMDUMPDriverBackend *backend;
    struct CAMDLegacyDriverAdapter *adapter;
    struct CamdBase *camd_base;
    ULONG port_count;
    BOOL attached;
    struct ump_driver_port ports[1];
};

static enum CAMDProviderResult driver_result(LONG result)
{
    if (result == CAMD_UMP_DRIVER_OK) return CAMD_PROVIDER_OK;
    if (result == CAMD_UMP_DRIVER_INVALID) return CAMD_PROVIDER_INVALID;
    if (result == CAMD_UMP_DRIVER_UNSUPPORTED) return CAMD_PROVIDER_UNSUPPORTED;
    if (result == CAMD_UMP_DRIVER_BUSY) return CAMD_PROVIDER_QUEUE_FULL;
    if (result == CAMD_UMP_DRIVER_OFFLINE) return CAMD_PROVIDER_RETIRED;
    return CAMD_PROVIDER_CALLBACK_FAILED;
}

static LONG public_result(enum CAMDProviderResult result)
{
    if (result == CAMD_PROVIDER_OK) return CAMD_UMP_DRIVER_OK;
    if (result == CAMD_PROVIDER_INVALID) return CAMD_UMP_DRIVER_INVALID;
    if (result == CAMD_PROVIDER_UNSUPPORTED) return CAMD_UMP_DRIVER_UNSUPPORTED;
    if (result == CAMD_PROVIDER_QUEUE_FULL) return CAMD_UMP_DRIVER_BUSY;
    if (result == CAMD_PROVIDER_RETIRED) return CAMD_UMP_DRIVER_OFFLINE;
    return CAMD_UMP_DRIVER_FAILED;
}

static LONG registry_driver_result(enum CAMDRegistryResult result)
{
    if (result == CAMD_REGISTRY_OK) return CAMD_UMP_DRIVER_OK;
    if (result == CAMD_REGISTRY_INVALID || result == CAMD_REGISTRY_RANGE)
        return CAMD_UMP_DRIVER_INVALID;
    if (result == CAMD_REGISTRY_UNSUPPORTED)
        return CAMD_UMP_DRIVER_UNSUPPORTED;
    if (result == CAMD_REGISTRY_BUSY || result == CAMD_REGISTRY_QUEUE_FULL)
        return CAMD_UMP_DRIVER_BUSY;
    if (result == CAMD_REGISTRY_RETIRED || result == CAMD_REGISTRY_STALE)
        return CAMD_UMP_DRIVER_OFFLINE;
    return CAMD_UMP_DRIVER_FAILED;
}

static enum CAMDProviderResult ump_acquire(void *context, uint32_t direction)
{
    struct ump_driver_port *port = context;
    return driver_result(port->host->extension->OpenPortUMP(
        port->host->extension->DriverContext, port->index, direction));
}

static enum CAMDProviderResult ump_release(void *context, uint32_t direction)
{
    struct ump_driver_port *port = context;
    port->host->extension->ClosePortUMP(
        port->host->extension->DriverContext, port->index, direction);
    return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult ump_submit(void *context,
    const struct CAMDUMPEventV1 *events, size_t count)
{
    struct ump_driver_port *port = context;
    if (count > (size_t)ULONG_MAX) return CAMD_PROVIDER_INVALID;
    return driver_result(port->host->extension->SendUMP(
        port->host->extension->DriverContext, port->index, events,
        (ULONG)count));
}

static void ump_drained(void *context)
{
    struct ump_driver_port *port = context;
    NotifyEndpointDrained(port->host->camd_base);
}

static LONG ASM ump_host_submit(REG(a0) APTR context, REG(d0) ULONG port,
    REG(a1) const struct CAMDUMPEventV1 *events, REG(d1) ULONG count,
    REG(a2) ULONG *dropped)
{
    struct ump_driver_host *host = context;
    enum CAMDProviderResult valid;
    uint32_t lost = 0;
    if (!host || port >= host->port_count || !host->backend)
        return CAMD_UMP_DRIVER_INVALID;
    valid = camd_provider_validate_ump(events, count);
    if (valid == CAMD_PROVIDER_OK)
        valid = camd_ump_driver_backend_receive(host->backend, port, events,
                                                count, &lost);
    if (dropped) *dropped = lost;
    return public_result(valid);
}

static LONG ASM ump_host_state(REG(a0) APTR context, REG(d0) ULONG port,
    REG(d1) ULONG state)
{
    struct ump_driver_host *host = context;
    if (!host || !host->adapter || port >= host->port_count)
        return CAMD_UMP_DRIVER_BUSY;
    return registry_driver_result(camd_legacy_driver_adapter_set_state(
        host->adapter, port, state));
}

static LONG ASM ump_host_topology(REG(a0) APTR context, REG(d0) ULONG port,
    REG(a1) struct CAMDGroupInfoV1 *groups, REG(d1) ULONG group_count,
    REG(a2) struct CAMDFunctionBlockInfoV1 *blocks,
    REG(d2) ULONG block_count)
{
    struct ump_driver_host *host = context;
    if (!host || !host->adapter || port >= host->port_count)
        return CAMD_UMP_DRIVER_BUSY;
    return registry_driver_result(camd_legacy_driver_adapter_set_topology(
        host->adapter, port, groups, group_count, blocks, block_count));
}

static void ASM ump_host_ready(REG(a0) APTR context, REG(d0) ULONG port)
{
    struct ump_driver_host *host = context;
    uint32_t woken;
    if (host && port < host->port_count)
        camd_native_worker_fanout_wake_all(
            host->ports[port].driver_data->EndpointOutputFanout, &woken);
}

static const struct MidiUMPDriverHostV1 ump_host_ops = {
    sizeof(struct MidiUMPDriverHostV1), 1, ump_host_submit, ump_host_state,
    ump_host_topology, ump_host_ready
};

static BOOL valid_ump_extension(struct MidiUMPDriverExtensionV1 *extension)
{
    return extension && extension->Size == sizeof(*extension) &&
           extension->Version == 1 && extension->Transport &&
           extension->Transport[0] && extension->Attach && extension->Detach &&
           extension->OpenPortUMP && extension->ClosePortUMP &&
           extension->SendUMP && extension->QueryPort;
}

static BOOL publish_ump_driver(struct Drivers *driver,
    const struct CAMDEndpointIDV1 *provider_id,
    struct CAMDEndpointIDV1 *ids, uint32_t kind,
    struct CamdBase *CamdBase)
{
    struct MidiUMPDriverExtensionV1 *extension =
        MDD_UMP_EXTENSION(driver->mididevicedata);
    struct ump_driver_host *host = NULL;
    struct CAMDUMPBackendPortV1 *backend_ports = NULL;
    struct CAMDLegacyPortDescriptorV1 *ports = NULL;
    struct MidiUMPDriverPortInfoV1 *info = NULL;
    struct CAMDUMPBackendConfigV1 backend_config;
    struct CAMDLegacyDriverDescriptorV1 descriptor;
    struct CAMDLegacyDriverAdapter *adapter = NULL;
    struct CAMDUMPDriverBackend *backend = NULL;
    enum CAMDRegistryResult published;
    ULONG count = driver->numports, i;
    ULONG protocol = 0, capabilities = 0;
    size_t host_size;
    char name[256];

    if (!valid_ump_extension(extension) ||
        strlen(extension->Transport) >= CAMD_ENDPOINT_TRANSPORT_BYTES)
        return FALSE;
    host_size = sizeof(*host) + (count - 1) * sizeof(host->ports[0]);
    host = AllocVec(host_size, MEMF_ANY | MEMF_CLEAR);
    backend_ports = AllocVec(count * sizeof(*backend_ports),
                             MEMF_ANY | MEMF_CLEAR);
    ports = AllocVec(count * sizeof(*ports), MEMF_ANY | MEMF_CLEAR);
    info = AllocVec(count * sizeof(*info), MEMF_ANY | MEMF_CLEAR);
    if (!host || !backend_ports || !ports || !info) goto fail;
    host->extension = extension;
    host->camd_base = CamdBase;
    host->port_count = count;
    for (i = 0; i < count; ++i) {
        info[i].Size = sizeof(info[i]);
        info[i].Version = 1;
        if (extension->QueryPort(extension->DriverContext, i, &info[i]) !=
                CAMD_UMP_DRIVER_OK ||
            info[i].Size != sizeof(info[i]) || info[i].Version != 1 ||
            info[i].Directions == 0 ||
            (info[i].Directions & ~CAMD_PROVIDER_DIRECTION_ALL) != 0 ||
            (info[i].State != CAMD_ENDPOINT_AVAILABLE &&
             info[i].State != CAMD_ENDPOINT_OFFLINE) ||
            (info[i].CurrentProtocol != CAMD_PROVIDER_PROTOCOL_MIDI1 &&
             info[i].CurrentProtocol != CAMD_PROVIDER_PROTOCOL_MIDI2) ||
            (info[i].ProtocolCapabilities & info[i].CurrentProtocol) == 0 ||
            (info[i].ProtocolCapabilities &
             ~(CAMD_PROVIDER_PROTOCOL_MIDI1 |
               CAMD_PROVIDER_PROTOCOL_MIDI2)) != 0 ||
            (i && info[i].CurrentProtocol != protocol) ||
            (info[i].ProductInstance &&
             strlen(info[i].ProductInstance) >= CAMD_ENDPOINT_PRODUCT_BYTES))
            goto fail;
        protocol = info[i].CurrentProtocol;
        capabilities |= info[i].ProtocolCapabilities;
        host->ports[i].host = host;
        host->ports[i].driver_data = driver->driverdatas[i];
        host->ports[i].index = i;
        backend_ports[i].Size = sizeof(backend_ports[i]);
        backend_ports[i].Version = 1;
        backend_ports[i].EndpointID = ids[i];
        backend_ports[i].Context = &host->ports[i];
        backend_ports[i].CapacityFanout =
            driver->driverdatas[i]->EndpointOutputFanout;
        ports[i].Size = sizeof(ports[i]);
        ports[i].Version = 1;
        ports[i].EndpointID = ids[i];
        ports[i].Directions = info[i].Directions;
        mysprintf(CamdBase, name, "%s.%ld", driver->mididevicedata->Name,
                  (long)i);
        name[sizeof(ports[i].Name) - 1] = '\0';
        strcpy(ports[i].Name, name);
        if (info[i].ProductInstance)
            strcpy(ports[i].ProductInstance, info[i].ProductInstance);
    }
    memset(&backend_config, 0, sizeof(backend_config));
    backend_config.Size = sizeof(backend_config);
    backend_config.Version = 1;
    backend_config.Callbacks.Size = sizeof(backend_config.Callbacks);
    backend_config.Callbacks.Version = 1;
    backend_config.Callbacks.Acquire = ump_acquire;
    backend_config.Callbacks.Release = ump_release;
    backend_config.Callbacks.Submit = ump_submit;
    backend_config.Callbacks.Drained = ump_drained;
    backend_config.Ports = backend_ports;
    backend_config.PortCount = count;
    backend_config.Protocol = protocol;
    backend_config.MaxQueueCapacity = ENDPOINT_OUTPUT_QUEUE;
    backend_config.WorkerItemBudget = ENDPOINT_OUTPUT_BUDGET;
    if (camd_ump_driver_backend_create(&backend_config, &backend) !=
        CAMD_PROVIDER_OK) goto fail;
    host->backend = backend;
    if (extension->Attach(extension->DriverContext, &ump_host_ops, host) !=
        CAMD_UMP_DRIVER_OK) goto fail;
    host->attached = TRUE;
    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.Size = sizeof(descriptor);
    descriptor.Version = 1;
    descriptor.ProviderID = *provider_id;
    descriptor.IdentityKind = kind;
    descriptor.ProtocolCapabilities = capabilities;
    descriptor.BackendContext = camd_ump_driver_backend_context(backend);
    descriptor.BackendOps = camd_ump_driver_backend_ops(backend);
    descriptor.Ports = ports;
    descriptor.PortCount = count;
    descriptor.Transport = extension->Transport;
    descriptor.DataFormat = CAMD_PROVIDER_FORMAT_UMP;
    descriptor.Protocol = protocol;
    published = camd_legacy_driver_adapter_create(
        CB(CamdBase)->EndpointRegistry, &descriptor, &adapter);
    if (published != CAMD_REGISTRY_OK) goto fail;
    host->adapter = adapter;
    for (i = 0; i < count; ++i) {
        if ((info[i].GroupCount || info[i].FunctionBlockCount) &&
            camd_legacy_driver_adapter_set_topology(
                adapter, i, info[i].Groups, info[i].GroupCount,
                info[i].FunctionBlocks, info[i].FunctionBlockCount) !=
                    CAMD_REGISTRY_OK)
            D(bug("camd.library: %s.%ld native topology refused\n",
                  driver->mididevicedata->Name, (long)i));
        if (info[i].State == CAMD_ENDPOINT_OFFLINE &&
            camd_legacy_driver_adapter_set_state(adapter, i,
                CAMD_ENDPOINT_OFFLINE) != CAMD_REGISTRY_OK)
            D(bug("camd.library: %s.%ld native state refused\n",
                  driver->mididevicedata->Name, (long)i));
    }
    driver->EndpointAdapter = adapter;
    driver->EndpointUMPBackend = backend;
    driver->EndpointUMPHost = host;
    FreeVec(info); FreeVec(ports); FreeVec(backend_ports);
    D(bug("camd.library: %s published %ld native UMP endpoints\n",
          driver->mididevicedata->Name, (long)count));
    return TRUE;

fail:
    if (host && host->attached) extension->Detach(extension->DriverContext);
    if (backend) camd_ump_driver_backend_destroy(backend);
    FreeVec(info); FreeVec(ports); FreeVec(backend_ports); FreeVec(host);
    return FALSE;
}

/* Must not hold CLSemaphore. */
void PublishDriverEndpoints(struct Drivers *driver, char *path,
                            struct CamdBase *CamdBase)
{
    struct CAMDIdentityMap *map;
    struct CAMDEndpointIDV1 provider_id, *ids;
    struct CAMDLegacyDriverOutputPortV1 *outputs = NULL;
    struct CAMDLegacyPortDescriptorV1 *ports = NULL;
    struct CAMDLegacyDriverOutputConfigV1 config;
    struct CAMDLegacyDriverDescriptorV1 descriptor;
    struct CAMDLegacyDriverOutputBackend *backend = NULL;
    struct CAMDLegacyDriverAdapter *adapter = NULL;
    enum CAMDIdentityMapResult resolved;
    enum CAMDRegistryResult published;
    uint32_t kind;
    ULONG count = driver->numports, i;
    char name[256];

    /* Without the identity store nothing is published automatically. */
    map = camd_identity_store_aros_map(CB(CamdBase)->EndpointIdentity);
    if (!map || count == 0)
        return;

    ids = AllocVec(count * sizeof(*ids), MEMF_ANY | MEMF_CLEAR);
    if (!ids)
        goto done;

    resolved = camd_legacy_identity_set_resolve(map, path, count,
                                                &provider_id, ids, &kind);
    if (resolved != CAMD_IDENTITY_MAP_OK) {
        /* The store is full or cannot take them: IDs for this run. */
        resolved = camd_identity_map_temporary(map, &provider_id);
        for (i = 0; i < count && resolved == CAMD_IDENTITY_MAP_OK; ++i)
            resolved = camd_identity_map_temporary(map, &ids[i]);
        kind = CAMD_IDENTITY_EPHEMERAL;
    }
    if (resolved != CAMD_IDENTITY_MAP_OK) {
        D(bug("camd.library: %s has no endpoint identity (%ld)\n",
              driver->mididevicedata->Name, (long)resolved));
        goto done;
    }

    if ((driver->mididevicedata->Flags & MDD_F_UMP_EXTENSION) != 0 &&
        publish_ump_driver(driver, &provider_id, ids, kind, CamdBase)) {
        show_id("provider", 0, &provider_id);
        for (i = 0; i < count; ++i) show_id("endpoint", (long)i, &ids[i]);
        goto done;
    }

    outputs = AllocVec(count * sizeof(*outputs), MEMF_ANY | MEMF_CLEAR);
    ports = AllocVec(count * sizeof(*ports), MEMF_ANY | MEMF_CLEAR);
    if (!outputs || !ports) goto done;

    for (i = 0; i < count; ++i) {
        outputs[i].Size = sizeof(outputs[i]);
        outputs[i].Version = 1;
        outputs[i].EndpointID = ids[i];
        outputs[i].DriverData = driver->driverdatas[i];

        ports[i].Size = sizeof(ports[i]);
        ports[i].Version = 1;
        ports[i].EndpointID = ids[i];
        ports[i].Directions = CAMD_PROVIDER_DIRECTION_ALL;
        mysprintf(CamdBase, name, "%s.%ld", driver->mididevicedata->Name,
                  (long)i);
        name[sizeof(ports[i].Name) - 1] = '\0';
        strcpy(ports[i].Name, name);
    }

    memset(&config, 0, sizeof(config));
    config.Size = sizeof(config);
    config.Version = 1;
    config.CamdBase = CamdBase;
    config.Ports = outputs;
    config.PortCount = count;
    config.MaxQueueCapacity = ENDPOINT_OUTPUT_QUEUE;
    config.MaxSysExBytes = ENDPOINT_OUTPUT_SYSEX;
    config.LimitSysExBytes = ENDPOINT_SYSEX_LIMIT;
    config.WorkerItemBudget = ENDPOINT_OUTPUT_BUDGET;
    if (camd_legacy_driver_output_backend_create(&config, &backend) !=
        CAMD_PROVIDER_OK)
        goto done;

    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.Size = sizeof(descriptor);
    descriptor.Version = 1;
    descriptor.ProviderID = provider_id;
    descriptor.IdentityKind = kind;
    descriptor.ProtocolCapabilities = CAMD_PROVIDER_PROTOCOL_MIDI1;
    descriptor.BackendContext =
        camd_legacy_driver_output_backend_context(backend);
    descriptor.BackendOps = camd_legacy_driver_output_backend_ops(backend);
    descriptor.Ports = ports;
    descriptor.PortCount = count;
    published = camd_legacy_driver_adapter_create(
        CB(CamdBase)->EndpointRegistry, &descriptor, &adapter);

    /* A failed create can hand back an adapter that is still pinned. Keep it
     * and its backend so that UnpublishDriverEndpoints() finishes the
     * rollback. */
    driver->EndpointAdapter = adapter;
    driver->EndpointBackend = backend;
    if (published != CAMD_REGISTRY_OK) {
        D(bug("camd.library: %s endpoints not published (%ld)\n",
              driver->mididevicedata->Name, (long)published));
        UnpublishDriverEndpoints(driver, CamdBase);
        goto done;
    }

    /* From here on the receiver processes deliver to input sessions. */
    for (i = 0; i < count; ++i) {
        driver->driverdatas[i]->EndpointSysExLength = 0;
        driver->driverdatas[i]->EndpointBackend = backend;
    }

    D(bug("camd.library: %s published %ld endpoints, identity kind %ld\n",
          driver->mididevicedata->Name, (long)count, (long)kind));
    show_id("provider", 0, &provider_id);
    for (i = 0; i < count; ++i)
        show_id("endpoint", (long)i, &ids[i]);

done:
    FreeVec(ports);
    FreeVec(outputs);
    FreeVec(ids);
}

/* Returns FALSE while an endpoint client still pins the provider or one of
 * its sessions; the DriverData the backend refers to must then stay
 * allocated. Nothing opens a private session yet, so today this only
 * happens if the registry itself fails. Must not hold CLSemaphore. */
BOOL UnpublishDriverEndpoints(struct Drivers *driver,
                              struct CamdBase *CamdBase)
{
    struct ump_driver_host *ump_host = driver->EndpointUMPHost;

    (void)CamdBase;

    if (driver->EndpointAdapter != NULL) {
        camd_legacy_driver_adapter_begin_retire(driver->EndpointAdapter);
        if (camd_legacy_driver_adapter_release(driver->EndpointAdapter) !=
            CAMD_REGISTRY_OK) {
            D(bug("camd.library: %s endpoints are still in use\n",
                  driver->mididevicedata->Name));
            return FALSE;
        }
        driver->EndpointAdapter = NULL;
        if (ump_host) ump_host->adapter = NULL;
    }
    if (driver->EndpointUMPBackend != NULL) {
        if (ump_host && ump_host->attached) {
            ump_host->extension->Detach(ump_host->extension->DriverContext);
            ump_host->attached = FALSE;
        }
        if (camd_ump_driver_backend_destroy(driver->EndpointUMPBackend) !=
            CAMD_PROVIDER_OK)
            return FALSE;
        driver->EndpointUMPBackend = NULL;
        driver->EndpointUMPHost = NULL;
        FreeVec(ump_host);
    }
    if (driver->EndpointBackend != NULL) {
        ULONG i;

        /* No session is left, so every port is closed for endpoints and its
         * receiver process delivers nothing any more. */
        for (i = 0; driver->driverdatas != NULL && i < driver->numports; ++i) {
            if (driver->driverdatas[i] != NULL)
                driver->driverdatas[i]->EndpointBackend = NULL;
        }
        if (camd_legacy_driver_output_backend_destroy(
                driver->EndpointBackend) != CAMD_PROVIDER_OK) {
            D(bug("camd.library: %s output sessions are still open\n",
                  driver->mididevicedata->Name));
            return FALSE;
        }
        driver->EndpointBackend = NULL;
    }
    return TRUE;
}

/* Called by the port's receiver process for each complete short message. */
void DeliverDriverEndpointMidi(struct DriverData *driverdata,
                               struct MyMidiMessage2 *msg2)
{
    struct CAMDMIDI1EventV1 event;
    uint32_t dropped = 0;

    if (msg2->len < 1 || msg2->len > 3)
        return;
    memset(&event, 0, sizeof(event));
    event.Size = sizeof(event);
    event.Version = 1;
    event.Length = msg2->len;
    event.Bytes[0] = msg2->status;
    event.Bytes[1] = msg2->data1;
    event.Bytes[2] = msg2->data2;
    camd_legacy_driver_output_backend_receive_midi1(
        driverdata->EndpointBackend, (size_t)driverdata->portnum, &event,
        &dropped);
    driverdata->EndpointInputDropped += dropped;
}

/* Called by the port's receiver process for each SysEx byte: 0xF0 starts a
 * message, 0xF7 ends it and 0xFF tells that the driver broke it off. Input
 * sessions get complete messages only. */
void DeliverDriverEndpointSysEx(struct DriverData *driverdata, UBYTE data)
{
    uint32_t dropped = 0;

    /* The buffer comes with a port's first SysEx message: most ports never
     * receive one. Without memory this port delivers no SysEx. */
    if (driverdata->EndpointSysEx == NULL && data == 0xf0)
        driverdata->EndpointSysEx = AllocVec(ENDPOINT_SYSEX_LIMIT, MEMF_ANY);
    if (driverdata->EndpointSysEx == NULL)
        return;
    if (data == 0xf0) {
        driverdata->EndpointSysExLength = 0;
        driverdata->EndpointSysExOverflow = FALSE;
    } else if (driverdata->EndpointSysExLength == 0) {
        return;
    }
    if (data == 0xff) {
        driverdata->EndpointSysExLength = 0;
        return;
    }
    if (driverdata->EndpointSysExLength < ENDPOINT_SYSEX_LIMIT)
        driverdata->EndpointSysEx[driverdata->EndpointSysExLength++] = data;
    else
        driverdata->EndpointSysExOverflow = TRUE;
    if (data == 0xf7) {
        if (driverdata->EndpointSysExOverflow) {
            ++driverdata->EndpointInputDropped;
        } else {
            camd_legacy_driver_output_backend_receive_sysex(
                driverdata->EndpointBackend, (size_t)driverdata->portnum,
                driverdata->EndpointSysEx, driverdata->EndpointSysExLength,
                &dropped);
            driverdata->EndpointInputDropped += dropped;
        }
        driverdata->EndpointSysExLength = 0;
    }
}
