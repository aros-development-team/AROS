#ifndef CAMD_IDENTITY_MAP_H
#define CAMD_IDENTITY_MAP_H

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
    CAMD_IDENTITY_MAP_GENERATOR
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
enum CAMDIdentityMapResult camd_identity_map_resolve(
    struct CAMDIdentityMap *map, const struct CAMDIdentityKeyV1 *key,
    struct CAMDIdentityResolutionV1 *resolution);

#endif
