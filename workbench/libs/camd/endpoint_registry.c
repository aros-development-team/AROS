#include "endpoint_registry.h"
#include "provider_contract.h"

#include <string.h>

#ifdef __AROS__
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <proto/exec.h>

typedef struct SignalSemaphore core_lock_t;

static void *core_alloc(size_t size, int clear)
{
    return AllocVec(size, MEMF_ANY | (clear ? MEMF_CLEAR : 0));
}

static void core_free(void *memory)
{
    FreeVec(memory);
}
#else
#include <pthread.h>
#include <stdlib.h>

typedef pthread_mutex_t core_lock_t;

static void *core_alloc(size_t size, int clear)
{
    return clear ? calloc(1, size) : malloc(size);
}

static void core_free(void *memory)
{
    free(memory);
}
#endif

struct endpoint_slot {
    uint32_t generation;
    uint32_t references;
    int occupied;
    uint32_t provider_slot;
    uint32_t provider_generation;
    struct CAMDEndpointInfoV1 endpoint;
    struct CAMDGroupInfoV1 *groups;
    size_t group_count;
    struct CAMDFunctionBlockInfoV1 *blocks;
    size_t block_count;
};

struct provider_slot {
    uint32_t generation;
    uint32_t endpoint_count;
    uint32_t session_count;
    uint32_t callbacks_inflight;
    int occupied;
    int retiring;
    int shutdown_callback_done;
    int release_checking;
    struct CAMDPrivateProvider provider;
};

struct receive_bridge {
    struct CAMDEndpointRegistry *registry;
    struct CAMDHandleV1 session;
    struct CAMDProviderReceiveSinkV1 consumer;
    struct CAMDProviderReceiveSinkV1 provider_sink;
};

struct session_slot {
    uint32_t generation;
    uint32_t callbacks_inflight;
    uint32_t provider_slot;
    uint32_t provider_generation;
    uint32_t endpoint_slot;
    uint32_t endpoint_generation;
    uint32_t direction;
    uint32_t data_format;
    uint32_t protocol;
    uint32_t requested_queue_capacity;
    uint32_t effective_queue_capacity;
    uint32_t max_sysex_bytes;
    void *provider_context;
    struct receive_bridge *receive_bridge;
    uint32_t receive_inflight;
    int occupied;
    int opening;
    int closing;
    int receive_starting;
    int receive_stopping;
    int receive_stop_confirmed;
    int receive_started;
};

struct CAMDEndpointRegistry {
    core_lock_t lock;
    struct endpoint_slot *slots;
    size_t slot_count;
    struct provider_slot *providers;
    size_t provider_count;
    struct session_slot *sessions;
    size_t session_count;
    struct CAMDGenerationV1 generation;
    struct CAMDEndpointWatch *watches;
};

#ifdef __AROS__
static int core_lock_init(core_lock_t *lock)
{
    InitSemaphore(lock);
    return 1;
}

static void core_lock_destroy(core_lock_t *lock)
{
    (void)lock;
}

static void core_lock_acquire(core_lock_t *lock)
{
    ObtainSemaphore(lock);
}

static void core_lock_release(core_lock_t *lock)
{
    ReleaseSemaphore(lock);
}
#else
static int core_lock_init(core_lock_t *lock)
{
    return pthread_mutex_init(lock, NULL) == 0;
}

static void core_lock_destroy(core_lock_t *lock)
{
    pthread_mutex_destroy(lock);
}

static void core_lock_acquire(core_lock_t *lock)
{
    pthread_mutex_lock(lock);
}

static void core_lock_release(core_lock_t *lock)
{
    pthread_mutex_unlock(lock);
}
#endif

struct snapshot_endpoint {
    struct CAMDEndpointInfoV1 endpoint;
    struct CAMDGroupInfoV1 *groups;
    size_t group_count;
    struct CAMDFunctionBlockInfoV1 *blocks;
    size_t block_count;
};

struct CAMDEndpointSnapshot {
    struct CAMDGenerationV1 generation;
    struct snapshot_endpoint *endpoints;
    size_t endpoint_count;
};

struct CAMDEndpointWatch {
    struct CAMDEndpointWatch *next;
    struct CAMDEndpointRegistry *registry;
    struct CAMDEndpointWatchEventV1 *events;
    size_t capacity;
    size_t head;
    size_t count;
    int lost;
    struct CAMDGenerationV1 lost_generation;
};

typedef char endpoint_size_must_be_360[
    sizeof(struct CAMDEndpointInfoV1) == 360 ? 1 : -1];
typedef char group_size_must_be_100[
    sizeof(struct CAMDGroupInfoV1) == 100 ? 1 : -1];
typedef char block_size_must_be_104[
    sizeof(struct CAMDFunctionBlockInfoV1) == 104 ? 1 : -1];
typedef char handle_size_must_be_8[
    sizeof(struct CAMDHandleV1) == 8 ? 1 : -1];
typedef char watch_event_size_must_be_36[
    sizeof(struct CAMDEndpointWatchEventV1) == 36 ? 1 : -1];

static int id_is_zero(const struct CAMDEndpointIDV1 *id)
{
    return !(id->word[0] | id->word[1] | id->word[2] | id->word[3]);
}

static int id_equal(const struct CAMDEndpointIDV1 *a,
                    const struct CAMDEndpointIDV1 *b)
{
    return memcmp(a, b, sizeof(*a)) == 0;
}

static void advance_generation(struct CAMDEndpointRegistry *registry)
{
    if (++registry->generation.low == 0)
        ++registry->generation.high;
}

static int generation_available(const struct CAMDEndpointRegistry *registry)
{
    return registry->generation.high != UINT32_MAX ||
           registry->generation.low != UINT32_MAX;
}

/* Called only with registry->lock held. */
static void notify_watches(struct CAMDEndpointRegistry *registry,
                           uint32_t type,
                           const struct CAMDEndpointIDV1 *id)
{
    struct CAMDEndpointWatch *watch;

    for (watch = registry->watches; watch; watch = watch->next) {
        struct CAMDEndpointWatchEventV1 *event;
        size_t tail;

        if (watch->lost || watch->count == watch->capacity) {
            watch->lost = 1;
            watch->lost_generation = registry->generation;
            continue;
        }
        tail = (watch->head + watch->count) % watch->capacity;
        event = &watch->events[tail];
        memset(event, 0, sizeof(*event));
        event->Size = sizeof(*event);
        event->Version = 1;
        event->Generation = registry->generation;
        event->EndpointID = *id;
        event->Type = type;
        ++watch->count;
    }
}

static void free_topology(struct endpoint_slot *slot)
{
    core_free(slot->groups);
    core_free(slot->blocks);
    slot->groups = NULL;
    slot->blocks = NULL;
    slot->group_count = 0;
    slot->block_count = 0;
}

static enum CAMDRegistryResult validate_endpoint(
    const struct CAMDEndpointInfoV1 *endpoint)
{
    if (!endpoint || endpoint->Size != sizeof(*endpoint) ||
        endpoint->Version != 1 || id_is_zero(&endpoint->ID) ||
        id_is_zero(&endpoint->ProviderID) ||
        endpoint->NativeDataFormats == 0 ||
        (endpoint->NativeDataFormats & ~CAMD_DATA_FORMAT_ALL) != 0 ||
        endpoint->State < CAMD_ENDPOINT_REGISTERED ||
        endpoint->State > CAMD_ENDPOINT_OFFLINE)
        return CAMD_REGISTRY_INVALID;
    if (!memchr(endpoint->Name, '\0', sizeof(endpoint->Name)) ||
        !memchr(endpoint->ProductInstance, '\0',
                sizeof(endpoint->ProductInstance)) ||
        !memchr(endpoint->Transport, '\0', sizeof(endpoint->Transport)))
        return CAMD_REGISTRY_INVALID;
    return CAMD_REGISTRY_OK;
}

static enum CAMDRegistryResult copy_topology(
    const struct CAMDEndpointInfoV1 *endpoint,
    const struct CAMDGroupInfoV1 *groups,
    size_t group_count,
    const struct CAMDFunctionBlockInfoV1 *blocks,
    size_t block_count,
    struct CAMDGroupInfoV1 **group_copy,
    struct CAMDFunctionBlockInfoV1 **block_copy)
{
    size_t i, j;

    *group_copy = NULL;
    *block_copy = NULL;
    if ((group_count && !groups) || (block_count && !blocks))
        return CAMD_REGISTRY_INVALID;
    if (group_count > SIZE_MAX / sizeof(*groups) ||
        block_count > SIZE_MAX / sizeof(*blocks))
        return CAMD_REGISTRY_RANGE;

    for (i = 0; i < group_count; ++i) {
        if (groups[i].Size != sizeof(groups[i]) || groups[i].Version != 1 ||
            groups[i].Group > 15 ||
            !id_equal(&groups[i].EndpointID, &endpoint->ID) ||
            !memchr(groups[i].Name, '\0', sizeof(groups[i].Name)))
            return CAMD_REGISTRY_INVALID;
        for (j = 0; j < i; ++j)
            if (groups[j].Group == groups[i].Group)
                return CAMD_REGISTRY_DUPLICATE;
    }

    for (i = 0; i < block_count; ++i) {
        if (blocks[i].Size != sizeof(blocks[i]) || blocks[i].Version != 1 ||
            blocks[i].FirstGroup > 15 || blocks[i].GroupCount == 0 ||
            blocks[i].GroupCount > 16 - blocks[i].FirstGroup ||
            !id_equal(&blocks[i].EndpointID, &endpoint->ID) ||
            !memchr(blocks[i].Name, '\0', sizeof(blocks[i].Name)))
            return CAMD_REGISTRY_INVALID;
        for (j = 0; j < i; ++j)
            if (blocks[j].Number == blocks[i].Number)
                return CAMD_REGISTRY_DUPLICATE;
    }

    if (group_count) {
        *group_copy = core_alloc(group_count * sizeof(**group_copy), 0);
        if (!*group_copy)
            return CAMD_REGISTRY_NOMEM;
        memcpy(*group_copy, groups, group_count * sizeof(**group_copy));
    }
    if (block_count) {
        *block_copy = core_alloc(block_count * sizeof(**block_copy), 0);
        if (!*block_copy) {
            core_free(*group_copy);
            *group_copy = NULL;
            return CAMD_REGISTRY_NOMEM;
        }
        memcpy(*block_copy, blocks, block_count * sizeof(**block_copy));
    }
    return CAMD_REGISTRY_OK;
}

static struct endpoint_slot *resolve(struct CAMDEndpointRegistry *registry,
                                     struct CAMDHandleV1 handle)
{
    struct endpoint_slot *slot;

    if (!registry || handle.slot == 0 || handle.slot > registry->slot_count)
        return NULL;
    slot = &registry->slots[handle.slot - 1];
    if (!slot->occupied || slot->generation != handle.generation)
        return NULL;
    return slot;
}

static struct provider_slot *resolve_provider(
    struct CAMDEndpointRegistry *registry, struct CAMDHandleV1 handle)
{
    struct provider_slot *slot;

    if (!registry || handle.slot == 0 || handle.slot > registry->provider_count)
        return NULL;
    slot = &registry->providers[handle.slot - 1];
    if (!slot->occupied || slot->generation != handle.generation)
        return NULL;
    return slot;
}

static struct session_slot *resolve_session(
    struct CAMDEndpointRegistry *registry, struct CAMDHandleV1 handle)
{
    struct session_slot *slot;

    if (!registry || handle.slot == 0 || handle.slot > registry->session_count)
        return NULL;
    slot = &registry->sessions[handle.slot - 1];
    if (!slot->occupied || slot->generation != handle.generation)
        return NULL;
    return slot;
}

static enum CAMDRegistryResult provider_result(
    enum CAMDProviderResult result)
{
    switch (result) {
    case CAMD_PROVIDER_OK:
        return CAMD_REGISTRY_OK;
    case CAMD_PROVIDER_INVALID:
        return CAMD_REGISTRY_INVALID;
    case CAMD_PROVIDER_UNSUPPORTED:
        return CAMD_REGISTRY_UNSUPPORTED;
    case CAMD_PROVIDER_STATE:
        return CAMD_REGISTRY_STATE;
    case CAMD_PROVIDER_RETIRED:
        return CAMD_REGISTRY_RETIRED;
    case CAMD_PROVIDER_QUEUE_FULL:
        return CAMD_REGISTRY_QUEUE_FULL;
    case CAMD_PROVIDER_TOO_LARGE:
        return CAMD_REGISTRY_TOO_LARGE;
    default:
        return CAMD_REGISTRY_CALLBACK_FAILED;
    }
}

/* Called only with registry->lock held. */
static void release_endpoint_reference(struct CAMDEndpointRegistry *registry,
                                       struct endpoint_slot *endpoint)
{
    --endpoint->references;
    if (endpoint->references == 0 &&
        endpoint->endpoint.State == CAMD_ENDPOINT_RETIRING) {
        struct CAMDHandleV1 owner;
        struct provider_slot *provider;

        owner.slot = endpoint->provider_slot;
        owner.generation = endpoint->provider_generation;
        provider = resolve_provider(registry, owner);
        if (provider && provider->endpoint_count != 0)
            --provider->endpoint_count;
        free_topology(endpoint);
        memset(&endpoint->endpoint, 0, sizeof(endpoint->endpoint));
        endpoint->occupied = 0;
    }
}

/* Called only with registry->lock held and no callback in flight. */
static void release_session(struct CAMDEndpointRegistry *registry,
                            struct session_slot *session)
{
    struct CAMDHandleV1 handle;
    struct provider_slot *provider;
    struct endpoint_slot *endpoint;
    uint32_t generation = session->generation;

    handle.slot = session->provider_slot;
    handle.generation = session->provider_generation;
    provider = resolve_provider(registry, handle);
    handle.slot = session->endpoint_slot;
    handle.generation = session->endpoint_generation;
    endpoint = resolve(registry, handle);
    if (provider && provider->session_count != 0)
        --provider->session_count;
    if (endpoint && endpoint->references != 0)
        release_endpoint_reference(registry, endpoint);
    core_free(session->receive_bridge);
    memset(session, 0, sizeof(*session));
    session->generation = generation;
}

static int transition_allowed(uint32_t from, uint32_t to)
{
    if (to == CAMD_ENDPOINT_RETIRING)
        return from != CAMD_ENDPOINT_RETIRING && from != CAMD_ENDPOINT_RETIRED;
    switch (from) {
    case CAMD_ENDPOINT_REGISTERED:
        return to == CAMD_ENDPOINT_DISCOVERING || to == CAMD_ENDPOINT_OFFLINE;
    case CAMD_ENDPOINT_DISCOVERING:
        return to == CAMD_ENDPOINT_AVAILABLE || to == CAMD_ENDPOINT_OFFLINE;
    case CAMD_ENDPOINT_AVAILABLE:
        return to == CAMD_ENDPOINT_OFFLINE;
    case CAMD_ENDPOINT_OFFLINE:
        return to == CAMD_ENDPOINT_DISCOVERING || to == CAMD_ENDPOINT_AVAILABLE;
    default:
        return 0;
    }
}

struct CAMDEndpointRegistry *camd_registry_create(void)
{
    struct CAMDEndpointRegistry *registry =
        core_alloc(sizeof(struct CAMDEndpointRegistry), 1);

    if (registry && !core_lock_init(&registry->lock)) {
        core_free(registry);
        registry = NULL;
    }
    return registry;
}

void camd_registry_destroy(struct CAMDEndpointRegistry *registry)
{
    struct CAMDEndpointWatch *watch, *next;
    size_t i;

    if (!registry)
        return;
    for (i = 0; i < registry->slot_count; ++i)
        free_topology(&registry->slots[i]);
    for (watch = registry->watches; watch; watch = next) {
        next = watch->next;
        core_free(watch->events);
        core_free(watch);
    }
    for (i = 0; i < registry->session_count; ++i)
        core_free(registry->sessions[i].receive_bridge);
    core_free(registry->slots);
    core_free(registry->providers);
    core_free(registry->sessions);
    core_lock_destroy(&registry->lock);
    core_free(registry);
}

enum CAMDRegistryResult camd_registry_provider_register(
    struct CAMDEndpointRegistry *registry,
    const struct CAMDProviderDescriptorV1 *descriptor,
    struct CAMDHandleV1 *provider_handle)
{
    struct CAMDPrivateProvider provider;
    struct provider_slot *slot = NULL;
    struct provider_slot *grown;
    enum CAMDRegistryResult result;
    size_t i, index = 0;

    if (!registry || !provider_handle ||
        camd_provider_init(&provider, descriptor) != CAMD_PROVIDER_OK)
        return CAMD_REGISTRY_INVALID;
    core_lock_acquire(&registry->lock);
    for (i = 0; i < registry->provider_count; ++i) {
        if (registry->providers[i].occupied &&
            id_equal(&registry->providers[i].provider.descriptor.ProviderID,
                     &provider.descriptor.ProviderID)) {
            result = CAMD_REGISTRY_DUPLICATE;
            goto out;
        }
        if (!slot && !registry->providers[i].occupied &&
            registry->providers[i].generation != UINT32_MAX) {
            slot = &registry->providers[i];
            index = i;
        }
    }
    if (!slot) {
        if (registry->provider_count == UINT32_MAX ||
            registry->provider_count >= SIZE_MAX / sizeof(*grown)) {
            result = CAMD_REGISTRY_RANGE;
            goto out;
        }
        index = registry->provider_count;
        grown = core_alloc((registry->provider_count + 1) * sizeof(*grown), 1);
        if (!grown) {
            result = CAMD_REGISTRY_NOMEM;
            goto out;
        }
        if (registry->provider_count)
            memcpy(grown, registry->providers,
                   registry->provider_count * sizeof(*grown));
        for (i = 0; i < registry->provider_count; ++i)
            if (grown[i].occupied)
                grown[i].provider.descriptor.Ops =
                    &grown[i].provider.operations;
        core_free(registry->providers);
        registry->providers = grown;
        slot = &registry->providers[index];
        ++registry->provider_count;
    }
    {
        uint32_t generation = slot->generation + 1;
        memset(slot, 0, sizeof(*slot));
        slot->generation = generation;
    }
    slot->occupied = 1;
    slot->provider = provider;
    slot->provider.descriptor.Ops = &slot->provider.operations;
    provider_handle->slot = (uint32_t)(index + 1);
    provider_handle->generation = slot->generation;
    result = CAMD_REGISTRY_OK;
out:
    core_lock_release(&registry->lock);
    return result;
}

enum CAMDRegistryResult camd_registry_provider_begin_retire(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 provider_handle)
{
    struct provider_slot *provider;
    CAMDProviderShutdownFnV1 shutdown;
    void *context;
    enum CAMDProviderResult callback_result;
    enum CAMDRegistryResult result;
    size_t i;
    int changed = 0;

    if (!registry)
        return CAMD_REGISTRY_INVALID;
    core_lock_acquire(&registry->lock);
    provider = resolve_provider(registry, provider_handle);
    if (!provider) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    if (provider->retiring) {
        result = CAMD_REGISTRY_RETIRED;
        goto out;
    }
    if (provider->callbacks_inflight != 0) {
        result = CAMD_REGISTRY_BUSY;
        goto out;
    }
    for (i = 0; i < registry->slot_count; ++i) {
        struct endpoint_slot *endpoint = &registry->slots[i];

        if (endpoint->occupied &&
            endpoint->provider_slot == provider_handle.slot &&
            endpoint->provider_generation == provider_handle.generation &&
            endpoint->endpoint.State != CAMD_ENDPOINT_RETIRING) {
            changed = 1;
            break;
        }
    }
    if (changed && !generation_available(registry)) {
        result = CAMD_REGISTRY_RANGE;
        goto out;
    }
    provider->retiring = 1;
    provider->provider.retiring = 1;
    if (changed)
        advance_generation(registry);
    for (i = 0; i < registry->slot_count; ++i) {
        struct endpoint_slot *endpoint = &registry->slots[i];

        if (endpoint->occupied &&
            endpoint->provider_slot == provider_handle.slot &&
            endpoint->provider_generation == provider_handle.generation &&
            endpoint->endpoint.State != CAMD_ENDPOINT_RETIRING) {
            endpoint->endpoint.State = CAMD_ENDPOINT_RETIRING;
            endpoint->endpoint.Generation = registry->generation;
            notify_watches(registry, CAMD_ENDPOINT_EVENT_RETIRED,
                           &endpoint->endpoint.ID);
        }
    }
    ++provider->callbacks_inflight;
    shutdown = provider->provider.operations.BeginShutdown;
    context = provider->provider.descriptor.Context;
    core_lock_release(&registry->lock);

    callback_result = shutdown(context);

    core_lock_acquire(&registry->lock);
    provider = resolve_provider(registry, provider_handle);
    if (!provider) {
        result = CAMD_REGISTRY_STALE;
    } else {
        --provider->callbacks_inflight;
        provider->shutdown_callback_done = 1;
        result = callback_result == CAMD_PROVIDER_OK
                     ? CAMD_REGISTRY_OK
                     : CAMD_REGISTRY_CALLBACK_FAILED;
    }
out:
    core_lock_release(&registry->lock);
    return result;
}

enum CAMDRegistryResult camd_registry_provider_release(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 provider_handle)
{
    struct provider_slot *provider;
    CAMDProviderShutdownReadyFnV1 shutdown_ready;
    void *context;
    enum CAMDRegistryResult result;
    int ready;

    if (!registry)
        return CAMD_REGISTRY_INVALID;
    core_lock_acquire(&registry->lock);
    provider = resolve_provider(registry, provider_handle);
    if (!provider) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    if (!provider->retiring || !provider->shutdown_callback_done) {
        result = CAMD_REGISTRY_STATE;
        goto out;
    }
    if (provider->endpoint_count != 0 || provider->session_count != 0 ||
        provider->callbacks_inflight != 0 || provider->release_checking) {
        result = CAMD_REGISTRY_BUSY;
        goto out;
    }
    provider->release_checking = 1;
    ++provider->callbacks_inflight;
    shutdown_ready = provider->provider.operations.ShutdownReady;
    context = provider->provider.descriptor.Context;
    core_lock_release(&registry->lock);

    ready = shutdown_ready(context);

    core_lock_acquire(&registry->lock);
    provider = resolve_provider(registry, provider_handle);
    if (!provider) {
        result = CAMD_REGISTRY_STALE;
    } else {
        --provider->callbacks_inflight;
        provider->release_checking = 0;
        if (!ready) {
            result = CAMD_REGISTRY_BUSY;
        } else {
            memset(&provider->provider, 0, sizeof(provider->provider));
            provider->occupied = 0;
            provider->retiring = 0;
            provider->shutdown_callback_done = 0;
            result = CAMD_REGISTRY_OK;
        }
    }
out:
    core_lock_release(&registry->lock);
    return result;
}

static enum CAMDRegistryResult allocate_session(
    struct CAMDEndpointRegistry *registry,
    struct session_slot **session_out,
    size_t *index_out)
{
    struct session_slot *session = NULL;
    struct session_slot *grown;
    size_t i, index = 0;

    for (i = 0; i < registry->session_count; ++i) {
        if (!registry->sessions[i].occupied &&
            registry->sessions[i].generation != UINT32_MAX) {
            session = &registry->sessions[i];
            index = i;
            break;
        }
    }
    if (!session) {
        if (registry->session_count == UINT32_MAX ||
            registry->session_count >= SIZE_MAX / sizeof(*grown))
            return CAMD_REGISTRY_RANGE;
        index = registry->session_count;
        grown = core_alloc((registry->session_count + 1) * sizeof(*grown), 1);
        if (!grown)
            return CAMD_REGISTRY_NOMEM;
        if (registry->session_count)
            memcpy(grown, registry->sessions,
                   registry->session_count * sizeof(*grown));
        core_free(registry->sessions);
        registry->sessions = grown;
        session = &registry->sessions[index];
        ++registry->session_count;
    }
    {
        uint32_t generation = session->generation + 1;
        memset(session, 0, sizeof(*session));
        session->generation = generation;
    }
    *session_out = session;
    *index_out = index;
    return CAMD_REGISTRY_OK;
}

enum CAMDRegistryResult camd_registry_session_open(
    struct CAMDEndpointRegistry *registry,
    const struct CAMDProviderOpenRequestV1 *request,
    struct CAMDHandleV1 *session_handle)
{
    struct endpoint_slot *endpoint = NULL;
    struct provider_slot *provider;
    struct session_slot *session;
    struct CAMDHandleV1 owner, reserved;
    CAMDProviderOpenFnV1 open_callback;
    CAMDProviderCloseFnV1 close_callback;
    void *provider_context;
    struct CAMDProviderOpenResultV1 open_result;
    enum CAMDProviderResult callback_result, close_result;
    enum CAMDRegistryResult result;
    uint32_t path;
    size_t i, index;
    int invalid_open_result;

    if (!registry || !request || !session_handle ||
        request->Size != sizeof(*request) || request->Version != 1 ||
        id_is_zero(&request->EndpointID) || request->QueueCapacity == 0 ||
        (request->Direction != CAMD_PROVIDER_DIRECTION_INPUT &&
         request->Direction != CAMD_PROVIDER_DIRECTION_OUTPUT))
        return CAMD_REGISTRY_INVALID;
    path = camd_provider_native_path(request->DataFormat, request->Protocol);
    if (path == 0)
        return CAMD_REGISTRY_UNSUPPORTED;

    core_lock_acquire(&registry->lock);
    for (i = 0; i < registry->slot_count; ++i) {
        if (registry->slots[i].occupied &&
            id_equal(&registry->slots[i].endpoint.ID, &request->EndpointID)) {
            endpoint = &registry->slots[i];
            break;
        }
    }
    if (!endpoint) {
        result = CAMD_REGISTRY_INVALID;
        goto out;
    }
    if (endpoint->endpoint.State == CAMD_ENDPOINT_RETIRING) {
        result = CAMD_REGISTRY_RETIRED;
        goto out;
    }
    if (endpoint->endpoint.State != CAMD_ENDPOINT_AVAILABLE) {
        result = CAMD_REGISTRY_STATE;
        goto out;
    }
    owner.slot = endpoint->provider_slot;
    owner.generation = endpoint->provider_generation;
    provider = resolve_provider(registry, owner);
    if (!provider) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    if (provider->retiring) {
        result = CAMD_REGISTRY_RETIRED;
        goto out;
    }
    if ((request->Direction & ~provider->provider.descriptor.Directions) != 0 ||
        (provider->provider.descriptor.NativePaths & path) == 0 ||
        (endpoint->endpoint.NativeDataFormats & request->DataFormat) == 0) {
        result = CAMD_REGISTRY_UNSUPPORTED;
        goto out;
    }
    if (endpoint->references == UINT32_MAX ||
        provider->session_count == UINT32_MAX ||
        provider->callbacks_inflight == UINT32_MAX) {
        result = CAMD_REGISTRY_RANGE;
        goto out;
    }
    result = allocate_session(registry, &session, &index);
    if (result != CAMD_REGISTRY_OK)
        goto out;
    session->occupied = 1;
    session->opening = 1;
    session->callbacks_inflight = 1;
    session->provider_slot = owner.slot;
    session->provider_generation = owner.generation;
    session->endpoint_slot = (uint32_t)(i + 1);
    session->endpoint_generation = endpoint->generation;
    session->direction = request->Direction;
    session->data_format = request->DataFormat;
    session->protocol = request->Protocol;
    session->requested_queue_capacity = request->QueueCapacity;
    ++endpoint->references;
    ++provider->session_count;
    ++provider->callbacks_inflight;
    reserved.slot = (uint32_t)(index + 1);
    reserved.generation = session->generation;
    open_callback = provider->provider.operations.Open;
    close_callback = provider->provider.operations.Close;
    provider_context = provider->provider.descriptor.Context;
    core_lock_release(&registry->lock);

    memset(&open_result, 0, sizeof(open_result));
    open_result.Size = sizeof(open_result);
    open_result.Version = 1;
    callback_result = open_callback(provider_context, request, &open_result);

    core_lock_acquire(&registry->lock);
    session = resolve_session(registry, reserved);
    provider = resolve_provider(registry, owner);
    if (!session || !provider) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    endpoint = resolve(registry, (struct CAMDHandleV1) {
        session->endpoint_slot, session->endpoint_generation
    });
    if (callback_result != CAMD_PROVIDER_OK) {
        --session->callbacks_inflight;
        --provider->callbacks_inflight;
        release_session(registry, session);
        result = provider_result(callback_result);
        goto out;
    }
    session->provider_context = open_result.SessionContext;
    invalid_open_result =
        camd_provider_validate_open_result(request, &open_result) !=
        CAMD_PROVIDER_OK;
    if (invalid_open_result ||
        !endpoint || endpoint->endpoint.State == CAMD_ENDPOINT_RETIRING) {
        session->closing = 1;
        core_lock_release(&registry->lock);
        close_result = close_callback(provider_context,
                                      open_result.SessionContext);
        core_lock_acquire(&registry->lock);
        session = resolve_session(registry, reserved);
        provider = resolve_provider(registry, owner);
        if (!session || !provider) {
            result = CAMD_REGISTRY_STALE;
            goto out;
        }
        --session->callbacks_inflight;
        --provider->callbacks_inflight;
        release_session(registry, session);
        result = close_result != CAMD_PROVIDER_OK || invalid_open_result
                     ? CAMD_REGISTRY_CALLBACK_FAILED
                     : CAMD_REGISTRY_RETIRED;
        goto out;
    }
    session->effective_queue_capacity = open_result.EffectiveQueueCapacity;
    session->max_sysex_bytes = open_result.MaxSysExBytes;
    --session->callbacks_inflight;
    --provider->callbacks_inflight;
    session->opening = 0;
    *session_handle = reserved;
    result = CAMD_REGISTRY_OK;
out:
    core_lock_release(&registry->lock);
    return result;
}

enum CAMDRegistryResult camd_registry_session_info(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session_handle,
    struct CAMDRegistrySessionInfoV1 *info)
{
    struct session_slot *session;
    enum CAMDRegistryResult result;

    if (!registry || !info || info->Size != sizeof(*info) ||
        info->Version != 1)
        return CAMD_REGISTRY_INVALID;
    core_lock_acquire(&registry->lock);
    session = resolve_session(registry, session_handle);
    if (!session) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    if (session->opening) {
        result = CAMD_REGISTRY_BUSY;
        goto out;
    }
    info->Direction = session->direction;
    info->DataFormat = session->data_format;
    info->Protocol = session->protocol;
    info->RequestedQueueCapacity = session->requested_queue_capacity;
    info->EffectiveQueueCapacity = session->effective_queue_capacity;
    info->MaxSysExBytes = session->max_sysex_bytes;
    result = CAMD_REGISTRY_OK;
out:
    core_lock_release(&registry->lock);
    return result;
}

static enum CAMDRegistryResult prepare_session_callback(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session_handle,
    uint32_t required_direction,
    uint32_t required_format,
    int allow_retiring,
    struct session_slot **session_out,
    struct provider_slot **provider_out)
{
    struct session_slot *session = resolve_session(registry, session_handle);
    struct provider_slot *provider;
    struct endpoint_slot *endpoint;
    struct CAMDHandleV1 handle;

    if (!session)
        return CAMD_REGISTRY_STALE;
    if (session->opening || session->closing || session->receive_starting ||
        session->receive_stopping)
        return CAMD_REGISTRY_BUSY;
    if ((session->direction & required_direction) == 0 ||
        (required_format != 0 && session->data_format != required_format))
        return CAMD_REGISTRY_UNSUPPORTED;
    handle.slot = session->provider_slot;
    handle.generation = session->provider_generation;
    provider = resolve_provider(registry, handle);
    handle.slot = session->endpoint_slot;
    handle.generation = session->endpoint_generation;
    endpoint = resolve(registry, handle);
    if (!provider || !endpoint)
        return CAMD_REGISTRY_STALE;
    if (!allow_retiring &&
        (provider->retiring ||
         endpoint->endpoint.State == CAMD_ENDPOINT_RETIRING))
        return CAMD_REGISTRY_RETIRED;
    if (!allow_retiring && endpoint->endpoint.State != CAMD_ENDPOINT_AVAILABLE)
        return CAMD_REGISTRY_STATE;
    if (session->callbacks_inflight == UINT32_MAX ||
        provider->callbacks_inflight == UINT32_MAX)
        return CAMD_REGISTRY_RANGE;
    ++session->callbacks_inflight;
    ++provider->callbacks_inflight;
    *session_out = session;
    *provider_out = provider;
    return CAMD_REGISTRY_OK;
}

static enum CAMDRegistryResult finish_session_callback(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session_handle,
    enum CAMDProviderResult callback_result)
{
    struct session_slot *session = resolve_session(registry, session_handle);
    struct provider_slot *provider;
    struct CAMDHandleV1 owner;

    if (!session)
        return CAMD_REGISTRY_STALE;
    owner.slot = session->provider_slot;
    owner.generation = session->provider_generation;
    provider = resolve_provider(registry, owner);
    if (!provider)
        return CAMD_REGISTRY_STALE;
    --session->callbacks_inflight;
    --provider->callbacks_inflight;
    return provider_result(callback_result);
}

enum CAMDRegistryResult camd_registry_session_send_midi1(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session_handle,
    const struct CAMDMIDI1EventV1 *events,
    size_t event_count)
{
    struct session_slot *session;
    struct provider_slot *provider;
    CAMDProviderSendMIDI1FnV1 callback;
    void *context, *provider_session;
    enum CAMDProviderResult callback_result;
    enum CAMDRegistryResult result;

    if (!registry)
        return CAMD_REGISTRY_INVALID;
    result = provider_result(camd_provider_validate_midi1(events, event_count));
    if (result != CAMD_REGISTRY_OK)
        return result;
    core_lock_acquire(&registry->lock);
    result = prepare_session_callback(registry, session_handle,
        CAMD_PROVIDER_DIRECTION_OUTPUT, CAMD_PROVIDER_FORMAT_MIDI1, 0,
        &session, &provider);
    if (result != CAMD_REGISTRY_OK)
        goto out;
    callback = provider->provider.operations.SendMIDI1;
    context = provider->provider.descriptor.Context;
    provider_session = session->provider_context;
    core_lock_release(&registry->lock);
    callback_result = callback(context, provider_session, events, event_count);
    core_lock_acquire(&registry->lock);
    result = finish_session_callback(registry, session_handle, callback_result);
out:
    core_lock_release(&registry->lock);
    return result;
}

enum CAMDRegistryResult camd_registry_session_send_midi1_sysex(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session_handle,
    const uint8_t *bytes,
    size_t byte_count,
    uint32_t time_high,
    uint32_t time_low,
    uint32_t clock_domain,
    uint32_t flags)
{
    struct session_slot *session;
    struct provider_slot *provider;
    CAMDProviderSendMIDI1SysExFnV1 callback;
    void *context, *provider_session;
    enum CAMDProviderResult callback_result;
    enum CAMDRegistryResult result;

    if (!registry)
        return CAMD_REGISTRY_INVALID;
    result = provider_result(
        camd_provider_validate_midi1_sysex(bytes, byte_count));
    if (result != CAMD_REGISTRY_OK)
        return result;
    core_lock_acquire(&registry->lock);
    result = prepare_session_callback(registry, session_handle,
        CAMD_PROVIDER_DIRECTION_OUTPUT, CAMD_PROVIDER_FORMAT_MIDI1, 0,
        &session, &provider);
    if (result != CAMD_REGISTRY_OK)
        goto out;
    if (byte_count > session->max_sysex_bytes) {
        --session->callbacks_inflight;
        --provider->callbacks_inflight;
        result = CAMD_REGISTRY_TOO_LARGE;
        goto out;
    }
    callback = provider->provider.operations.SendMIDI1SysEx;
    context = provider->provider.descriptor.Context;
    provider_session = session->provider_context;
    core_lock_release(&registry->lock);
    callback_result = callback(context, provider_session, bytes, byte_count,
                               time_high, time_low, clock_domain, flags);
    core_lock_acquire(&registry->lock);
    result = finish_session_callback(registry, session_handle, callback_result);
out:
    core_lock_release(&registry->lock);
    return result;
}

enum CAMDRegistryResult camd_registry_session_send_ump(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session_handle,
    const struct CAMDUMPEventV1 *events,
    size_t event_count)
{
    struct session_slot *session;
    struct provider_slot *provider;
    CAMDProviderSendUMPFnV1 callback;
    void *context, *provider_session;
    enum CAMDProviderResult callback_result;
    enum CAMDRegistryResult result;

    if (!registry)
        return CAMD_REGISTRY_INVALID;
    result = provider_result(camd_provider_validate_ump(events, event_count));
    if (result != CAMD_REGISTRY_OK)
        return result;
    core_lock_acquire(&registry->lock);
    result = prepare_session_callback(registry, session_handle,
        CAMD_PROVIDER_DIRECTION_OUTPUT, CAMD_PROVIDER_FORMAT_UMP, 0,
        &session, &provider);
    if (result != CAMD_REGISTRY_OK)
        goto out;
    callback = provider->provider.operations.SendUMP;
    context = provider->provider.descriptor.Context;
    provider_session = session->provider_context;
    core_lock_release(&registry->lock);
    callback_result = callback(context, provider_session, events, event_count);
    core_lock_acquire(&registry->lock);
    result = finish_session_callback(registry, session_handle, callback_result);
out:
    core_lock_release(&registry->lock);
    return result;
}

static enum CAMDProviderResult receive_bridge_begin(
    struct receive_bridge *bridge,
    uint32_t required_format,
    struct session_slot **session_out,
    struct provider_slot **provider_out)
{
    struct CAMDEndpointRegistry *registry;
    struct session_slot *session;
    struct provider_slot *provider;
    struct endpoint_slot *endpoint;
    struct CAMDHandleV1 handle;

    if (!bridge || !bridge->registry)
        return CAMD_PROVIDER_STATE;
    registry = bridge->registry;
    core_lock_acquire(&registry->lock);
    session = resolve_session(registry, bridge->session);
    if (!session || session->receive_bridge != bridge ||
        !session->receive_started || session->receive_stopping ||
        session->data_format != required_format) {
        core_lock_release(&registry->lock);
        return CAMD_PROVIDER_STATE;
    }
    handle.slot = session->provider_slot;
    handle.generation = session->provider_generation;
    provider = resolve_provider(registry, handle);
    handle.slot = session->endpoint_slot;
    handle.generation = session->endpoint_generation;
    endpoint = resolve(registry, handle);
    if (!provider || !endpoint) {
        core_lock_release(&registry->lock);
        return CAMD_PROVIDER_STATE;
    }
    if (provider->retiring ||
        endpoint->endpoint.State == CAMD_ENDPOINT_RETIRING) {
        core_lock_release(&registry->lock);
        return CAMD_PROVIDER_RETIRED;
    }
    if (session->callbacks_inflight == UINT32_MAX ||
        session->receive_inflight == UINT32_MAX ||
        provider->callbacks_inflight == UINT32_MAX) {
        core_lock_release(&registry->lock);
        return CAMD_PROVIDER_STATE;
    }
    ++session->callbacks_inflight;
    ++session->receive_inflight;
    ++provider->callbacks_inflight;
    *session_out = session;
    *provider_out = provider;
    core_lock_release(&registry->lock);
    return CAMD_PROVIDER_OK;
}

static void receive_bridge_end(struct receive_bridge *bridge)
{
    struct CAMDEndpointRegistry *registry = bridge->registry;
    struct session_slot *session;
    struct provider_slot *provider;
    struct CAMDHandleV1 owner;

    core_lock_acquire(&registry->lock);
    session = resolve_session(registry, bridge->session);
    if (session && session->receive_bridge == bridge) {
        owner.slot = session->provider_slot;
        owner.generation = session->provider_generation;
        provider = resolve_provider(registry, owner);
        --session->callbacks_inflight;
        --session->receive_inflight;
        if (provider)
            --provider->callbacks_inflight;
    }
    core_lock_release(&registry->lock);
}

static enum CAMDProviderResult receive_bridge_midi1(
    void *context, const struct CAMDMIDI1EventV1 *events, size_t event_count)
{
    struct receive_bridge *bridge = context;
    struct session_slot *session;
    struct provider_slot *provider;
    CAMDReceiveMIDI1FnV1 callback;
    void *consumer_context;
    enum CAMDProviderResult result;

    result = camd_provider_validate_midi1(events, event_count);
    if (result != CAMD_PROVIDER_OK)
        return result;
    result = receive_bridge_begin(bridge, CAMD_PROVIDER_FORMAT_MIDI1,
                                  &session, &provider);
    if (result != CAMD_PROVIDER_OK)
        return result;
    (void)session;
    (void)provider;
    callback = bridge->consumer.SubmitMIDI1;
    consumer_context = bridge->consumer.Context;
    result = callback(consumer_context, events, event_count);
    receive_bridge_end(bridge);
    return result;
}

static enum CAMDProviderResult receive_bridge_midi1_sysex(
    void *context, const uint8_t *bytes, size_t byte_count,
    uint32_t time_high, uint32_t time_low, uint32_t clock_domain,
    uint32_t flags)
{
    struct receive_bridge *bridge = context;
    struct session_slot *session;
    struct provider_slot *provider;
    CAMDReceiveMIDI1SysExFnV1 callback;
    void *consumer_context;
    enum CAMDProviderResult result;

    result = camd_provider_validate_midi1_sysex(bytes, byte_count);
    if (result != CAMD_PROVIDER_OK)
        return result;
    result = receive_bridge_begin(bridge, CAMD_PROVIDER_FORMAT_MIDI1,
                                  &session, &provider);
    if (result != CAMD_PROVIDER_OK)
        return result;
    (void)session;
    (void)provider;
    callback = bridge->consumer.SubmitMIDI1SysEx;
    consumer_context = bridge->consumer.Context;
    result = callback(consumer_context, bytes, byte_count, time_high, time_low,
                      clock_domain, flags);
    receive_bridge_end(bridge);
    return result;
}

static enum CAMDProviderResult receive_bridge_ump(
    void *context, const struct CAMDUMPEventV1 *events, size_t event_count)
{
    struct receive_bridge *bridge = context;
    struct session_slot *session;
    struct provider_slot *provider;
    CAMDReceiveUMPFnV1 callback;
    void *consumer_context;
    enum CAMDProviderResult result;

    result = camd_provider_validate_ump(events, event_count);
    if (result != CAMD_PROVIDER_OK)
        return result;
    result = receive_bridge_begin(bridge, CAMD_PROVIDER_FORMAT_UMP,
                                  &session, &provider);
    if (result != CAMD_PROVIDER_OK)
        return result;
    (void)session;
    (void)provider;
    callback = bridge->consumer.SubmitUMP;
    consumer_context = bridge->consumer.Context;
    result = callback(consumer_context, events, event_count);
    receive_bridge_end(bridge);
    return result;
}

enum CAMDRegistryResult camd_registry_session_start_receive(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session_handle,
    const struct CAMDProviderReceiveSinkV1 *sink)
{
    struct session_slot *session;
    struct provider_slot *provider;
    struct endpoint_slot *endpoint;
    struct CAMDHandleV1 endpoint_handle;
    struct receive_bridge *bridge;
    CAMDProviderStartReceiveFnV1 start_callback;
    CAMDProviderSessionFnV1 stop_callback;
    void *context, *provider_session;
    enum CAMDProviderResult callback_result, stop_result;
    enum CAMDRegistryResult result;

    if (!registry || !sink || sink->Size != sizeof(*sink) ||
        sink->Version != 1)
        return CAMD_REGISTRY_INVALID;
    core_lock_acquire(&registry->lock);
    result = prepare_session_callback(registry, session_handle,
        CAMD_PROVIDER_DIRECTION_INPUT, 0, 0, &session, &provider);
    if (result != CAMD_REGISTRY_OK)
        goto out;
    if (session->receive_started || session->receive_bridge) {
        --session->callbacks_inflight;
        --provider->callbacks_inflight;
        result = CAMD_REGISTRY_STATE;
        goto out;
    }
    if ((session->data_format == CAMD_PROVIDER_FORMAT_MIDI1 &&
         (!sink->SubmitMIDI1 || !sink->SubmitMIDI1SysEx)) ||
        (session->data_format == CAMD_PROVIDER_FORMAT_UMP &&
         !sink->SubmitUMP)) {
        --session->callbacks_inflight;
        --provider->callbacks_inflight;
        result = CAMD_REGISTRY_INVALID;
        goto out;
    }
    bridge = core_alloc(sizeof(*bridge), 1);
    if (!bridge) {
        --session->callbacks_inflight;
        --provider->callbacks_inflight;
        result = CAMD_REGISTRY_NOMEM;
        goto out;
    }
    bridge->registry = registry;
    bridge->session = session_handle;
    bridge->consumer = *sink;
    bridge->provider_sink.Size = sizeof(bridge->provider_sink);
    bridge->provider_sink.Version = 1;
    bridge->provider_sink.Context = bridge;
    if (session->data_format == CAMD_PROVIDER_FORMAT_MIDI1) {
        bridge->provider_sink.SubmitMIDI1 = receive_bridge_midi1;
        bridge->provider_sink.SubmitMIDI1SysEx = receive_bridge_midi1_sysex;
    } else {
        bridge->provider_sink.SubmitUMP = receive_bridge_ump;
    }
    session->receive_bridge = bridge;
    session->receive_starting = 1;
    session->receive_started = 1;
    start_callback = provider->provider.operations.StartReceive;
    stop_callback = provider->provider.operations.StopReceive;
    context = provider->provider.descriptor.Context;
    provider_session = session->provider_context;
    endpoint_handle.slot = session->endpoint_slot;
    endpoint_handle.generation = session->endpoint_generation;
    core_lock_release(&registry->lock);

    callback_result = start_callback(context, provider_session,
                                     &bridge->provider_sink);

    core_lock_acquire(&registry->lock);
    session = resolve_session(registry, session_handle);
    if (!session) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    provider = resolve_provider(registry, (struct CAMDHandleV1) {
        session->provider_slot, session->provider_generation
    });
    if (!provider) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    if (callback_result != CAMD_PROVIDER_OK) {
        --session->callbacks_inflight;
        --provider->callbacks_inflight;
        session->receive_starting = 0;
        session->receive_started = 0;
        core_free(session->receive_bridge);
        session->receive_bridge = NULL;
        result = provider_result(callback_result);
        goto out;
    }
    endpoint = resolve(registry, endpoint_handle);
    if (!endpoint || endpoint->endpoint.State == CAMD_ENDPOINT_RETIRING) {
        session->receive_stopping = 1;
        core_lock_release(&registry->lock);
        stop_result = stop_callback(context, provider_session);
        core_lock_acquire(&registry->lock);
        session = resolve_session(registry, session_handle);
        if (!session) {
            result = CAMD_REGISTRY_STALE;
            goto out;
        }
        provider = resolve_provider(registry, (struct CAMDHandleV1) {
            session->provider_slot, session->provider_generation
        });
        if (!provider) {
            result = CAMD_REGISTRY_STALE;
            goto out;
        }
        --session->callbacks_inflight;
        --provider->callbacks_inflight;
        session->receive_starting = 0;
        if (stop_result == CAMD_PROVIDER_OK) {
            session->receive_started = 0;
            session->receive_stop_confirmed = 1;
            if (session->receive_inflight == 0) {
                core_free(session->receive_bridge);
                session->receive_bridge = NULL;
                session->receive_stopping = 0;
                session->receive_stop_confirmed = 0;
                result = CAMD_REGISTRY_RETIRED;
            } else {
                result = CAMD_REGISTRY_BUSY;
            }
        } else {
            session->receive_stopping = 0;
            result = CAMD_REGISTRY_CALLBACK_FAILED;
        }
        goto out;
    }
    --session->callbacks_inflight;
    --provider->callbacks_inflight;
    session->receive_starting = 0;
    result = CAMD_REGISTRY_OK;
out:
    core_lock_release(&registry->lock);
    return result;
}

enum CAMDRegistryResult camd_registry_session_stop_receive(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session_handle)
{
    struct session_slot *session;
    struct provider_slot *provider;
    CAMDProviderSessionFnV1 callback;
    void *context, *provider_session;
    enum CAMDProviderResult callback_result;
    enum CAMDRegistryResult result;

    if (!registry)
        return CAMD_REGISTRY_INVALID;
    core_lock_acquire(&registry->lock);
    session = resolve_session(registry, session_handle);
    if (!session) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    if (session->receive_stopping) {
        if (!session->receive_stop_confirmed ||
            session->receive_inflight != 0) {
            result = CAMD_REGISTRY_BUSY;
            goto out;
        }
        core_free(session->receive_bridge);
        session->receive_bridge = NULL;
        session->receive_stopping = 0;
        session->receive_stop_confirmed = 0;
        result = CAMD_REGISTRY_OK;
        goto out;
    }
    result = prepare_session_callback(registry, session_handle,
        CAMD_PROVIDER_DIRECTION_INPUT, 0, 1, &session, &provider);
    if (result != CAMD_REGISTRY_OK)
        goto out;
    if (!session->receive_started || !session->receive_bridge) {
        --session->callbacks_inflight;
        --provider->callbacks_inflight;
        result = CAMD_REGISTRY_STATE;
        goto out;
    }
    session->receive_stopping = 1;
    callback = provider->provider.operations.StopReceive;
    context = provider->provider.descriptor.Context;
    provider_session = session->provider_context;
    core_lock_release(&registry->lock);
    callback_result = callback(context, provider_session);
    core_lock_acquire(&registry->lock);
    session = resolve_session(registry, session_handle);
    if (!session) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    provider = resolve_provider(registry, (struct CAMDHandleV1) {
        session->provider_slot, session->provider_generation
    });
    if (!provider) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    --session->callbacks_inflight;
    --provider->callbacks_inflight;
    if (callback_result == CAMD_PROVIDER_OK) {
        session->receive_started = 0;
        session->receive_stop_confirmed = 1;
        if (session->receive_inflight == 0) {
            core_free(session->receive_bridge);
            session->receive_bridge = NULL;
            session->receive_stopping = 0;
            session->receive_stop_confirmed = 0;
            result = CAMD_REGISTRY_OK;
        } else {
            result = CAMD_REGISTRY_BUSY;
        }
    } else {
        session->receive_stopping = 0;
        result = provider_result(callback_result);
    }
out:
    core_lock_release(&registry->lock);
    return result;
}

static enum CAMDRegistryResult session_control(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session_handle,
    int drain)
{
    struct session_slot *session;
    struct provider_slot *provider;
    CAMDProviderSessionFnV1 callback;
    void *context, *provider_session;
    enum CAMDProviderResult callback_result;
    enum CAMDRegistryResult result;

    if (!registry)
        return CAMD_REGISTRY_INVALID;
    core_lock_acquire(&registry->lock);
    result = prepare_session_callback(registry, session_handle,
        CAMD_PROVIDER_DIRECTION_OUTPUT, 0, 1, &session, &provider);
    if (result != CAMD_REGISTRY_OK)
        goto out;
    callback = drain ? provider->provider.operations.Drain
                     : provider->provider.operations.Cancel;
    context = provider->provider.descriptor.Context;
    provider_session = session->provider_context;
    core_lock_release(&registry->lock);
    callback_result = callback(context, provider_session);
    core_lock_acquire(&registry->lock);
    result = finish_session_callback(registry, session_handle, callback_result);
out:
    core_lock_release(&registry->lock);
    return result;
}

enum CAMDRegistryResult camd_registry_session_drain(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session)
{
    return session_control(registry, session, 1);
}

enum CAMDRegistryResult camd_registry_session_cancel(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session)
{
    return session_control(registry, session, 0);
}

enum CAMDRegistryResult camd_registry_session_close(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 session_handle)
{
    struct session_slot *session;
    struct provider_slot *provider;
    struct CAMDHandleV1 owner;
    CAMDProviderCloseFnV1 callback;
    void *context, *provider_session;
    enum CAMDProviderResult callback_result;
    enum CAMDRegistryResult result;

    if (!registry)
        return CAMD_REGISTRY_INVALID;
    core_lock_acquire(&registry->lock);
    session = resolve_session(registry, session_handle);
    if (!session) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    if (session->opening || session->closing || session->receive_starting ||
        session->callbacks_inflight != 0) {
        result = CAMD_REGISTRY_BUSY;
        goto out;
    }
    if (session->receive_started) {
        result = CAMD_REGISTRY_STATE;
        goto out;
    }
    owner.slot = session->provider_slot;
    owner.generation = session->provider_generation;
    provider = resolve_provider(registry, owner);
    if (!provider) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    session->closing = 1;
    ++session->callbacks_inflight;
    ++provider->callbacks_inflight;
    callback = provider->provider.operations.Close;
    context = provider->provider.descriptor.Context;
    provider_session = session->provider_context;
    core_lock_release(&registry->lock);
    callback_result = callback(context, provider_session);
    core_lock_acquire(&registry->lock);
    session = resolve_session(registry, session_handle);
    provider = resolve_provider(registry, owner);
    if (!session || !provider) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    --session->callbacks_inflight;
    --provider->callbacks_inflight;
    if (callback_result == CAMD_PROVIDER_OK) {
        release_session(registry, session);
        result = CAMD_REGISTRY_OK;
    } else {
        session->closing = 0;
        result = provider_result(callback_result);
    }
out:
    core_lock_release(&registry->lock);
    return result;
}

enum CAMDRegistryResult camd_registry_publish(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 provider_handle,
    const struct CAMDEndpointInfoV1 *endpoint,
    const struct CAMDGroupInfoV1 *groups,
    size_t group_count,
    const struct CAMDFunctionBlockInfoV1 *blocks,
    size_t block_count,
    struct CAMDHandleV1 *provider_lease)
{
    struct CAMDGroupInfoV1 *group_copy;
    struct CAMDFunctionBlockInfoV1 *block_copy;
    struct endpoint_slot *slot = NULL;
    struct provider_slot *provider;
    struct endpoint_slot *grown;
    enum CAMDRegistryResult result;
    size_t i, index = 0;

    if (!registry || !provider_lease)
        return CAMD_REGISTRY_INVALID;
    result = validate_endpoint(endpoint);
    if (result != CAMD_REGISTRY_OK)
        return result;
    result = copy_topology(endpoint, groups, group_count, blocks, block_count,
                           &group_copy, &block_copy);
    if (result != CAMD_REGISTRY_OK)
        return result;
    core_lock_acquire(&registry->lock);
    provider = resolve_provider(registry, provider_handle);
    if (!provider) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    if (provider->retiring) {
        result = CAMD_REGISTRY_RETIRED;
        goto out;
    }
    if (!id_equal(&provider->provider.descriptor.ProviderID,
                  &endpoint->ProviderID)) {
        result = CAMD_REGISTRY_INVALID;
        goto out;
    }
    if (((endpoint->NativeDataFormats & CAMD_DATA_FORMAT_MIDI1) != 0 &&
         (provider->provider.descriptor.NativePaths &
          CAMD_PROVIDER_PATH_MIDI1) == 0) ||
        ((endpoint->NativeDataFormats & CAMD_DATA_FORMAT_UMP) != 0 &&
         (provider->provider.descriptor.NativePaths &
          (CAMD_PROVIDER_PATH_UMP_MIDI1 |
           CAMD_PROVIDER_PATH_UMP_MIDI2)) == 0)) {
        result = CAMD_REGISTRY_UNSUPPORTED;
        goto out;
    }
    if (provider->endpoint_count == UINT32_MAX) {
        result = CAMD_REGISTRY_RANGE;
        goto out;
    }
    if (!generation_available(registry)) {
        result = CAMD_REGISTRY_RANGE;
        goto out;
    }
    for (i = 0; i < registry->slot_count; ++i) {
        if (registry->slots[i].occupied &&
            id_equal(&registry->slots[i].endpoint.ID, &endpoint->ID)) {
            result = CAMD_REGISTRY_DUPLICATE;
            goto out;
        }
        /* A generation that reached UINT32_MAX is quarantined forever. */
        if (!slot && !registry->slots[i].occupied &&
            registry->slots[i].generation != UINT32_MAX) {
            slot = &registry->slots[i];
            index = i;
        }
    }
    if (!slot) {
        if (registry->slot_count == UINT32_MAX ||
            registry->slot_count >= SIZE_MAX / sizeof(*grown)) {
            result = CAMD_REGISTRY_RANGE;
            goto out;
        }
        index = registry->slot_count;
        grown = core_alloc((registry->slot_count + 1) * sizeof(*grown), 1);
        if (!grown) {
            result = CAMD_REGISTRY_NOMEM;
            goto out;
        }
        if (registry->slot_count)
            memcpy(grown, registry->slots,
                   registry->slot_count * sizeof(*grown));
        core_free(registry->slots);
        registry->slots = grown;
        slot = &registry->slots[index];
        memset(slot, 0, sizeof(*slot));
        ++registry->slot_count;
    }
    ++slot->generation;
    slot->occupied = 1;
    slot->references = 1;
    slot->provider_slot = provider_handle.slot;
    slot->provider_generation = provider_handle.generation;
    slot->endpoint = *endpoint;
    slot->groups = group_copy;
    slot->group_count = group_count;
    slot->blocks = block_copy;
    slot->block_count = block_count;
    advance_generation(registry);
    slot->endpoint.Generation = registry->generation;
    notify_watches(registry, CAMD_ENDPOINT_EVENT_ADDED,
                   &slot->endpoint.ID);
    ++provider->endpoint_count;
    provider_lease->slot = (uint32_t)(index + 1);
    provider_lease->generation = slot->generation;
    group_copy = NULL;
    block_copy = NULL;
    result = CAMD_REGISTRY_OK;
out:
    core_lock_release(&registry->lock);
    core_free(group_copy);
    core_free(block_copy);
    return result;
}

enum CAMDRegistryResult camd_registry_replace(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 provider_lease,
    const struct CAMDEndpointInfoV1 *endpoint,
    const struct CAMDGroupInfoV1 *groups,
    size_t group_count,
    const struct CAMDFunctionBlockInfoV1 *blocks,
    size_t block_count)
{
    struct endpoint_slot *slot;
    struct CAMDGroupInfoV1 *group_copy;
    struct CAMDFunctionBlockInfoV1 *block_copy;
    enum CAMDRegistryResult result;

    if (!registry)
        return CAMD_REGISTRY_INVALID;
    result = validate_endpoint(endpoint);
    if (result != CAMD_REGISTRY_OK)
        return result;
    result = copy_topology(endpoint, groups, group_count, blocks, block_count,
                           &group_copy, &block_copy);
    if (result != CAMD_REGISTRY_OK)
        return result;
    core_lock_acquire(&registry->lock);
    slot = resolve(registry, provider_lease);
    if (!slot) {
        result = CAMD_REGISTRY_STALE;
        goto out;
    }
    if (slot->endpoint.State == CAMD_ENDPOINT_RETIRING) {
        result = CAMD_REGISTRY_RETIRED;
        goto out;
    }
    if (!generation_available(registry)) {
        result = CAMD_REGISTRY_RANGE;
        goto out;
    }
    if (!id_equal(&slot->endpoint.ID, &endpoint->ID) ||
        !id_equal(&slot->endpoint.ProviderID, &endpoint->ProviderID) ||
        slot->endpoint.State != endpoint->State) {
        result = CAMD_REGISTRY_INVALID;
        goto out;
    }
    free_topology(slot);
    slot->endpoint = *endpoint;
    slot->groups = group_copy;
    slot->group_count = group_count;
    slot->blocks = block_copy;
    slot->block_count = block_count;
    advance_generation(registry);
    slot->endpoint.Generation = registry->generation;
    notify_watches(registry, CAMD_ENDPOINT_EVENT_UPDATED,
                   &slot->endpoint.ID);
    group_copy = NULL;
    block_copy = NULL;
    result = CAMD_REGISTRY_OK;
out:
    core_lock_release(&registry->lock);
    core_free(group_copy);
    core_free(block_copy);
    return result;
}

enum CAMDRegistryResult camd_registry_set_state(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 provider_lease,
    uint32_t state)
{
    struct endpoint_slot *slot;
    enum CAMDRegistryResult result;

    if (!registry)
        return CAMD_REGISTRY_INVALID;
    core_lock_acquire(&registry->lock);
    slot = resolve(registry, provider_lease);
    if (!slot)
        result = CAMD_REGISTRY_STALE;
    else if (!transition_allowed(slot->endpoint.State, state))
        result = CAMD_REGISTRY_STATE;
    else if (!generation_available(registry))
        result = CAMD_REGISTRY_RANGE;
    else {
        uint32_t event_type = CAMD_ENDPOINT_EVENT_UPDATED;

        slot->endpoint.State = state;
        advance_generation(registry);
        slot->endpoint.Generation = registry->generation;
        if (state == CAMD_ENDPOINT_OFFLINE)
            event_type = CAMD_ENDPOINT_EVENT_OFFLINE;
        else if (state == CAMD_ENDPOINT_RETIRING)
            event_type = CAMD_ENDPOINT_EVENT_RETIRED;
        notify_watches(registry, event_type, &slot->endpoint.ID);
        result = CAMD_REGISTRY_OK;
    }
    core_lock_release(&registry->lock);
    return result;
}

enum CAMDRegistryResult camd_registry_acquire(
    struct CAMDEndpointRegistry *registry,
    const struct CAMDEndpointIDV1 *id,
    struct CAMDHandleV1 *lease)
{
    size_t i;
    enum CAMDRegistryResult result = CAMD_REGISTRY_INVALID;

    if (!registry || !id || !lease || id_is_zero(id))
        return CAMD_REGISTRY_INVALID;
    core_lock_acquire(&registry->lock);
    for (i = 0; i < registry->slot_count; ++i) {
        struct endpoint_slot *slot = &registry->slots[i];
        if (slot->occupied && id_equal(&slot->endpoint.ID, id)) {
            if (slot->endpoint.State == CAMD_ENDPOINT_RETIRING)
                result = CAMD_REGISTRY_RETIRED;
            else if (slot->references == UINT32_MAX)
                result = CAMD_REGISTRY_RANGE;
            else {
                ++slot->references;
                lease->slot = (uint32_t)(i + 1);
                lease->generation = slot->generation;
                result = CAMD_REGISTRY_OK;
            }
            break;
        }
    }
    core_lock_release(&registry->lock);
    return result;
}

enum CAMDRegistryResult camd_registry_retire(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 provider_lease)
{
    return camd_registry_set_state(registry, provider_lease,
                                   CAMD_ENDPOINT_RETIRING);
}

enum CAMDRegistryResult camd_registry_release(
    struct CAMDEndpointRegistry *registry,
    struct CAMDHandleV1 lease)
{
    struct endpoint_slot *slot;
    enum CAMDRegistryResult result;

    if (!registry)
        return CAMD_REGISTRY_INVALID;
    core_lock_acquire(&registry->lock);
    slot = resolve(registry, lease);
    if (!slot)
        result = CAMD_REGISTRY_STALE;
    else if (slot->references == 0)
        result = CAMD_REGISTRY_INVALID;
    else {
        release_endpoint_reference(registry, slot);
        result = CAMD_REGISTRY_OK;
    }
    core_lock_release(&registry->lock);
    return result;
}

enum CAMDRegistryResult camd_registry_snapshot(
    struct CAMDEndpointRegistry *registry,
    struct CAMDEndpointSnapshot **snapshot_out)
{
    struct CAMDEndpointSnapshot *snapshot;
    size_t i, capacity;

    if (!registry || !snapshot_out)
        return CAMD_REGISTRY_INVALID;
    core_lock_acquire(&registry->lock);
    capacity = registry->slot_count;
    snapshot = core_alloc(sizeof(*snapshot), 1);
    if (!snapshot) {
        core_lock_release(&registry->lock);
        return CAMD_REGISTRY_NOMEM;
    }
    if (capacity) {
        if (capacity > SIZE_MAX / sizeof(*snapshot->endpoints)) {
            core_free(snapshot);
            core_lock_release(&registry->lock);
            return CAMD_REGISTRY_RANGE;
        }
        snapshot->endpoints = core_alloc(
            capacity * sizeof(*snapshot->endpoints), 1);
        if (!snapshot->endpoints) {
            core_free(snapshot);
            core_lock_release(&registry->lock);
            return CAMD_REGISTRY_NOMEM;
        }
    }
    snapshot->generation = registry->generation;
    for (i = 0; i < registry->slot_count; ++i) {
        struct endpoint_slot *slot = &registry->slots[i];
        struct snapshot_endpoint *copy;
        if (!slot->occupied ||
            slot->endpoint.State == CAMD_ENDPOINT_RETIRING)
            continue;
        copy = &snapshot->endpoints[snapshot->endpoint_count];
        copy->endpoint = slot->endpoint;
        copy->group_count = slot->group_count;
        copy->block_count = slot->block_count;
        if (copy->group_count) {
            copy->groups = core_alloc(
                copy->group_count * sizeof(*copy->groups), 0);
            if (!copy->groups)
                goto no_memory;
            memcpy(copy->groups, slot->groups,
                   copy->group_count * sizeof(*copy->groups));
        }
        if (copy->block_count) {
            copy->blocks = core_alloc(
                copy->block_count * sizeof(*copy->blocks), 0);
            if (!copy->blocks) {
                core_free(copy->groups);
                copy->groups = NULL;
                goto no_memory;
            }
            memcpy(copy->blocks, slot->blocks,
                   copy->block_count * sizeof(*copy->blocks));
        }
        ++snapshot->endpoint_count;
    }
    *snapshot_out = snapshot;
    core_lock_release(&registry->lock);
    return CAMD_REGISTRY_OK;

no_memory:
    core_lock_release(&registry->lock);
    camd_snapshot_destroy(snapshot);
    return CAMD_REGISTRY_NOMEM;
}

enum CAMDRegistryResult camd_registry_watch_start(
    struct CAMDEndpointRegistry *registry,
    size_t capacity,
    struct CAMDEndpointWatch **watch_out,
    struct CAMDGenerationV1 *generation)
{
    struct CAMDEndpointWatch *watch;

    if (!registry || !watch_out || !generation || capacity == 0 ||
        capacity > CAMD_ENDPOINT_WATCH_MAX_EVENTS ||
        capacity > SIZE_MAX / sizeof(*watch->events))
        return CAMD_REGISTRY_INVALID;
    watch = core_alloc(sizeof(*watch), 1);
    if (!watch)
        return CAMD_REGISTRY_NOMEM;
    watch->events = core_alloc(capacity * sizeof(*watch->events), 1);
    if (!watch->events) {
        core_free(watch);
        return CAMD_REGISTRY_NOMEM;
    }
    watch->registry = registry;
    watch->capacity = capacity;
    core_lock_acquire(&registry->lock);
    watch->next = registry->watches;
    registry->watches = watch;
    *generation = registry->generation;
    *watch_out = watch;
    core_lock_release(&registry->lock);
    return CAMD_REGISTRY_OK;
}

enum CAMDRegistryResult camd_endpoint_watch_read(
    struct CAMDEndpointWatch *watch,
    struct CAMDEndpointWatchEventV1 *event)
{
    struct CAMDEndpointRegistry *registry;
    enum CAMDRegistryResult result;

    if (!watch || !event || !watch->registry)
        return CAMD_REGISTRY_INVALID;
    registry = watch->registry;
    core_lock_acquire(&registry->lock);
    if (watch->lost) {
        memset(event, 0, sizeof(*event));
        event->Size = sizeof(*event);
        event->Version = 1;
        event->Generation = watch->lost_generation;
        event->Type = CAMD_ENDPOINT_EVENT_LOST;
        watch->head = 0;
        watch->count = 0;
        watch->lost = 0;
        result = CAMD_REGISTRY_OK;
    } else if (watch->count == 0) {
        result = CAMD_REGISTRY_EMPTY;
    } else {
        *event = watch->events[watch->head];
        watch->head = (watch->head + 1) % watch->capacity;
        --watch->count;
        result = CAMD_REGISTRY_OK;
    }
    core_lock_release(&registry->lock);
    return result;
}

void camd_endpoint_watch_end(struct CAMDEndpointWatch *watch)
{
    struct CAMDEndpointRegistry *registry;
    struct CAMDEndpointWatch **link;

    if (!watch || !watch->registry)
        return;
    registry = watch->registry;
    core_lock_acquire(&registry->lock);
    for (link = &registry->watches; *link; link = &(*link)->next) {
        if (*link == watch) {
            *link = watch->next;
            watch->registry = NULL;
            break;
        }
    }
    core_lock_release(&registry->lock);
    if (!watch->registry) {
        core_free(watch->events);
        core_free(watch);
    }
}

void camd_snapshot_destroy(struct CAMDEndpointSnapshot *snapshot)
{
    size_t i;

    if (!snapshot)
        return;
    for (i = 0; i < snapshot->endpoint_count; ++i) {
        core_free(snapshot->endpoints[i].groups);
        core_free(snapshot->endpoints[i].blocks);
    }
    core_free(snapshot->endpoints);
    core_free(snapshot);
}

struct CAMDGenerationV1 camd_snapshot_generation(
    const struct CAMDEndpointSnapshot *snapshot)
{
    struct CAMDGenerationV1 zero = { 0, 0 };
    return snapshot ? snapshot->generation : zero;
}

size_t camd_snapshot_endpoint_count(const struct CAMDEndpointSnapshot *snapshot)
{
    return snapshot ? snapshot->endpoint_count : 0;
}

enum CAMDRegistryResult camd_snapshot_endpoint(
    const struct CAMDEndpointSnapshot *snapshot,
    size_t index,
    const struct CAMDEndpointInfoV1 **endpoint,
    const struct CAMDGroupInfoV1 **groups,
    size_t *group_count,
    const struct CAMDFunctionBlockInfoV1 **blocks,
    size_t *block_count)
{
    const struct snapshot_endpoint *item;

    if (!snapshot || index >= snapshot->endpoint_count || !endpoint ||
        !groups || !group_count || !blocks || !block_count)
        return CAMD_REGISTRY_RANGE;
    item = &snapshot->endpoints[index];
    *endpoint = &item->endpoint;
    *groups = item->groups;
    *group_count = item->group_count;
    *blocks = item->blocks;
    *block_count = item->block_count;
    return CAMD_REGISTRY_OK;
}
