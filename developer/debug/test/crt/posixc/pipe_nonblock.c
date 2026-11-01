/* Nonblocking pipe calls must return without leaving pending IO requests. */
#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "test.h"

static int channel[2] = {-1, -1};
static int alias = -1;
static DIR *directory;

void cleanup(void)
{
    int i;

    if (directory != NULL)
    {
        closedir(directory);
        directory = NULL;
    }
    if (alias >= 0)
    {
        close(alias);
        alias = -1;
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

static int enable(int fd)
{
    int flags = fcntl(fd, F_GETFL);

    return flags < 0 ? -1 : fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int again(ssize_t result, int saved_errno)
{
    return result == -1 && (saved_errno == EAGAIN || saved_errno == EWOULDBLOCK);
}

int main(void)
{
    char buffer[8192];
    ssize_t result = 0;
    int saved_errno = 0;
    int total = 0;
    int flags;
    int fd;
    unsigned int attempts;

    memset(buffer, 'X', sizeof(buffer));
    TEST(pipe(channel) == 0);
    TEST(enable(channel[0]) == 0);
    TEST(enable(channel[1]) == 0);
    errno = 0;
    result = read(channel[0], buffer, sizeof(buffer));
    saved_errno = errno;
    TEST(again(result, saved_errno));

    alias = dup(channel[0]);
    TEST(alias >= 0);
    TEST((fcntl(alias, F_GETFL) & O_NONBLOCK) != 0);
    TEST(write(channel[1], "X", 1) == 1);
    TEST(read(alias, buffer, sizeof(buffer)) == 1);
    TEST(buffer[0] == 'X');

    for (attempts = 0; attempts < 4096; ++attempts)
    {
        errno = 0;
        result = write(channel[1], buffer, sizeof(buffer));
        saved_errno = errno;
        if (again(result, saved_errno))
            break;
        TEST(result > 0);
        total += result;
    }
    TEST(again(result, saved_errno));
    TEST(total > 0);
    TEST(read(channel[0], buffer, 1) == 1);
    errno = 0;
    result = write(channel[1], "YZ", 2);
    saved_errno = errno;
    TEST(again(result, saved_errno));
    TEST(read(channel[0], buffer, sizeof(buffer)) > 0);
    TEST(write(channel[1], "Y", 1) == 1);

    for (attempts = 0; attempts < 4096; ++attempts)
    {
        errno = 0;
        result = read(channel[0], buffer, sizeof(buffer));
        saved_errno = errno;
        if (result <= 0)
            break;
    }
    TEST(again(result, saved_errno));
    TEST(close(channel[1]) == 0);
    channel[1] = -1;
    TEST(read(alias, buffer, sizeof(buffer)) == 0);
    flags = fcntl(alias, F_GETFL);
    TEST(flags >= 0);
    TEST(fcntl(alias, F_SETFL, flags & ~O_NONBLOCK) == 0);
    TEST((fcntl(channel[0], F_GETFL) & O_NONBLOCK) == 0);
    cleanup();

    directory = opendir("RAM:");
    TEST(directory != NULL);
    fd = dirfd(directory);
    flags = fcntl(fd, F_GETFL);
    TEST(flags >= 0);
    TEST(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0);
    cleanup();
    printf("PIPE_NONBLOCK_PASS capacity=%d\n", total);
    return OK;
}
