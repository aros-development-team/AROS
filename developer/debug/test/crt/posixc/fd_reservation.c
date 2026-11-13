/* Verify retained descriptor reservations independently of posixc clients. */
#include <proto/exec.h>
#include <proto/fd.h>
#include <libraries/fd.h>
#include <errno.h>
#include <stdio.h>

#include "test.h"

struct Library *FDBase;
static struct Library *held_library;
static LONG held_fd = -1;
static LONG held_owner;
static unsigned int references;

void cleanup(void)
{
    while (references != 0)
    {
        FD_Free(held_fd, held_owner);
        --references;
    }
    if (held_library != NULL)
    {
        CloseLibrary(held_library);
        held_library = NULL;
    }
}

int main(void)
{
    LONG next = -1;
    LONG released_fd;

    held_library = OpenLibrary((CONST_STRPTR)"fd.library", 51);
    TEST(held_library != NULL);
    FDBase = held_library;

    TEST(FD_Alloc(64, FD_OWNER_POSIXC, NULL, &held_fd) == 0);
    held_owner = FD_OWNER_POSIXC;
    references = 1;
    TEST(FD_Reserve(held_fd, FD_OWNER_POSIXC, NULL) == EBUSY);
    TEST(FD_Retain(held_fd, FD_OWNER_BSDSOCKET) == EBADF);
    TEST(FD_Retain(held_fd, FD_OWNER_POSIXC) == 0);
    ++references;

    TEST(FD_Free(held_fd, FD_OWNER_POSIXC) == 0);
    --references;
    TEST(FD_GetOwner(held_fd) == FD_OWNER_POSIXC);
    TEST(FD_Check(held_fd) == EBUSY);
    TEST(FD_Free(held_fd, FD_OWNER_POSIXC) == 0);
    --references;
    TEST(FD_Check(held_fd) == 0);
    TEST(FD_Retain(held_fd, FD_OWNER_POSIXC) == EBADF);

    released_fd = held_fd;
    TEST(FD_Alloc(released_fd, FD_OWNER_BSDSOCKET, NULL, &next) == 0);
    held_fd = next;
    held_owner = FD_OWNER_BSDSOCKET;
    references = 1;
    TEST(next == released_fd);
    TEST(FD_Free(held_fd, FD_OWNER_BSDSOCKET) == 0);
    --references;
    cleanup();
    return OK;
}
