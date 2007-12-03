#include <exec/memory.h>
#include <libraries/thread.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/thread.h>
#include <stdio.h>
#include <stdint.h>

struct thread_data {
    void *mutex;
    void *cond;
};

void *waiter_thread(void *data) {
    struct thread_data *td = (struct thread_data *) data;
    uint32_t id = CurrentThread();

    printf("[%d] starting, locking the mutex\n", id);
    LockMutex(td->mutex);

    printf("[%d] waiting on the condition\n", id);
    WaitCondition(td->cond, td->mutex);

    printf("[%d] condition signalled, unlocking the mutex\n", id);
    UnlockMutex(td->mutex);

    printf("[%d] all done, exiting\n", id);

    return NULL;
}

int main (int argc, char **argv) {
    struct thread_data *td;
    uint32_t tw;

    td = AllocMem(sizeof(struct thread_data), MEMF_PUBLIC | MEMF_CLEAR);

    printf("creating mutex\n");
    td->mutex = CreateMutex();

    printf("creating condition\n");
    td->cond = CreateCondition();

    printf("starting waiter thread\n");
    tw = CreateThread(waiter_thread, (void *) td);

    printf("sleeping for 2s\n");
    Delay(100);

    printf("signalling condition\n");
    SignalCondition(td->cond);

    printf("waiting for waiter thread\n");
    WaitThread(tw, NULL);

    printf("destroying the condition\n");
    DestroyCondition(td->cond);

    printf("destroying the mutex\n");
    DestroyMutex(td->mutex);

    FreeMem(td, sizeof(struct thread_data));

    printf("all done\n");

    return 0;
}
