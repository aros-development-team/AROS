#include "legacy_identity_key.h"

#include <string.h>

static int module_leaf(const char *path, const char **leaf, size_t *length)
{
    const char *cursor, *start;
    size_t count = 0;

    if (!path || !*path)
        return 0;
    start = path;
    for (cursor = path; *cursor; ++cursor) {
        unsigned char byte = (unsigned char)*cursor;

        if (byte < 0x20 || byte == 0x7f)
            return 0;
        if (*cursor == '/' || *cursor == ':')
            start = cursor + 1;
    }
    while (start[count])
        ++count;
    if (count == 0 || (count == 1 && start[0] == '.') ||
        (count == 2 && start[0] == '.' && start[1] == '.'))
        return 0;
    *leaf = start;
    *length = count;
    return 1;
}

static uint8_t fold_ascii(uint8_t byte)
{
    if (byte >= 'A' && byte <= 'Z')
        return (uint8_t)(byte + ('a' - 'A'));
    return byte;
}

static enum CAMDIdentityMapResult make_key(
    const char *module_path, uint32_t namespace_id, int include_port,
    uint32_t port_index, struct CAMDIdentityKeyV1 *key)
{
    const char *leaf;
    size_t length, i, required;

    if (!key || !module_leaf(module_path, &leaf, &length))
        return CAMD_IDENTITY_MAP_INVALID;
    required = 2 + length + (include_port ? 4 : 0);
    if (length > 255 || required > sizeof(key->Bytes))
        return CAMD_IDENTITY_MAP_INVALID;
    memset(key, 0, sizeof(*key));
    key->Size = sizeof(*key);
    key->Version = 1;
    key->Namespace = namespace_id;
    key->Confidence = CAMD_IDENTITY_PATH_BOUND;
    key->Bytes[0] = 1; /* key format version */
    key->Bytes[1] = (uint8_t)length;
    for (i = 0; i < length; ++i)
        key->Bytes[2 + i] = fold_ascii((uint8_t)leaf[i]);
    if (include_port) {
        size_t offset = 2 + length;

        key->Bytes[offset] = (uint8_t)(port_index >> 24);
        key->Bytes[offset + 1] = (uint8_t)(port_index >> 16);
        key->Bytes[offset + 2] = (uint8_t)(port_index >> 8);
        key->Bytes[offset + 3] = (uint8_t)port_index;
    }
    key->ByteCount = (uint32_t)required;
    return CAMD_IDENTITY_MAP_OK;
}

enum CAMDIdentityMapResult camd_legacy_provider_identity_key(
    const char *module_path, struct CAMDIdentityKeyV1 *key)
{
    return make_key(module_path, CAMD_IDENTITY_NAMESPACE_LEGACY_PROVIDER,
                    0, 0, key);
}

enum CAMDIdentityMapResult camd_legacy_endpoint_identity_key(
    const char *module_path, uint32_t port_index,
    struct CAMDIdentityKeyV1 *key)
{
    return make_key(module_path, CAMD_IDENTITY_NAMESPACE_LEGACY_ENDPOINT,
                    1, port_index, key);
}
