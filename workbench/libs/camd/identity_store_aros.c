#include "identity_store_aros.h"

#include "identity_file.h"
#include "identity_map.h"
#include "identity_store_recovery.h"

#include <aros/debug.h>
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

struct CAMDIdentityStoreAROS {
    struct CAMDIdentityMap *map;
    struct Library *uuid_base;
    UBYTE *file_bytes;
    ULONG file_capacity;
    /* No complete snapshot could be read: leave the files as they are. */
    int unreadable;
    /* The slot the records came from, or -1 when there was no file. */
    int loaded_slot;
};

/*
 * Test hook, compiled in with CAMD_TEST_HOOKS only (see mmakefile.src). The
 * variable CAMD_IDENTITY_FAULT holds a number n: the n-th call that changes
 * a file fails once, counted from the creation of the store. A failed Write
 * leaves half of the data behind, a failed Close still closes, and the
 * others do nothing. tools/camd-compat-hosted.sh fails each call in turn
 * and checks that no stored ID is lost.
 */
#ifdef CAMD_TEST_HOOKS
static LONG fault_at, fault_calls;

static void fault_setup(void)
{
    char text[16];
    LONG value = 0;

    fault_at = fault_calls = 0;
    if (GetVar("CAMD_IDENTITY_FAULT", text, sizeof(text), GVF_GLOBAL_ONLY) > 0 &&
        StrToLong(text, &value) > 0 && value > 0) {
        fault_at = value;
        bug("camd.library: CAMD_IDENTITY_FAULT fails file call %ld\n",
            (long)value);
    }
}

static int fault_now(const char *what)
{
    if (fault_at == 0 || ++fault_calls != fault_at)
        return 0;
    bug("camd.library: CAMD_IDENTITY_FAULT failed %s (call %ld)\n", what,
        (long)fault_calls);
    return 1;
}
#else
#define fault_setup()
#define fault_now(what) 0
#endif

struct recovery_context {
    struct CAMDIdentityStoreAROS *store;
    struct CAMDIdentityRecordV1 *records;
    size_t *record_count;
};

static CONST_STRPTR slot_path(enum CAMDIdentityStoreSlot slot)
{
    static CONST_STRPTR const paths[CAMD_IDENTITY_STORE_SLOT_COUNT] = {
        IDENTITY_PATH,
        IDENTITY_NEW_PATH,
        IDENTITY_BACKUP_PATH
    };

    return slot >= CAMD_IDENTITY_STORE_MAIN &&
                   slot < CAMD_IDENTITY_STORE_SLOT_COUNT
               ? paths[slot]
               : NULL;
}

static int path_exists(CONST_STRPTR path)
{
    BPTR lock = Lock(path, ACCESS_READ);

    if (!lock)
        return 0;
    UnLock(lock);
    return 1;
}

static enum CAMDIdentityStoreLoadState load_path(
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
        return IoErr() == ERROR_OBJECT_NOT_FOUND
                   ? CAMD_IDENTITY_STORE_MISSING
                   : CAMD_IDENTITY_STORE_INVALID;
    got = Read(file, bytes, (LONG)byte_capacity);
    extra = got >= 0 ? Read(file, &trailing, 1) : -1;
    Close(file);
    if (got < 0 || extra != 0)
        return CAMD_IDENTITY_STORE_INVALID;
    if (camd_identity_file_decode(bytes, (size_t)got, records,
                                  record_capacity, record_count) !=
        CAMD_IDENTITY_FILE_OK)
        return CAMD_IDENTITY_STORE_INVALID;
    return CAMD_IDENTITY_STORE_VALID;
}

static enum CAMDIdentityStoreLoadState recovery_load(
    void *opaque, enum CAMDIdentityStoreSlot slot)
{
    struct recovery_context *context = opaque;
    enum CAMDIdentityStoreLoadState state;

    state = load_path(slot_path(slot), context->store->file_bytes,
                      context->store->file_capacity, context->records,
                      IDENTITY_CAPACITY, context->record_count);
    if (state == CAMD_IDENTITY_STORE_VALID)
        context->store->loaded_slot = (int)slot;
    return state;
}

static int recovery_exists(void *opaque, enum CAMDIdentityStoreSlot slot)
{
    (void)opaque;
    return path_exists(slot_path(slot));
}

static int recovery_remove(void *opaque, enum CAMDIdentityStoreSlot slot)
{
    (void)opaque;
    if (fault_now("DeleteFile"))
        return 0;
    return DeleteFile(slot_path(slot));
}

static int recovery_move(void *opaque, enum CAMDIdentityStoreSlot from,
                         enum CAMDIdentityStoreSlot to)
{
    (void)opaque;
    if (fault_now("Rename"))
        return 0;
    return Rename(slot_path(from), slot_path(to));
}

static void recovery_ops(struct CAMDIdentityStoreOpsV1 *ops,
                         struct recovery_context *context)
{
    memset(ops, 0, sizeof(*ops));
    ops->Size = sizeof(*ops);
    ops->Version = 1;
    ops->Context = context;
    ops->Load = recovery_load;
    ops->Exists = recovery_exists;
    ops->Remove = recovery_remove;
    ops->Move = recovery_move;
}

static int load_recoverable(struct CAMDIdentityStoreAROS *store,
                            struct CAMDIdentityRecordV1 *records,
                            size_t *record_count)
{
    struct recovery_context context = { store, records, record_count };
    struct CAMDIdentityStoreOpsV1 ops;

    *record_count = 0;
    recovery_ops(&ops, &context);
    return camd_identity_store_recover(&ops);
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
    BPTR file;
    LONG written;
    int ok;

    if (fault_now("Open"))
        return 0;
    file = Open(IDENTITY_NEW_PATH, MODE_NEWFILE);
    if (!file)
        return 0;
    if (fault_now("Write"))
        written = Write(file, store->file_bytes, (LONG)(byte_count / 2));
    else
        written = Write(file, store->file_bytes, (LONG)byte_count);
    ok = written == (LONG)byte_count && !fault_now("Flush") && Flush(file);
    if (!Close(file) || (ok && fault_now("Close")))
        ok = 0;
    /* A fault may leave the half-written file: recovery has to cope. */
    if (!ok && !fault_now("DeleteFile"))
        DeleteFile(IDENTITY_NEW_PATH);
    return ok;
}

static int install_new(void)
{
    struct CAMDIdentityStoreOpsV1 ops;

    recovery_ops(&ops, NULL);
    return camd_identity_store_install(&ops);
}

static int commit_records(void *context,
                          const struct CAMDIdentityRecordV1 *records,
                          size_t record_count)
{
    struct CAMDIdentityStoreAROS *store = context;
    size_t byte_count;

    if (store->unreadable)
        return 0;
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
    if (!store->file_bytes || !store->uuid_base)
        goto fail;
    store->loaded_slot = -1;
    fault_setup();
    /* Files that exist but hold no complete snapshot may be damaged or just
     * unreadable now. Writing a new one over them could lose every stored
     * ID, so this run gives temporary IDs instead and touches nothing. */
    if (!load_recoverable(store, records, &record_count)) {
        store->unreadable = 1;
        record_count = 0;
        bug("camd.library: " IDENTITY_PATH
            " has no complete copy; endpoint IDs are temporary\n");
    }
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

unsigned int camd_identity_store_aros_state(
    struct CAMDIdentityStoreAROS *store)
{
    if (!store)
        return CAMD_IDENTITY_STORE_ABSENT;
    if (store->unreadable)
        return CAMD_IDENTITY_STORE_UNREADABLE;
    if (store->loaded_slot > (int)CAMD_IDENTITY_STORE_MAIN)
        return CAMD_IDENTITY_STORE_RECOVERED;
    return CAMD_IDENTITY_STORE_COMPLETE;
}
