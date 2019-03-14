/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: called when a function is exited
*/

#include <aros/debug.h>
#include <proto/exec.h>

void __cyg_profile_func_exit  (void *this_fn,
                               void *call_site)
{
    struct Task *thisTask = FindTask(NULL);
    bug("[0x%p<<] Exit Func @ 0x%p", thisTask, this_fn);
}
