/* Closing a duplicate must not block a writer behind the descriptor table. */
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <proto/dos.h>
#include "test.h"

extern int __get_default_file(int, long *);

static int channel[2] = {-1, -1};
static int duplicate = -1;
static BPTR raw_writer;
static atomic_int reading;
static atomic_int closing;
static long close_ms;
static int close_rc, write_rc, read_rc;
static pthread_t threads[4];
static unsigned int started;

static long now_ms(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return -1;
    return now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static void *reader(void *unused)
{
    char byte;

    (void)unused;
    atomic_store(&reading, 1);
    read_rc = read(channel[0], &byte, 1);
    return NULL;
}

static void *closer(void *unused)
{
    long start;
    long end;

    (void)unused;
    start = now_ms();
    atomic_store(&closing, 1);
    close_rc = close(duplicate);
    if (close_rc == 0)
        duplicate = -1;
    end = now_ms();
    close_ms = start >= 0 && end >= start ? end - start : -1;
    return NULL;
}

static void *writer(void *unused)
{
    (void)unused;
    write_rc = write(channel[1], "W", 1);
    return NULL;
}

static void *rescue(void *unused)
{
    (void)unused;
    Delay(30);
    Write(raw_writer, (APTR)"R", 1);
    return NULL;
}

static int wait_ready(atomic_int *flag)
{
    unsigned int polls;

    for (polls = 0; polls < 300; ++polls)
    {
        if (atomic_load(flag))
            return 0;
        Delay(1);
    }
    return -1;
}

void cleanup(void)
{
    unsigned int i;

    /* Rescue starts first and bypasses posixc, including its table lock. */
    for (i = 0; i < started; ++i)
        pthread_join(threads[i], NULL);
    started = 0;
    if (duplicate >= 0)
    {
        close(duplicate);
        duplicate = -1;
    }
    for (i = 0; i < 2; ++i)
    {
        if (channel[i] >= 0)
        {
            close(channel[i]);
            channel[i] = -1;
        }
    }
}

int main(void)
{
    long handle = 0;

    TEST(pipe(channel) == 0);
    duplicate = dup(channel[0]);
    TEST(duplicate >= 0);
    TEST(__get_default_file(channel[1], &handle) == 0);
    raw_writer = (BPTR)handle;
    TEST(pthread_create(&threads[started], NULL, rescue, NULL) == 0);
    ++started;
    TEST(pthread_create(&threads[started], NULL, reader, NULL) == 0);
    ++started;
    TEST(wait_ready(&reading) == 0);
    Delay(5);
    TEST(pthread_create(&threads[started], NULL, closer, NULL) == 0);
    ++started;
    TEST(wait_ready(&closing) == 0);
    Delay(3);
    TEST(pthread_create(&threads[started], NULL, writer, NULL) == 0);
    ++started;
    cleanup();
    printf("close=%d duration=%ldms read=%d write=%d\n",
        close_rc, close_ms, read_rc, write_rc);
    TEST(close_rc == 0);
    TEST(close_ms >= 0 && close_ms < 300);
    TEST(read_rc == 1 && write_rc == 1);
    return OK;
}
