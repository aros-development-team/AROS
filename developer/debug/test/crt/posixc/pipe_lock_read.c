/* OpenFromLock must initialize its PIPEKEY in blocking mode. */
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <dos/dosextens.h>
#include <dos/pipe.h>
#include <proto/dos.h>
#include "test.h"

static BPTR writer, seed, lock, reader;
static pthread_t thread;
static int started;
static atomic_int written;

static long now_ms(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return -1;
    return now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static void *produce(void *unused)
{
    (void)unused;
    Write(writer, (APTR)"X", 1);
    atomic_store(&written, 1);
    Delay(15);
    Close(writer);
    writer = 0;
    return NULL;
}

void cleanup(void)
{
    if (started)
    {
        pthread_join(thread, NULL);
        started = 0;
    }
    if (reader) { Close(reader); reader = 0; }
    if (seed) { Close(seed); seed = 0; }
    if (lock) { UnLock(lock); lock = 0; }
    if (writer) { Close(writer); writer = 0; }
}

int main(void)
{
    char name[80], data[64];
    struct FileHandle *handle;
    unsigned int polls;
    LONG result;
    long start, end;

    snprintf(name, sizeof(name), "PIPE:lock-read-%ld", (long)getpid());
    writer = Open((CONST_STRPTR)name, MODE_NEWFILE);
    TEST(writer != 0);
    /* Recycle a nonblocking key before exercising the third constructor. */
    seed = Open((CONST_STRPTR)name, MODE_OLDFILE);
    TEST(seed != 0);
    handle = BADDR(seed);
    TEST(DoPkt(handle->fh_Type, ACTION_PIPE_SET_NONBLOCK,
        handle->fh_Arg1, 1, 0, 0, 0) == 1);
    Close(seed);
    seed = 0;
    lock = Lock((CONST_STRPTR)name, SHARED_LOCK);
    TEST(lock != 0);
    reader = OpenFromLock(lock);
    TEST(reader != 0);
    lock = 0;
    TEST(pthread_create(&thread, NULL, produce, NULL) == 0);
    started = 1;
    for (polls = 0; polls < 300 && !atomic_load(&written); ++polls)
        Delay(1);
    TEST(atomic_load(&written));
    start = now_ms();
    TEST(start >= 0);
    result = Read(reader, data, sizeof(data));
    end = now_ms();
    cleanup();
    printf("PIPE_LOCK_READ n=%ld byte=%d elapsed_ms=%ld\n",
        (long)result, result > 0 ? data[0] : 0, end - start);
    TEST(result == 1 && data[0] == 'X');
    TEST(end >= start && end - start >= 200);
    return OK;
}
