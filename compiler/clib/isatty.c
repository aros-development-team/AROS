#include <proto/dos.h>

#include "__open.h"

#include <unistd.h>

int isatty(int fd)
{
    fdesc *desc = __getfdesc(fd);

    if (desc)
        return IsInteractive(desc->fh)?1:0;

    return 0;
}
