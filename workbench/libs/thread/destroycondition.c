/*
 * thread.library - threading and synchronisation primitives
 *
 * Copyright © 2007 Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include "thread_intern.h"

#include <exec/types.h>
#include <proto/exec.h>
#include <assert.h>

/*****************************************************************************

    NAME */
        AROS_LH1(BOOL, DestroyCondition,

/*  SYNOPSIS */
        AROS_LHA(void *, cond, A0),

/*  LOCATION */
        struct ThreadBase *, ThreadBase, 16, Thread)

/*  FUNCTION
        Destroys a condition variable.

    INPUTS
        cond - the condition variable to destroy.

    RESULT
        TRUE if the condition was destroyed, otherwise FALSE.

    NOTES
        You cannot destroy a condition variable if other threads are waiting on
        it.

    EXAMPLE
        DestroyCondition(cond);

    BUGS

    SEE ALSO
        CreateCondition(), WaitCondition(), SignalCondition(),
        BroadcastCondition()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct _Condition *c = (struct _Condition *) cond;

    assert(c != NULL);

    /* we can only destroy the cond if noone is waiting on it */
    ObtainSemaphoreShared(&c->lock);
    if (c->count > 0) {
        ReleaseSemaphore(&c->lock);
        return FALSE;
    }

    FreeMem(c, sizeof(struct _Condition));

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* DestroyCondition */
