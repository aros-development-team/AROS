/*
 * Publishes a loaded driver's fixed ports in the private endpoint registry.
 *
 * Legacy cluster traffic does not pass through these objects, and a failure
 * here leaves the driver's clusters working. Only output is published until
 * the input handoff exists.
 */

#include <exec/memory.h>
#include <proto/exec.h>

#include <string.h>

#include "camd_intern.h"
#include "identity_store_aros.h"
#include "legacy_driver_adapter.h"
#include "legacy_driver_output_backend.h"
#include "legacy_identity_set.h"

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

/* Must not hold CLSemaphore. */
void PublishDriverEndpoints(struct Drivers *driver, char *path,
                            struct CamdBase *CamdBase)
{
    struct CAMDIdentityMap *map;
    struct CAMDEndpointIDV1 provider_id, *ids;
    struct CAMDLegacyDriverOutputPortV1 *outputs;
    struct CAMDLegacyPortDescriptorV1 *ports;
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
    outputs = AllocVec(count * sizeof(*outputs), MEMF_ANY | MEMF_CLEAR);
    ports = AllocVec(count * sizeof(*ports), MEMF_ANY | MEMF_CLEAR);
    if (!ids || !outputs || !ports)
        goto done;

    resolved = camd_legacy_identity_set_resolve(map, path, count,
                                                &provider_id, ids, &kind);
    if (resolved != CAMD_IDENTITY_MAP_OK) {
        D(bug("camd.library: %s has no endpoint identity (%ld)\n",
              driver->mididevicedata->Name, (long)resolved));
        goto done;
    }

    for (i = 0; i < count; ++i) {
        outputs[i].Size = sizeof(outputs[i]);
        outputs[i].Version = 1;
        outputs[i].EndpointID = ids[i];
        outputs[i].DriverData = driver->driverdatas[i];

        ports[i].Size = sizeof(ports[i]);
        ports[i].Version = 1;
        ports[i].EndpointID = ids[i];
        ports[i].Directions = CAMD_PROVIDER_DIRECTION_OUTPUT;
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
    }
    if (driver->EndpointBackend != NULL) {
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
