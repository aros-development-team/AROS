/* Run the complete descriptor matrix from a pthread. */
#define main matrix_main
#include "vfork_fd.c"
#undef main
#include <pthread.h>
#include <stdint.h>

struct input
{
    int argc;
    char **argv;
};

static void *worker(void *raw)
{
    struct input *input = raw;

    return (void *)(intptr_t)matrix_main(input->argc, input->argv);
}

int main(int argc, char **argv)
{
    struct input input = {argc, argv};
    pthread_t thread;
    void *result;

    if (argc == 4 && strcmp(argv[1], "--child") == 0)
        return child_main(argv);
    TEST(pthread_create(&thread, NULL, worker, &input) == 0);
    TEST(pthread_join(thread, &result) == 0);
    TEST((int)(intptr_t)result == OK);
    return OK;
}
