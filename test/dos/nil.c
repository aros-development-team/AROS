#include <proto/dos.h>
#include <stdio.h>
#include "test.h"


BPTR nilfh      = BNULL;
BPTR nillock    = BNULL;

static void closehandles()
{
    if (nilfh != BNULL) Close(nilfh);
    if (nillock != BNULL) UnLock(nillock);
    nilfh   = BNULL;
    nillock = BNULL;
}
int main()
{
    TEXT buffer[20];
    LONG result = 0;
    BPTR bresult = BNULL;

    /* Open */
    nilfh = Open("NIL:", MODE_OLDFILE);
    TEST((nilfh !=BNULL));
    closehandles();

    nilfh = Open("NIL:", MODE_NEWFILE);
    TEST((nilfh !=BNULL));
    closehandles();

    nilfh = Open("NIL:", MODE_READWRITE);
    TEST((nilfh !=BNULL));
    closehandles();


    /* Lock */
    nillock = Lock("NIL:", SHARED_LOCK);
    TEST((nillock == BNULL));
    closehandles();

    nillock = Lock("NIL:", EXCLUSIVE_LOCK);
    TEST((nillock == BNULL));
    closehandles();

    /* DupLockFromFH */
    nilfh = Open("NIL:", MODE_OLDFILE);
    nillock = DupLockFromFH(nilfh);
    Close(nilfh);
    nilfh = BNULL;
    TEST((nillock != BNULL));

    /* OpenFromLock */
    nilfh = OpenFromLock(nillock);
    nillock = BNULL; /* Lock was consumed when opening */
    TEST((nilfh != BNULL));
    closehandles();

    /* Write */
    nilfh = Open("NIL:", MODE_OLDFILE);
    result = Write(nilfh, buffer, sizeof(buffer));
    TEST ((result == sizeof(buffer)));
    closehandles();

    /* Info */
    nilfh = Open("NIL:", MODE_OLDFILE);
    nillock = DupLockFromFH(nilfh);
    result = Info(nillock, NULL);
    TEST((result == 0));
    closehandles();

    /* ParentOfFH */
    nilfh = Open("NIL:", MODE_OLDFILE);
    bresult = ParentOfFH(nilfh);
    TEST((bresult == BNULL));
    closehandles();

    cleanup();

    return OK;
}

void cleanup()
{
    closehandles();
}
