/* Self-contained descriptor inheritance matrix. */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <proto/dos.h>
#include "test.h"

static int descriptors[2] = {-1, -1};
static pid_t pending_child = -1;

void cleanup(void)
{
    int i;

    for (i = 0; i < 2; ++i)
    {
        if (descriptors[i] >= 0)
        {
            close(descriptors[i]);
            descriptors[i] = -1;
        }
    }
    if (pending_child > 0 && waitpid(pending_child, NULL, WNOHANG) == pending_child)
        pending_child = -1;
}

static int wait_child(int *status)
{
    unsigned int polls;

    for (polls = 0; polls < 300; ++polls)
    {
        pid_t waited = waitpid(pending_child, status, WNOHANG);
        int saved_errno = errno;

        if (waited == pending_child)
        {
            pending_child = -1;
            return 0;
        }
        if (waited < 0)
        {
            printf("waitpid failed: errno=%d\n", saved_errno);
            return -1;
        }
        Delay(1);
    }
    puts("waitpid timed out");
    return -1;
}

static int child_main(char **argv)
{
    int reader = atoi(argv[2]);
    int writer = atoi(argv[3]);
    int result;
    int saved_errno;

    errno = 0;
    result = fcntl(reader, F_GETFD);
    saved_errno = errno;
    TEST(result == -1 && saved_errno == EBADF);
    errno = 0;
    result = fcntl(writer, F_GETFD);
    saved_errno = errno;
    TEST(result == -1 && saved_errno == EBADF);
    return OK;
}

static int run_case(int mode, const char *self)
{
    char reader[24];
    char writer[24];
    int status = -1;
    int result;
    int saved_errno;
    unsigned char byte = 0;
    pid_t child;

    TEST(pipe(descriptors) == 0);
    snprintf(reader, sizeof(reader), "%d", descriptors[0]);
    snprintf(writer, sizeof(writer), "%d", descriptors[1]);
    if (mode == 4)
    {
        TEST(fcntl(descriptors[1], F_SETFD, FD_CLOEXEC) == 0);
    }

    if (mode != 0)
    {
        child = vfork();
        if (child == 0)
        {
            if (mode == 1)
                _exit(0);
            if (mode == 2)
                _exit(write(descriptors[1], "X", 1) == 1 ? 0 : 122);
            close(descriptors[0]);
            if (mode != 4)
                close(descriptors[1]);
            execl(self, self, "--child", reader, writer, (char *)NULL);
            _exit(111);
        }
        TEST(child > 0);
        pending_child = child;
        TEST(wait_child(&status) == 0);
        TEST(WIFEXITED(status));
        TEST(WEXITSTATUS(status) == 0);
    }

    TEST(fcntl(descriptors[0], F_GETFD) >= 0);
    TEST(fcntl(descriptors[1], F_GETFD) >= 0);
    errno = 0;
    result = close(descriptors[1]);
    saved_errno = errno;
    printf("mode=%d close=%d errno=%d\n", mode, result, saved_errno);
    TEST(result == 0);
    descriptors[1] = -1;

    /* The child was reaped and the sole remaining writer has been closed. */
    if (mode == 2)
    {
        TEST(read(descriptors[0], &byte, 1) == 1);
        TEST(byte == 'X');
    }
    TEST(read(descriptors[0], &byte, 1) == 0);
    cleanup();
    return OK;
}

int main(int argc, char **argv)
{
    char self[512];
    const char *filename;
    int mode;
    int length;

    if (argc == 4 && strcmp(argv[1], "--child") == 0)
        return child_main(argv);
    filename = (const char *)FilePart((CONST_STRPTR)argv[0]);
    TEST(filename != NULL && filename[0] != '\0');
    length = snprintf(self, sizeof(self), "PROGDIR:%s", filename);
    TEST(length >= 0 && (size_t)length < sizeof(self));
    for (mode = 0; mode < 5; ++mode)
    {
        TEST(run_case(mode, self) == OK);
    }
    return OK;
}
