/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <time.h>
#include <sys/timeb.h>

int ftime(struct timeb *tb)
{
    tb->time     = time(NULL);
    tb->millitm  = 0; // FIXME
    tb->timezone = 0; // FIXME
    tb->dstflag  = 0; // FIXME

    return 0;
}
