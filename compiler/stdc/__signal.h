/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <aros/types/__sighandler_t.h>

struct signal_func_data
{
    __sighandler_t *sigfunc;
    UBYTE flags;
};

#define __SIG_RUNNING 1 /* Signal handler is running */
#define __SIG_PENDING 2 /* Signal handler is pending */

struct signal_func_data *__sig_getfuncdata(int signum);
__sighandler_t __sig_default;
