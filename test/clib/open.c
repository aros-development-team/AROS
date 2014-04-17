#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "test.h"

#define FIFO1   "T:fifo1"
#define FIFO2   "T:fifo2"

int f1filedesc[2];
int f2filedesc[2];

int main()
{
    int tmpfd;

    f1filedesc[0] = f1filedesc[1] = -1;

    tmpfd = open(FIFO1, O_RDWR | O_CREAT);
    TEST((tmpfd >= 0));
    close(tmpfd);

    f1filedesc[1] = open(FIFO1, O_WRONLY);
    f1filedesc[0] = open(FIFO1, O_RDONLY);

    TEST((f1filedesc[1] >= 0));
    TEST((f1filedesc[0] >= 0));

    f2filedesc[0] = f2filedesc[1] = -1;

    f2filedesc[1] = open(FIFO2, O_WRONLY | O_CREAT, 0644);
    f2filedesc[0] = open(FIFO2, O_RDONLY);

    TEST((f2filedesc[1] >= 0));
    TEST((f2filedesc[0] >= 0));

    cleanup();

    return OK;
}

void cleanup()
{
    if (f1filedesc[1] >= 0) close(f1filedesc[1]);
    if (f1filedesc[0] >= 0) close(f1filedesc[0]);
    if (f2filedesc[1] >= 0) close(f2filedesc[1]);
    if (f2filedesc[0] >= 0) close(f2filedesc[0]);
    remove(FIFO1);
    remove(FIFO2);
}
