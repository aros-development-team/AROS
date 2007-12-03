#include <libraries/thread.h>
#include <proto/thread.h>
#include <proto/dos.h>
#include <stdio.h>

void *thread_main(void *data) {
    int i;

    printf("thread starting\n");

    for (i = 0; i < 10; i++) {
        printf("count: %d\n", i);
        Delay(25);
    }

    printf("thread exiting\n");

    return NULL;
}

int main (int argc, char **argv) {
    CreateThread(thread_main, NULL);
    Delay(100);
    return 0;
}
