/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
/* Host-only packet-contract tests around the actual NIL handler.
 * The types/allocator below are boundary doubles, not target ABI coverage.
 * See README.nil for FAST/shifted BPTR builds and scope limits. */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef intptr_t SIPTR;
typedef int32_t LONG;
typedef void *APTR;
#ifdef AROS_FAST_BPTR
typedef APTR BPTR;
#define MKBADDR(p) ((BPTR)(p))
#define BADDR(p) ((APTR)(uintptr_t)(p))
#define BPTR_MODE "fast"
#else
typedef uintptr_t BPTR;
#define MKBADDR(p) ((BPTR)((uintptr_t)(p) >> 2))
#define BADDR(p) ((APTR)((uintptr_t)(p) << 2))
#define BPTR_MODE "shifted"
#endif
#define BNULL 0
#define TRUE 1
#define DOSFALSE 0
#define DOSTRUE (-1)
#define SHARED_LOCK (-2)
#define MEMF_PUBLIC 1
#define MEMF_CLEAR (1UL << 16)
#define ACTION_FREE_LOCK 15
#define ACTION_INFO 26
#define ACTION_WRITE 'W'
#define ACTION_FINDUPDATE 1004
#define ACTION_FINDINPUT 1005
#define ACTION_FINDOUTPUT 1006
#define ACTION_FH_FROM_LOCK 1026
#define ACTION_COPY_DIR_FH 1030
#define ACTION_PARENT_FH 1031

struct FileHandle {
    uint32_t fh_Flags;
    LONG fh_Interactive;
    APTR fh_Type;
};
struct FileLock {
    BPTR fl_Link;
    uintptr_t fl_Key;
    LONG fl_Access;
    APTR fl_Task;
    BPTR fl_Volume;
};
static union {
    uintptr_t align;
    struct FileLock value;
} lock_storage;
static struct FileHandle destination;
static unsigned allocations, releases, failures;
static APTR released_address;
static size_t released_size;

static APTR AllocMem(size_t size, unsigned long flags)
{
    assert(size == sizeof(struct FileLock));
    assert(flags == (MEMF_PUBLIC | MEMF_CLEAR));
    assert(allocations++ == 0);
    memset(&lock_storage, 0, sizeof(lock_storage));
    return &lock_storage.value;
}

/* A spy: do not free/dereference an incorrect address from the old handler.
 * This makes NULL/wrong-BPTR releases deterministic rather than UB. */
static void FreeMem(APTR address, size_t size)
{
    ++releases;
    released_address = address;
    released_size = size;
}

#define DOS_INTERN_H
#ifndef NIL_SOURCE
#define NIL_SOURCE "../../../../rom/dos/nil.c"
#endif
#include NIL_SOURCE

static void check(int condition, const char *description)
{
    if (!condition) {
        fprintf(stderr, "FAIL %s: %s\n", BPTR_MODE, description);
        ++failures;
    }
}

static BPTR fresh_lock(void)
{
    BPTR lock;
    allocations = releases = 0;
    released_address = NULL;
    released_size = 0;
    lock = (BPTR)handleNIL(ACTION_COPY_DIR_FH, 0, 0, 0);
    assert(allocations == 1 && BADDR(lock) == &lock_storage.value);
    assert(lock_storage.value.fl_Access == SHARED_LOCK);
    assert(lock_storage.value.fl_Task == NULL);
    return lock;
}

static void from_lock(void)
{
    BPTR lock = fresh_lock();
    unsigned char original[sizeof(struct FileLock)];
    SIPTR result;
    unsigned before = failures;

    /* A nonzero sentinel exposes writes directed at the source lock instead
     * of the supplied destination; the FreeMem spy retains its bytes. */
    memset(&lock_storage.value, 0x6b, sizeof(lock_storage.value));
    memcpy(original, &lock_storage.value, sizeof(original));
    memset(&destination, 0, sizeof(destination));
    destination.fh_Interactive = 0x12345678;
    result = handleNIL(ACTION_FH_FROM_LOCK,
                      (SIPTR)MKBADDR(&destination), (SIPTR)lock, 0);

    check(result == DOSTRUE, "FH_FROM_LOCK success");
    check(destination.fh_Interactive == DOSFALSE, "destination initialized");
    check(releases == 1 && released_address == BADDR(lock) &&
          released_size == sizeof(struct FileLock), "source lock consumed exactly once");
    check(memcmp(original, &lock_storage.value, sizeof(original)) == 0,
          "source lock not used as destination");
    printf("NIL %s FH_FROM_LOCK %s\n", BPTR_MODE,
           before == failures ? "PASS" : "FAIL");
}

static void free_lock(void)
{
    BPTR lock = fresh_lock();
    unsigned before = failures;
    (void)handleNIL(ACTION_FREE_LOCK, (SIPTR)lock, 0, 0);
    check(releases == 1 && released_address == BADDR(lock) &&
          released_size == sizeof(struct FileLock), "FREE_LOCK decodes BPTR");
    /* Return convention is deliberately outside this pointer-contract test. */
    printf("NIL %s FREE_LOCK %s\n", BPTR_MODE,
           before == failures ? "PASS" : "FAIL");
}

int main(void)
{
    assert((uintptr_t)&lock_storage.value % 4 == 0);
    assert((uintptr_t)&destination % 4 == 0);
    from_lock();
    free_lock();
    return failures ? 1 : 0;
}
