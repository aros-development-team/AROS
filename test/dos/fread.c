#include <proto/dos.h>
#include <stdio.h>
#include "test.h"


BPTR fh = BNULL;

static void closehandles()
{
    if (fh != BNULL) Close(fh);
    fh = BNULL;
}
int main()
{
    LONG result = 0;
    LONG ioerr = 0;
    TEXT buffer[16];

    fh = Open("T:a", MODE_NEWFILE);

    /* Invalid parameters */
    SetIoErr(0);
    result = FRead(fh, buffer, 0, 0);
    ioerr = IoErr();
    TEST((result == 0));
    TEST((ioerr == 0));

    /* EOF */
    SetIoErr(0);
    result = FRead(fh, buffer, 1, 1);
    ioerr = IoErr();
    TEST((result == 0));
    TEST((ioerr == 0));

    /* BNULL file handle */
    SetIoErr(0);
    result = FRead(BNULL, buffer, 1, 1);
    ioerr = IoErr();
    TEST((result == 0));
    TEST((ioerr == 0));

    cleanup();

    return OK;
}

void cleanup()
{
    closehandles();
}
