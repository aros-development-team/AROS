#include <libraries/thread.h>
#include <proto/thread.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdint.h>

void *thread_main(void *data) {
    ThreadIdentifier id = CurrentThread();
    int i;

    printf("[%d] starting\n", id);

    for (i = 0; i < 10; i++) {
        printf("[%d] count: %d\n", id, i);
        Delay(25);
    }

    printf("[%d] exiting\n", id);

    return NULL;
}

int main (int argc, char **argv) {
    ThreadIdentifier t1, t2;

    t1 = CreateThread(thread_main, NULL);
    printf("created thread %d\n", t1);

    Delay(100);

    t2 = CreateThread(thread_main, NULL);
    printf("created thread %d\n", t2);

    printf("waiting for thread %d\n", t2);
    WaitThread(t2, NULL);
    printf("thread %d completed\n", t2);

    printf("waiting for thread %d\n", t1);
    WaitThread(t1, NULL);
    printf("thread %d completed\n", t1);

    return 0;
}
