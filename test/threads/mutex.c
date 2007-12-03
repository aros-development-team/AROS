#include <libraries/thread.h>
#include <proto/thread.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdint.h>

void *locker_thread(void *data) {
    Mutex mutex = (Mutex) data;
    ThreadIdentifier id = CurrentThread();

    printf("[%d] starting, locking the mutex\n", id);
    LockMutex(mutex);

    printf("[%d] got it, pausing for 5s\n", id);
    Delay(250);

    printf("[%d] unlocking the mutex\n", id);
    UnlockMutex(mutex);

    printf("[%d] all done, exiting\n", id);

    return NULL;
}

void *waiter_thread(void *data) {
    Mutex mutex = (Mutex) data;
    ThreadIdentifier id = CurrentThread();

    printf("[%d] starting, locking the mutex\n", id);
    LockMutex(mutex);

    printf("[%d] got it, unlocking\n", id);
    UnlockMutex(mutex);

    printf("[%d] all done, exiting\n", id);

    return NULL;
}

int main (int argc, char **argv) {
    Mutex mutex;
    ThreadIdentifier tl, tw;

    printf("creating mutex\n");
    mutex = CreateMutex();

    printf("starting locker thread\n");
    tl = CreateThread(locker_thread, (void *) mutex);

    printf("sleeping for 2s\n");
    Delay(100);

    printf("starting waiter thread\n");
    tw = CreateThread(waiter_thread, (void *) mutex);

    printf("waiting for locker thread to exit\n");
    WaitThread(tl, NULL);

    printf("waiting for waiter thread to exit\n");
    WaitThread(tw, NULL);

    printf("destroying the mutex\n");
    DestroyMutex(mutex);

    printf("all done\n");

    return 0;
}
