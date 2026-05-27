/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 1

#include <aros/debug.h>
#include <proto/exec.h>

#include <drm-compat/drm_compat_funcs.h>

bool queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
    /* wq is NULL in current implementation */

    D(bug("queue_work emulation called\n"));

    if (!work)
        return FALSE;

    /* Dummy implementation, just execute */
    work->func(work);

    return TRUE;
}
