#include "identity_store_aros.h"

#include "identity_file.h"
#include "identity_map.h"

#include <dos/dos.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <libraries/uuid.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/uuid.h>

#include <limits.h>
#include <string.h>

#define IDENTITY_CAPACITY 256u
#define IDENTITY_PATH "ENVARC:Sys/camd-identities.iff"
#define IDENTITY_NEW_PATH "ENVARC:Sys/camd-identities.iff.new"
#define IDENTITY_BACKUP_PATH "ENVARC:Sys/camd-identities.iff.bak"

enum load_state {
    LOAD_MISSING = 0,
    LOAD_VALID,
    LOAD_INVALID
};

struct CAMDIdentityStoreAROS {
    struct CAMDIdentityMap *map;
    struct Library *uuid_base;
    UBYTE *file_bytes;
    ULONG file_capacity;
};

static int path_exists(CONST_STRPTR path)
{
    BPTR lock = Lock(path, ACCESS_READ);

    if (!lock)
        return 0;
    UnLock(lock);
    return 1;
}

static enum load_state load_path(
    CONST_STRPTR path, UBYTE *bytes, ULONG byte_capacity,
    struct CAMDIdentityRecordV1 *records, size_t record_capacity,
    size_t *record_count)
{
    BPTR file;
    LONG got, extra;
    UBYTE trailing;

    *record_count = 0;
    file = Open(path, MODE_OLDFILE);
    if (!file)
        return IoErr() == ERROR_OBJECT_NOT_FOUND ? LOAD_MISSING : LOAD_INVALID;
    got = Read(file, bytes, (LONG)byte_capacity);
    extra = got >= 0 ? Read(file, &trailing, 1) : -1;
    Close(file);
    if (got < 0 || extra != 0)
        return LOAD_INVALID;
    if (camd_identity_file_decode(bytes, (size_t)got, records,
                                  record_capacity, record_count) !=
        CAMD_IDENTITY_FILE_OK)
        return LOAD_INVALID;
    return LOAD_VALID;
}

static int replace_invalid_main(CONST_STRPTR candidate,
                                enum load_state main_state)
{
    if (main_state == LOAD_INVALID && !DeleteFile(IDENTITY_PATH))
        return 0;
    if (!Rename(candidate, IDENTITY_PATH))
        return 0;
    DeleteFile(IDENTITY_BACKUP_PATH);
    return 1;
}

static int load_recoverable(struct CAMDIdentityStoreAROS *store,
                            struct CAMDIdentityRecordV1 *records,
                            size_t *record_count)
{
    enum load_state main_state, new_state, backup_state;

    main_state = load_path(IDENTITY_PATH, store->file_bytes,
                           store->file_capacity, records, IDENTITY_CAPACITY,
                           record_count);
    if (main_state == LOAD_VALID)
        return 1;
    new_state = load_path(IDENTITY_NEW_PATH, store->file_bytes,
                          store->file_capacity, records, IDENTITY_CAPACITY,
                          record_count);
    if (new_state == LOAD_VALID)
        return replace_invalid_main(IDENTITY_NEW_PATH, main_state);
    backup_state = load_path(IDENTITY_BACKUP_PATH, store->file_bytes,
                             store->file_capacity, records,
                             IDENTITY_CAPACITY, record_count);
    if (backup_state == LOAD_VALID)
        return replace_invalid_main(IDENTITY_BACKUP_PATH, main_state);
    if (main_state == LOAD_MISSING && new_state == LOAD_MISSING &&
        backup_state == LOAD_MISSING) {
        *record_count = 0;
        return 1;
    }
    return 0;
}

static uint32_t packed_word(const uint8_t *bytes)
{
    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3];
}

static int generate_id(void *context, struct CAMDEndpointIDV1 *id)
{
    struct CAMDIdentityStoreAROS *store = context;
    struct Library *UUIDBase = store->uuid_base;
    uuid_t uuid;
    uint8_t packed[16];
    unsigned int i;

    UUID_Generate(UUID_TYPE_DCE_TIME, &uuid);
    UUID_Pack(&uuid, packed);
    for (i = 0; i < 4; ++i)
        id->word[i] = packed_word(packed + i * 4);
    return 1;
}

static int write_new(struct CAMDIdentityStoreAROS *store, ULONG byte_count)
{
    BPTR file = Open(IDENTITY_NEW_PATH, MODE_NEWFILE);
    LONG written;
    int ok;

    if (!file)
        return 0;
    written = Write(file, store->file_bytes, (LONG)byte_count);
    ok = written == (LONG)byte_count && Flush(file);
    if (!Close(file))
        ok = 0;
    if (!ok)
        DeleteFile(IDENTITY_NEW_PATH);
    return ok;
}

static int install_new(void)
{
    int had_main = path_exists(IDENTITY_PATH);

    if (had_main) {
        if (path_exists(IDENTITY_BACKUP_PATH) &&
            !DeleteFile(IDENTITY_BACKUP_PATH))
            return 0;
        if (!Rename(IDENTITY_PATH, IDENTITY_BACKUP_PATH))
            return 0;
    }
    if (!Rename(IDENTITY_NEW_PATH, IDENTITY_PATH)) {
        if (had_main)
            Rename(IDENTITY_BACKUP_PATH, IDENTITY_PATH);
        return 0;
    }
    if (had_main)
        DeleteFile(IDENTITY_BACKUP_PATH);
    return 1;
}

static int commit_records(void *context,
                          const struct CAMDIdentityRecordV1 *records,
                          size_t record_count)
{
    struct CAMDIdentityStoreAROS *store = context;
    size_t byte_count;

    if (camd_identity_file_encode(records, record_count, store->file_bytes,
                                  store->file_capacity, &byte_count) !=
        CAMD_IDENTITY_FILE_OK)
        return 0;
    if (byte_count > (size_t)LONG_MAX ||
        !write_new(store, (ULONG)byte_count))
        return 0;
    return install_new();
}

struct CAMDIdentityStoreAROS *camd_identity_store_aros_create(void)
{
    struct CAMDIdentityStoreAROS *store;
    struct CAMDIdentityRecordV1 *records;
    struct CAMDIdentityMapConfigV1 config;
    size_t file_capacity, record_count = 0;

    file_capacity = camd_identity_file_size(IDENTITY_CAPACITY);
    if (file_capacity == 0 || file_capacity > ULONG_MAX)
        return NULL;
    store = AllocVec(sizeof(*store), MEMF_ANY | MEMF_CLEAR);
    records = AllocVec(sizeof(*records) * IDENTITY_CAPACITY,
                       MEMF_ANY | MEMF_CLEAR);
    if (!store || !records)
        goto fail;
    store->file_capacity = (ULONG)file_capacity;
    store->file_bytes = AllocVec(store->file_capacity, MEMF_ANY | MEMF_CLEAR);
    store->uuid_base = OpenLibrary("uuid.library", 1);
    if (!store->file_bytes || !store->uuid_base ||
        !load_recoverable(store, records, &record_count))
        goto fail;
    memset(&config, 0, sizeof(config));
    config.Size = sizeof(config);
    config.Version = 1;
    config.Capacity = IDENTITY_CAPACITY;
    config.InitialRecords = records;
    config.InitialRecordCount = record_count;
    config.CallbackContext = store;
    config.Generate = generate_id;
    config.Commit = commit_records;
    if (camd_identity_map_create(&config, &store->map) !=
        CAMD_IDENTITY_MAP_OK)
        goto fail;
    FreeVec(records);
    return store;

fail:
    if (store) {
        if (store->uuid_base)
            CloseLibrary(store->uuid_base);
        if (store->file_bytes)
            FreeVec(store->file_bytes);
        FreeVec(store);
    }
    if (records)
        FreeVec(records);
    return NULL;
}

void camd_identity_store_aros_destroy(struct CAMDIdentityStoreAROS *store)
{
    if (!store)
        return;
    camd_identity_map_destroy(store->map);
    CloseLibrary(store->uuid_base);
    FreeVec(store->file_bytes);
    FreeVec(store);
}

struct CAMDIdentityMap *camd_identity_store_aros_map(
    struct CAMDIdentityStoreAROS *store)
{
    return store ? store->map : NULL;
}
