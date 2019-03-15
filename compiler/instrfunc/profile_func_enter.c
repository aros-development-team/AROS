/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: called when a function is entered
*/

#include <aros/debug.h>
#include <proto/exec.h>

void __cyg_profile_func_enter (void *this_fn,
                               void *call_site)
{
    struct Task *thisTask = FindTask(NULL);
    int *frame = NULL;
    frame = (int *)__builtin_frame_address(1); /*of the 'func'*/
    bug("[0x%p>>] Enter Func @ 0x%p (frame @ 0x%p)\n", thisTask, this_fn, frame);
}
