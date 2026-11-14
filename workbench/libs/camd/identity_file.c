#include "identity_file.h"

#include <limits.h>
#include <string.h>

#define FORM_SIZE_OFFSET 4u
#define FORM_TYPE_OFFSET 8u
#define IDMP_CHUNK_OFFSET 12u
#define IDMP_SIZE_OFFSET 16u
#define IDMP_DATA_OFFSET 20u
#define IDMP_VERSION_OFFSET 20u
#define IDMP_RECORD_SIZE_OFFSET 24u
#define IDMP_COUNT_OFFSET 28u
#define IDMP_RECORDS_OFFSET 32u

static const uint8_t form_magic[4] = { 'F', 'O', 'R', 'M' };
static const uint8_t camd_type[4] = { 'C', 'A', 'M', 'D' };
static const uint8_t idmp_chunk[4] = { 'I', 'D', 'M', 'P' };

static uint32_t read_u32(const uint8_t *bytes)
{
    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3];
}

static void write_u32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)value;
}

static uint32_t crc32(const uint8_t *bytes, size_t byte_count)
{
    uint32_t crc = UINT32_C(0xffffffff);
    size_t i;
    unsigned int bit;

    for (i = 0; i < byte_count; ++i) {
        crc ^= bytes[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^
                  (UINT32_C(0xedb88320) & (uint32_t)-(int32_t)(crc & 1u));
    }
    return ~crc;
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

static int same_key(const struct CAMDIdentityRecordV1 *left,
                    const struct CAMDIdentityRecordV1 *right)
{
    return left->Namespace == right->Namespace &&
           left->KeyByteCount == right->KeyByteCount &&
           memcmp(left->Key, right->Key, left->KeyByteCount) == 0;
}

static int records_collide(const struct CAMDIdentityRecordV1 *records,
                           size_t record_count)
{
    size_t i, j;

    for (i = 0; i < record_count; ++i) {
        for (j = 0; j < i; ++j) {
            if (same_key(&records[i], &records[j]) ||
                id_equal(&records[i].ID, &records[j].ID))
                return 1;
        }
    }
    return 0;
}

size_t camd_identity_file_size(size_t record_count)
{
    size_t size;

    if (record_count > UINT32_MAX ||
        record_count > (SIZE_MAX - CAMD_IDENTITY_FILE_HEADER_BYTES) /
                           CAMD_IDENTITY_FILE_RECORD_BYTES)
        return 0;
    size = CAMD_IDENTITY_FILE_HEADER_BYTES +
           record_count * CAMD_IDENTITY_FILE_RECORD_BYTES;
    if (size - 8 > UINT32_MAX)
        return 0;
    return size;
}

enum CAMDIdentityFileResult camd_identity_file_encode(
    const struct CAMDIdentityRecordV1 *records, size_t record_count,
    uint8_t *bytes, size_t byte_capacity, size_t *byte_count)
{
    size_t required, i, offset;

    if (!byte_count)
        return CAMD_IDENTITY_FILE_INVALID;
    *byte_count = 0;
    required = camd_identity_file_size(record_count);
    if (required == 0 || !bytes ||
        (record_count != 0 && !records))
        return CAMD_IDENTITY_FILE_INVALID;
    if (byte_capacity < required)
        return CAMD_IDENTITY_FILE_CAPACITY;
    for (i = 0; i < record_count; ++i) {
        if (!valid_record(&records[i]))
            return CAMD_IDENTITY_FILE_INVALID;
    }
    if (records_collide(records, record_count))
        return CAMD_IDENTITY_FILE_COLLISION;

    memset(bytes, 0, required);
    memcpy(bytes, form_magic, sizeof(form_magic));
    write_u32(bytes + FORM_SIZE_OFFSET, (uint32_t)(required - 8));
    memcpy(bytes + FORM_TYPE_OFFSET, camd_type, sizeof(camd_type));
    memcpy(bytes + IDMP_CHUNK_OFFSET, idmp_chunk, sizeof(idmp_chunk));
    write_u32(bytes + IDMP_SIZE_OFFSET, (uint32_t)(required - 20));
    write_u32(bytes + IDMP_VERSION_OFFSET, 1);
    write_u32(bytes + IDMP_RECORD_SIZE_OFFSET,
              CAMD_IDENTITY_FILE_RECORD_BYTES);
    write_u32(bytes + IDMP_COUNT_OFFSET, (uint32_t)record_count);
    for (i = 0; i < record_count; ++i) {
        offset = IDMP_RECORDS_OFFSET +
                 i * CAMD_IDENTITY_FILE_RECORD_BYTES;
        write_u32(bytes + offset, records[i].Namespace);
        write_u32(bytes + offset + 4, records[i].KeyByteCount);
        memcpy(bytes + offset + 8, records[i].Key, sizeof(records[i].Key));
        write_u32(bytes + offset + 264, records[i].ID.word[0]);
        write_u32(bytes + offset + 268, records[i].ID.word[1]);
        write_u32(bytes + offset + 272, records[i].ID.word[2]);
        write_u32(bytes + offset + 276, records[i].ID.word[3]);
    }
    offset = IDMP_RECORDS_OFFSET +
             record_count * CAMD_IDENTITY_FILE_RECORD_BYTES;
    write_u32(bytes + offset,
              crc32(bytes + IDMP_DATA_OFFSET, offset - IDMP_DATA_OFFSET));
    *byte_count = required;
    return CAMD_IDENTITY_FILE_OK;
}

enum CAMDIdentityFileResult camd_identity_file_decode(
    const uint8_t *bytes, size_t byte_count,
    struct CAMDIdentityRecordV1 *records, size_t record_capacity,
    size_t *record_count)
{
    size_t count = 0, required, i, offset, data_offset;
    size_t chunk_size, padded_size, idmp_data = 0;
    int found = 0;

    if (!record_count)
        return CAMD_IDENTITY_FILE_INVALID;
    *record_count = 0;
    if (!bytes || byte_count < 12 || byte_count - 8 > UINT32_MAX ||
        memcmp(bytes, form_magic, sizeof(form_magic)) != 0 ||
        read_u32(bytes + FORM_SIZE_OFFSET) != byte_count - 8 ||
        memcmp(bytes + FORM_TYPE_OFFSET, camd_type, sizeof(camd_type)) != 0)
        return CAMD_IDENTITY_FILE_INVALID;

    offset = 12;
    while (offset < byte_count) {
        if (byte_count - offset < 8)
            return CAMD_IDENTITY_FILE_INVALID;
        chunk_size = read_u32(bytes + offset + 4);
        data_offset = offset + 8;
        padded_size = chunk_size + (chunk_size & 1u);
        if (padded_size < chunk_size || padded_size > byte_count - data_offset)
            return CAMD_IDENTITY_FILE_INVALID;
        if (memcmp(bytes + offset, idmp_chunk, sizeof(idmp_chunk)) == 0) {
            if (found || chunk_size < 16 ||
                read_u32(bytes + data_offset) != 1 ||
                read_u32(bytes + data_offset + 4) !=
                    CAMD_IDENTITY_FILE_RECORD_BYTES)
                return CAMD_IDENTITY_FILE_INVALID;
            count = read_u32(bytes + data_offset + 8);
            if (count > (SIZE_MAX - 16) / CAMD_IDENTITY_FILE_RECORD_BYTES)
                return CAMD_IDENTITY_FILE_INVALID;
            required = 16 + count * CAMD_IDENTITY_FILE_RECORD_BYTES;
            if (required != chunk_size ||
                read_u32(bytes + data_offset + required - 4) !=
                    crc32(bytes + data_offset, required - 4))
                return CAMD_IDENTITY_FILE_INVALID;
            if (count > record_capacity)
                return CAMD_IDENTITY_FILE_CAPACITY;
            if (count != 0 && !records)
                return CAMD_IDENTITY_FILE_INVALID;
            idmp_data = data_offset;
            found = 1;
        }
        offset = data_offset + padded_size;
    }
    if (!found || offset != byte_count)
        return CAMD_IDENTITY_FILE_INVALID;

    for (i = 0; i < count; ++i) {
        offset = idmp_data + 12 + i * CAMD_IDENTITY_FILE_RECORD_BYTES;
        memset(&records[i], 0, sizeof(records[i]));
        records[i].Size = sizeof(records[i]);
        records[i].Version = 1;
        records[i].Namespace = read_u32(bytes + offset);
        records[i].KeyByteCount = read_u32(bytes + offset + 4);
        memcpy(records[i].Key, bytes + offset + 8, sizeof(records[i].Key));
        records[i].ID.word[0] = read_u32(bytes + offset + 264);
        records[i].ID.word[1] = read_u32(bytes + offset + 268);
        records[i].ID.word[2] = read_u32(bytes + offset + 272);
        records[i].ID.word[3] = read_u32(bytes + offset + 276);
        if (!valid_record(&records[i])) {
            memset(records, 0, count * sizeof(*records));
            return CAMD_IDENTITY_FILE_INVALID;
        }
    }
    if (records_collide(records, count)) {
        memset(records, 0, count * sizeof(*records));
        return CAMD_IDENTITY_FILE_COLLISION;
    }
    *record_count = count;
    return CAMD_IDENTITY_FILE_OK;
}
