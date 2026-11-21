#ifndef CAMD_IDENTITY_MAP_H
#define CAMD_IDENTITY_MAP_H

/* Private persistent mapping from provider evidence keys to opaque CAMD IDs. */

#include "endpoint_registry.h"

#include <stddef.h>
#include <stdint.h>

#define CAMD_IDENTITY_KEY_BYTES 256

enum CAMDIdentityMapResult {
    CAMD_IDENTITY_MAP_OK = 0,
    CAMD_IDENTITY_MAP_INVALID,
    CAMD_IDENTITY_MAP_NOMEM,
    CAMD_IDENTITY_MAP_FULL,
    CAMD_IDENTITY_MAP_COLLISION,
    CAMD_IDENTITY_MAP_GENERATOR,
    CAMD_IDENTITY_MAP_MISSING,
    CAMD_IDENTITY_MAP_STORAGE
};

enum CAMDIdentityConfidenceV1 {
    CAMD_IDENTITY_AUTHORITATIVE = 1,
    CAMD_IDENTITY_CONFIGURED,
    CAMD_IDENTITY_PATH_BOUND,
    CAMD_IDENTITY_EPHEMERAL
};

struct CAMDIdentityKeyV1 {
    uint32_t Size;
    uint32_t Version;
    uint32_t Namespace;
    uint32_t Confidence;
    uint32_t ByteCount;
    uint8_t Bytes[CAMD_IDENTITY_KEY_BYTES];
};

/* This is the versioned persistence record. Reserved must be zero. */
struct CAMDIdentityRecordV1 {
    uint32_t Size;
    uint32_t Version;
    uint32_t Namespace;
    uint32_t KeyByteCount;
    uint8_t Key[CAMD_IDENTITY_KEY_BYTES];
    struct CAMDEndpointIDV1 ID;
    uint32_t Reserved[4];
};

struct CAMDIdentityResolutionV1 {
    uint32_t Size;
    uint32_t Version;
    struct CAMDEndpointIDV1 ID;
    uint32_t EffectiveConfidence;
    uint32_t Persistent;
    uint32_t Existing;
};

typedef int (*CAMDIdentityGenerateFnV1)(
    void *context, struct CAMDEndpointIDV1 *id);
/* Atomically replace persistent storage with this complete record set. The
 * callback must not reenter the map. A zero return leaves every new mapping
 * usable only as an in-memory ephemeral identity. */
typedef int (*CAMDIdentityCommitFnV1)(
    void *context, const struct CAMDIdentityRecordV1 *records,
    size_t record_count);

struct CAMDIdentityMapConfigV1 {
    uint32_t Size;
    uint32_t Version;
    uint32_t Capacity;
    const struct CAMDIdentityRecordV1 *InitialRecords;
    size_t InitialRecordCount;
    void *CallbackContext;
    CAMDIdentityGenerateFnV1 Generate;
    CAMDIdentityCommitFnV1 Commit;
};

struct CAMDIdentityMap;

enum CAMDIdentityMapResult camd_identity_map_create(
    const struct CAMDIdentityMapConfigV1 *config,
    struct CAMDIdentityMap **map);
void camd_identity_map_destroy(struct CAMDIdentityMap *map);

/* An ID that belongs to no key and is not stored: for one run only. It
 * differs from every ID the map holds. */
enum CAMDIdentityMapResult camd_identity_map_temporary(
    struct CAMDIdentityMap *map, struct CAMDEndpointIDV1 *id);

enum CAMDIdentityMapResult camd_identity_map_resolve(
    struct CAMDIdentityMap *map, const struct CAMDIdentityKeyV1 *key,
    struct CAMDIdentityResolutionV1 *resolution);

/* How many mappings the map holds, and how many it has room for. */
uint32_t camd_identity_map_count(struct CAMDIdentityMap *map);
uint32_t camd_identity_map_capacity(struct CAMDIdentityMap *map);

/* A copy of mapping number index, counted from 0 in no particular order,
 * and whether it is stored. CAMD_IDENTITY_MAP_MISSING beyond the last. The
 * order changes when a mapping is removed. */
enum CAMDIdentityMapResult camd_identity_map_get(
    struct CAMDIdentityMap *map, uint32_t index,
    struct CAMDIdentityRecordV1 *record, uint32_t *persistent);

/* Forgets the mapping to this ID: its key gets a new ID the next time it is
 * resolved. A stored mapping goes from storage first, and stays when that
 * fails (CAMD_IDENTITY_MAP_STORAGE). CAMD_IDENTITY_MAP_MISSING when no
 * mapping has the ID. Whether something still uses the ID is the caller's
 * to know. */
enum CAMDIdentityMapResult camd_identity_map_remove(
    struct CAMDIdentityMap *map, const struct CAMDEndpointIDV1 *id);

#endif
