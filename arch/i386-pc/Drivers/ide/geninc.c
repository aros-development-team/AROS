/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/alerts.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <exec/etask.h>
#include <dos/dosextens.h>
#include <stdio.h>
#include <stddef.h>
#include <signal.h>
#include <devices/timer.h>

#define FuncOffset(x)       (int)__AROS_GETJUMPVEC(0,x)

int main (void)
{
    printf ("/* timerequest Structure */\n");
    printf ("#define io_Device     %d\n", offsetof (struct IORequest, io_Device));
    printf ("#define tr_time       %d\n", offsetof (struct timerequest, tr_time));

    printf ("/* timeval structure */\n");
    printf ("#define tv_secs       %d\n", offsetof (struct timeval, tv_secs));
    printf ("#define tv_micro      %d\n", offsetof (struct timeval, tv_micro));

    printf ("\n/* Timer functions */\n");
    printf ("#define GetSysTime    %d\n", FuncOffset (11));

    return 0;
}


