#include "identity_map.h"

#include <limits.h>
#include <string.h>

#ifdef __AROS__
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <proto/exec.h>
#else
#include <pthread.h>
#include <stdlib.h>
#endif

#define GENERATE_ATTEMPTS 16

struct identity_lock {
#ifdef __AROS__
    struct SignalSemaphore semaphore;
#else
    pthread_mutex_t mutex;
#endif
};

struct identity_entry {
    struct CAMDIdentityRecordV1 record;
    int persistent;
};

struct CAMDIdentityMap {
    struct identity_lock lock;
    struct identity_entry *entries;
    struct CAMDIdentityRecordV1 *commit_records;
    uint32_t capacity;
    uint32_t count;
    void *callback_context;
    CAMDIdentityGenerateFnV1 generate;
    CAMDIdentityCommitFnV1 commit;
};

static void *map_alloc(size_t size)
{
#ifdef __AROS__
    if (size > (size_t)ULONG_MAX)
        return NULL;
    return AllocVec((ULONG)size, MEMF_ANY | MEMF_CLEAR);
#else
    return calloc(1, size);
#endif
}

static void map_free(void *pointer)
{
#ifdef __AROS__
    if (pointer)
        FreeVec(pointer);
#else
    free(pointer);
#endif
}

static int lock_init(struct identity_lock *lock)
{
#ifdef __AROS__
    InitSemaphore(&lock->semaphore);
    return 1;
#else
    return pthread_mutex_init(&lock->mutex, NULL) == 0;
#endif
}

static void lock_destroy(struct identity_lock *lock)
{
#ifdef __AROS__
    (void)lock;
#else
    pthread_mutex_destroy(&lock->mutex);
#endif
}

static void lock_acquire(struct identity_lock *lock)
{
#ifdef __AROS__
    ObtainSemaphore(&lock->semaphore);
#else
    pthread_mutex_lock(&lock->mutex);
#endif
}

static void lock_release(struct identity_lock *lock)
{
#ifdef __AROS__
    ReleaseSemaphore(&lock->semaphore);
#else
    pthread_mutex_unlock(&lock->mutex);
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

static int valid_confidence(uint32_t confidence)
{
    return confidence >= CAMD_IDENTITY_AUTHORITATIVE &&
           confidence <= CAMD_IDENTITY_EPHEMERAL;
}

static int valid_key(const struct CAMDIdentityKeyV1 *key)
{
    return key && key->Size == sizeof(*key) && key->Version == 1 &&
           key->Namespace != 0 && valid_confidence(key->Confidence) &&
           key->ByteCount != 0 && key->ByteCount <= sizeof(key->Bytes);
}

static int valid_record(const struct CAMDIdentityRecordV1 *record)
{
    static const uint32_t zero[4] = { 0, 0, 0, 0 };

    return record && record->Size == sizeof(*record) &&
           record->Version == 1 && record->Namespace != 0 &&
           record->KeyByteCount != 0 &&
           record->KeyByteCount <= sizeof(record->Key) &&
           !id_is_zero(&record->ID) &&
           memcmp(record->Reserved, zero, sizeof(zero)) == 0;
}

static int same_key(const struct CAMDIdentityRecordV1 *record,
                    const struct CAMDIdentityKeyV1 *key)
{
    return record->Namespace == key->Namespace &&
           record->KeyByteCount == key->ByteCount &&
           memcmp(record->Key, key->Bytes, key->ByteCount) == 0;
}

static int record_key_equal(const struct CAMDIdentityRecordV1 *left,
                            const struct CAMDIdentityRecordV1 *right)
{
    return left->Namespace == right->Namespace &&
           left->KeyByteCount == right->KeyByteCount &&
           memcmp(left->Key, right->Key, left->KeyByteCount) == 0;
}

static int id_exists(const struct CAMDIdentityMap *map,
                     const struct CAMDEndpointIDV1 *id)
{
    uint32_t i;

    for (i = 0; i < map->count; ++i) {
        if (id_equal(&map->entries[i].record.ID, id))
            return 1;
    }
    return 0;
}

static int commit_all(struct CAMDIdentityMap *map)
{
    uint32_t i;

    if (!map->commit)
        return 0;
    for (i = 0; i < map->count; ++i)
        map->commit_records[i] = map->entries[i].record;
    if (!map->commit(map->callback_context, map->commit_records, map->count))
        return 0;
    for (i = 0; i < map->count; ++i)
        map->entries[i].persistent = 1;
    return 1;
}

enum CAMDIdentityMapResult camd_identity_map_create(
    const struct CAMDIdentityMapConfigV1 *config,
    struct CAMDIdentityMap **map_out)
{
    struct CAMDIdentityMap *map;
    size_t entry_bytes, record_bytes;
    uint32_t i, j;

    if (!map_out)
        return CAMD_IDENTITY_MAP_INVALID;
    *map_out = NULL;
    if (!config || config->Size != sizeof(*config) ||
        config->Version != 1 || config->Capacity == 0 || !config->Generate ||
        config->InitialRecordCount > config->Capacity ||
        (config->InitialRecordCount != 0 && !config->InitialRecords))
        return CAMD_IDENTITY_MAP_INVALID;
#if SIZE_MAX <= UINT32_MAX
    if (config->Capacity > SIZE_MAX / sizeof(struct identity_entry) ||
        config->Capacity > SIZE_MAX / sizeof(struct CAMDIdentityRecordV1))
        return CAMD_IDENTITY_MAP_INVALID;
#endif
    for (i = 0; i < config->InitialRecordCount; ++i) {
        if (!valid_record(&config->InitialRecords[i]))
            return CAMD_IDENTITY_MAP_INVALID;
        for (j = 0; j < i; ++j) {
            if (record_key_equal(&config->InitialRecords[i],
                                 &config->InitialRecords[j]) ||
                id_equal(&config->InitialRecords[i].ID,
                         &config->InitialRecords[j].ID))
                return CAMD_IDENTITY_MAP_COLLISION;
        }
    }
    map = map_alloc(sizeof(*map));
    if (!map)
        return CAMD_IDENTITY_MAP_NOMEM;
    if (!lock_init(&map->lock)) {
        map_free(map);
        return CAMD_IDENTITY_MAP_NOMEM;
    }
    entry_bytes = (size_t)config->Capacity * sizeof(*map->entries);
    record_bytes = (size_t)config->Capacity * sizeof(*map->commit_records);
    map->entries = map_alloc(entry_bytes);
    map->commit_records = map_alloc(record_bytes);
    if (!map->entries || !map->commit_records) {
        map_free(map->commit_records);
        map_free(map->entries);
        lock_destroy(&map->lock);
        map_free(map);
        return CAMD_IDENTITY_MAP_NOMEM;
    }
    map->capacity = config->Capacity;
    map->count = (uint32_t)config->InitialRecordCount;
    map->callback_context = config->CallbackContext;
    map->generate = config->Generate;
    map->commit = config->Commit;
    for (i = 0; i < map->count; ++i) {
        map->entries[i].record = config->InitialRecords[i];
        map->entries[i].persistent = 1;
    }
    *map_out = map;
    return CAMD_IDENTITY_MAP_OK;
}

void camd_identity_map_destroy(struct CAMDIdentityMap *map)
{
    if (!map)
        return;
    map_free(map->commit_records);
    map_free(map->entries);
    lock_destroy(&map->lock);
    map_free(map);
}

enum CAMDIdentityMapResult camd_identity_map_temporary(
    struct CAMDIdentityMap *map, struct CAMDEndpointIDV1 *id)
{
    enum CAMDIdentityMapResult result = CAMD_IDENTITY_MAP_COLLISION;
    unsigned int attempt;

    if (!map || !id)
        return CAMD_IDENTITY_MAP_INVALID;
    lock_acquire(&map->lock);
    for (attempt = 0; attempt < GENERATE_ATTEMPTS; ++attempt) {
        if (!map->generate(map->callback_context, id)) {
            result = CAMD_IDENTITY_MAP_GENERATOR;
            break;
        }
        if (!id_is_zero(id) && !id_exists(map, id)) {
            result = CAMD_IDENTITY_MAP_OK;
            break;
        }
    }
    lock_release(&map->lock);
    if (result != CAMD_IDENTITY_MAP_OK)
        memset(id, 0, sizeof(*id));
    return result;
}

enum CAMDIdentityMapResult camd_identity_map_resolve(
    struct CAMDIdentityMap *map, const struct CAMDIdentityKeyV1 *key,
    struct CAMDIdentityResolutionV1 *resolution)
{
    struct identity_entry *entry = NULL;
    struct CAMDEndpointIDV1 id;
    uint32_t i, attempt;
    int existing = 0;

    if (!map || !valid_key(key) || !resolution ||
        resolution->Size != sizeof(*resolution) || resolution->Version != 1)
        return CAMD_IDENTITY_MAP_INVALID;
    lock_acquire(&map->lock);
    for (i = 0; i < map->count; ++i) {
        if (same_key(&map->entries[i].record, key)) {
            entry = &map->entries[i];
            existing = 1;
            break;
        }
    }
    if (!entry) {
        if (map->count == map->capacity) {
            lock_release(&map->lock);
            return CAMD_IDENTITY_MAP_FULL;
        }
        memset(&id, 0, sizeof(id));
        for (attempt = 0; attempt < GENERATE_ATTEMPTS; ++attempt) {
            if (!map->generate(map->callback_context, &id)) {
                lock_release(&map->lock);
                return CAMD_IDENTITY_MAP_GENERATOR;
            }
            if (!id_is_zero(&id) && !id_exists(map, &id))
                break;
        }
        if (attempt == GENERATE_ATTEMPTS) {
            lock_release(&map->lock);
            return CAMD_IDENTITY_MAP_COLLISION;
        }
        entry = &map->entries[map->count++];
        entry->record.Size = sizeof(entry->record);
        entry->record.Version = 1;
        entry->record.Namespace = key->Namespace;
        entry->record.KeyByteCount = key->ByteCount;
        memcpy(entry->record.Key, key->Bytes, key->ByteCount);
        entry->record.ID = id;
    }
    if (!entry->persistent)
        commit_all(map);
    resolution->ID = entry->record.ID;
    resolution->Persistent = entry->persistent ? 1u : 0u;
    resolution->EffectiveConfidence = entry->persistent
                                          ? key->Confidence
                                          : CAMD_IDENTITY_EPHEMERAL;
    resolution->Existing = existing ? 1u : 0u;
    lock_release(&map->lock);
    return CAMD_IDENTITY_MAP_OK;
}
