#ifndef CAMD_IDENTITY_FILE_H
#define CAMD_IDENTITY_FILE_H

/* Architecture-independent on-disk encoding for the private identity map. */

#include "identity_map.h"

#include <stddef.h>
#include <stdint.h>

/* Bytes in an empty FORM CAMD containing its required IDMP chunk. */
#define CAMD_IDENTITY_FILE_HEADER_BYTES 36u
#define CAMD_IDENTITY_FILE_RECORD_BYTES 280u

enum CAMDIdentityFileResult {
    CAMD_IDENTITY_FILE_OK = 0,
    CAMD_IDENTITY_FILE_INVALID,
    CAMD_IDENTITY_FILE_CAPACITY,
    CAMD_IDENTITY_FILE_COLLISION
};

/* Returns zero when record_count cannot be represented by format version 1. */
size_t camd_identity_file_size(size_t record_count);

enum CAMDIdentityFileResult camd_identity_file_encode(
    const struct CAMDIdentityRecordV1 *records, size_t record_count,
    uint8_t *bytes, size_t byte_capacity, size_t *byte_count);

enum CAMDIdentityFileResult camd_identity_file_decode(
    const uint8_t *bytes, size_t byte_count,
    struct CAMDIdentityRecordV1 *records, size_t record_capacity,
    size_t *record_count);
#endif
