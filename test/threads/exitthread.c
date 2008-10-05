#include <proto/thread.h>
#include <proto/dos.h>
#include <stdio.h>

void *thread_sub(void *data) {
    uint32_t id = CurrentThread();

    printf("[%d] starting sub\n", id);

    Delay(50);

    printf("[%d] exiting sub\n", id);

    ExitThread((void*) id);

    return NULL;
}

void *thread_main(void *data) {
    int i;
    uint32_t id_sub[10], ret;
    uint32_t id = CurrentThread();

    printf("[%d] starting\n", id);

    Delay(50);

    for (i = 0; i < 10; i++) {
        id_sub[i] = CreateThread(thread_sub, NULL);
        printf("created sub thread %d\n", id_sub[i]);
    }

    printf("[%d] exiting\n", id);

    for (i = 0; i < 10; i++) {
        printf("waiting for sub thread %d\n", id_sub[i]);
        WaitThread(id_sub[i], (void **) &ret);
        printf("sub thread %d return %d\n", id_sub[i], ret);
    }

    ExitThread((void*) id);

    return NULL;
}

int main (int argc, char **argv) {
    int i;
    uint32_t id[10], ret;

    for (i = 0; i < 10; i++) {
        id[i] = CreateThread(thread_main, NULL);
        printf("created thread %d\n", id[i]);
    }

    for (i = 0; i < 10; i++) {
        printf("waiting for thread %d\n", id[i]);
        WaitThread(id[i], (void **) &ret);
        printf("thread %d return %d\n", id[i], ret);
    }

    return 0;
}
