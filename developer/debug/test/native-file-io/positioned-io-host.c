/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
 * Licensed under the AROS Public License (APL), Version 1.1. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
typedef int64_t QUAD;
#define OFFSET_BEGINNING 0
#define _FCB_ISDIR 1U
#define FLUSHONREADCHECK do {} while (0);
typedef struct {
    pthread_mutex_t lock;
    int flags;
    unsigned privflags;
    QUAD position, size;
} fcb;
typedef struct { fcb *fcb; } fdesc;
static fcb file;
static fdesc descriptors[2] = {{&file}, {&file}};
static _Thread_local int ioerror;
static int fail_position, fail_size, fail_seek, fail_read, fail_write, fail_restore;
static int seeks;
static QUAD original, transferred;
static unsigned char storage[4096];
static int worker_write;
static pthread_mutex_t gate_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gate_cond = PTHREAD_COND_INITIALIZER;
static atomic_int pause_seek;
static int entered, release_seek, attempted, protected;
static fdesc *__getfdesc(int fd) { return fd == 7 || fd == 8 ? &descriptors[fd - 7] : NULL; }
static void __fcb_lock(fcb *f) { assert(!pthread_mutex_lock(&f->lock)); }
static void __fcb_unlock(fcb *f) { assert(!pthread_mutex_unlock(&f->lock)); }
static int IoErr(void) { return ioerror; }
static int __stdc_ioerr2errno(int error) { return error; }
static QUAD __dos64_getpos(fcb *f)
{
    __fcb_lock(f);
    QUAD result = f->position;
    if (fail_position) { result = -1; ioerror = EIO; }
    __fcb_unlock(f);
    return result;
}
static QUAD __dos64_getsize(fcb *f)
{
    __fcb_lock(f);
    QUAD result = f->size;
    if (fail_size) { f->position = 90; result = -1; ioerror = EIO; }
    __fcb_unlock(f);
    return result;
}
static QUAD __dos64_seek(fcb *f, QUAD position, int mode)
{
    assert(mode == OFFSET_BEGINNING);
    __fcb_lock(f);
    QUAD result = f->position;
    seeks++;
    if ((fail_seek && position == 8) || (fail_restore && position == original)) {
        f->position = 99; result = -1; ioerror = fail_restore ? ESPIPE : EOVERFLOW;
    } else f->position = position;
    __fcb_unlock(f);
    if (position == 8 && atomic_exchange(&pause_seek, 0)) {
        assert(!pthread_mutex_lock(&gate_lock));
        entered = 1; pthread_cond_broadcast(&gate_cond);
        while (!release_seek) pthread_cond_wait(&gate_cond, &gate_lock);
        assert(!pthread_mutex_unlock(&gate_lock));
    }
    return result;
}
static QUAD __dos64_read(fcb *f, void *buffer, QUAD count)
{
    __fcb_lock(f);
    transferred = count;
    QUAD result = count > 8 ? 8 : count;
    if (result > f->size - f->position) result = f->size - f->position;
    if (result < 0) result = 0;
    for (QUAD i = 0; i < result; i++) ((unsigned char *)buffer)[i] = f->position + i < (QUAD)sizeof(storage)
        ? storage[f->position + i] : (unsigned char)(f->position + i);
    f->position += result;
    if (fail_read) { result = -1; ioerror = EIO; }
    __fcb_unlock(f);
    return result;
}
static QUAD __dos64_write(fcb *f, const void *buffer, QUAD count)
{
    __fcb_lock(f);
    transferred = count;
    QUAD result = count > 8 ? 8 : count;
    for (QUAD i = 0; i < result; i++)
        if (f->position + i < (QUAD)sizeof(storage)) storage[f->position + i] = ((const unsigned char *)buffer)[i];
    f->position += result;
    if (f->position > f->size) f->size = f->position;
    if (fail_write) { result = -1; ioerror = EIO; }
    __fcb_unlock(f);
    return result;
}
#include "production-pread.c"
#include "production-pwrite.c"
static void reset(void)
{
    fail_position = fail_size = fail_seek = fail_read = fail_write = fail_restore = seeks = 0;
    transferred = -1;
    file.flags = O_RDWR; file.privflags = 0;
    original = file.position = 3; file.size = 256;
    for (size_t i = 0; i < sizeof(storage); i++) storage[i] = (unsigned char)i;
}
static void *positioned(void *unused)
{
    (void)unused;
    unsigned char bytes[8];
    if (worker_write) {
        memset(bytes, 42, sizeof(bytes));
        assert(aros_test_pwrite64(7, bytes, sizeof(bytes), 8) == 8);
    } else {
        assert(aros_test_pread64(7, bytes, sizeof(bytes), 8) == 8);
        for (int i = 0; i < 8; i++) assert(bytes[i] == 8 + i);
    }
    return NULL;
}
static void *ordinary(void *unused)
{
    (void)unused;
    int status = pthread_mutex_trylock(&file.lock);
    if (!status) pthread_mutex_unlock(&file.lock);
    assert(!pthread_mutex_lock(&gate_lock));
    protected = status == EBUSY; attempted = 1;
    pthread_cond_broadcast(&gate_cond);
    assert(!pthread_mutex_unlock(&gate_lock));
    unsigned char byte;
    assert(__dos64_read(&file, &byte, 1) == 1 && byte == 3);
    return NULL;
}
static void *parallel(void *value)
{
    QUAD offset = (intptr_t)value;
    for (int n = 0; n < 500; n++) {
        unsigned char bytes[8];
        assert(aros_test_pread64(n & 1 ? 7 : 8, bytes, 8, offset) == 8);
        for (int i = 0; i < 8; i++) assert(bytes[i] == (unsigned char)(offset + i));
    }
    return NULL;
}
int main(void)
{
    pthread_mutexattr_t attr;
    assert(!pthread_mutexattr_init(&attr));
    assert(!pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE));
    assert(!pthread_mutex_init(&file.lock, &attr));
    pthread_mutexattr_destroy(&attr);
    unsigned char bytes[8];
    reset(); assert(aros_test_pread64(-1, bytes, 8, 0) == -1 && errno == EBADF);
    assert(aros_test_pread64(7, bytes, 8, -1) == -1 && errno == EINVAL);
    assert(aros_test_pread64(7, NULL, 8, 0) == -1 && errno == EFAULT);
    file.flags = O_WRONLY;
    assert(aros_test_pread64(7, bytes, 8, 0) == -1 && errno == EBADF);
    reset(); file.privflags = _FCB_ISDIR;
    assert(aros_test_pread64(7, bytes, 8, 0) == -1 && errno == EISDIR);
    reset(); assert(aros_test_pread64(7, NULL, 0, 8) == 0 && !seeks && file.position == 3);
    assert(aros_test_pread64(7, bytes, 8, 256) == 0 && !seeks && file.position == 3 && file.size == 256);
    assert(aros_test_pread64(7, bytes, 8, 260) == 0 && !seeks && file.size == 256);
    assert(aros_test_pread64(7, bytes, 8, 254) == 2 && bytes[0] == 254 && bytes[1] == 255 && file.position == 3);
    reset(); file.size = (INT64_C(1) << 33) + 128;
    assert(aros_test_pread64(7, bytes, 8, (INT64_C(1) << 33) + 4) == 8 && bytes[0] == 4 && file.position == 3);
    reset(); assert(aros_test_pread64(7, bytes, SIZE_MAX, 8) == 8 && transferred == INT64_MAX);
    reset(); fail_position = 1;
    assert(aros_test_pread64(7, bytes, 8, 8) == -1 && errno == EIO && !seeks);
    reset(); fail_size = 1;
    assert(aros_test_pread64(7, bytes, 8, 8) == -1 && errno == EIO && file.position == 3);
    reset(); fail_seek = 1;
    assert(aros_test_pread64(7, bytes, 8, 8) == -1 && errno == EOVERFLOW && file.position == 3);
    reset(); fail_read = 1;
    assert(aros_test_pread64(7, bytes, 8, 8) == -1 && errno == EIO && file.position == 3);
    reset(); fail_restore = 1;
    assert(aros_test_pread64(7, bytes, 8, 8) == -1 && errno == ESPIPE);
    reset(); fail_read = fail_restore = 1;
    assert(aros_test_pread64(7, bytes, 8, 8) == -1 && errno == EIO);
    for (int writing = 0; writing < 2; writing++) {
        reset(); worker_write = writing; atomic_store(&pause_seek, 1);
        entered = release_seek = attempted = protected = 0;
        pthread_t reader, other;
        assert(!pthread_create(&reader, NULL, positioned, NULL));
        assert(!pthread_mutex_lock(&gate_lock));
        while (!entered) pthread_cond_wait(&gate_cond, &gate_lock);
        assert(!pthread_create(&other, NULL, ordinary, NULL));
        while (!attempted) pthread_cond_wait(&gate_cond, &gate_lock);
        assert(protected);
        release_seek = 1; pthread_cond_broadcast(&gate_cond);
        assert(!pthread_mutex_unlock(&gate_lock));
        assert(!pthread_join(reader, NULL)); assert(!pthread_join(other, NULL));
        assert(file.position == 4);
    }
    reset(); pthread_t threads[4];
    for (intptr_t i = 0; i < 4; i++) assert(!pthread_create(&threads[i], NULL, parallel, (void *)(i * 16)));
    for (int i = 0; i < 4; i++) assert(!pthread_join(threads[i], NULL));
    assert(file.position == 3);
    reset(); memset(bytes, 42, sizeof(bytes));
    assert(aros_test_pwrite64(-1, bytes, 8, 0) == -1 && errno == EBADF);
    assert(aros_test_pwrite64(7, bytes, 8, -1) == -1 && errno == EINVAL);
    assert(aros_test_pwrite64(7, NULL, 8, 0) == -1 && errno == EFAULT);
    file.flags = O_RDONLY;
    assert(aros_test_pwrite64(7, bytes, 8, 8) == -1 && errno == EBADF);
    reset(); file.privflags = _FCB_ISDIR;
    assert(aros_test_pwrite64(7, bytes, 8, 8) == -1 && errno == EISDIR);
    reset(); assert(aros_test_pwrite64(7, NULL, 0, 260) == 0 && !seeks && file.size == 256 && file.position == 3);
    assert(aros_test_pwrite64(7, bytes, 8, 8) == 8 && file.position == 3 && !memcmp(storage + 8, bytes, 8));
    reset(); file.flags |= O_APPEND;
    assert(aros_test_pwrite64(7, bytes, 8, 8) == 8 && !memcmp(storage + 8, bytes, 8) && file.size == 256 && file.flags == (O_RDWR | O_APPEND));
    reset(); fail_position = 1;
    assert(aros_test_pwrite64(7, bytes, 8, 8) == -1 && errno == EIO && !seeks);
    reset(); fail_seek = 1;
    assert(aros_test_pwrite64(7, bytes, 8, 8) == -1 && errno == EOVERFLOW && file.position == 3 && transferred == -1 && file.size == 256);
    reset(); fail_write = 1;
    assert(aros_test_pwrite64(7, bytes, 8, 8) == -1 && errno == EIO && file.position == 3);
    reset(); fail_restore = 1;
    assert(aros_test_pwrite64(7, bytes, 8, 8) == -1 && errno == ESPIPE);
    reset(); fail_write = fail_restore = 1;
    assert(aros_test_pwrite64(7, bytes, 8, 8) == -1 && errno == EIO);
    reset(); assert(aros_test_pwrite64(7, bytes, SIZE_MAX, 8) == 8 && transferred == INT64_MAX);
    reset(); assert(aros_test_pwrite64(7, bytes, 8, 256) == 8 && file.size == 264 && file.position == 3);
    pthread_mutex_destroy(&file.lock);
    puts("PASS actual pread64/pwrite64: EOF/offset/access/error/position restore, 2000 parallel reads and deterministic ordinary-read exclusion for both operations; DOS modeled");
    return 0;
}
