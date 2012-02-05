/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/nodes.h>
#include <exec/lists.h>

#include "exec_intern.h"
#include "taskstorage.h"

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH0(APTR, SaveTaskStorage,

/*  LOCATION */
        struct ExecBase *, SysBase, 177, Exec)

/*  FUNCTION
        The will remember the current state of the task storage slots

    INPUTS
        None.

    RESULT
        An id will be returned whit which the current state can be restored
        using RestoreTaskStorageSlots().
        NULL is returned when not enough memory was available.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        RestoreTaskStorage()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    IPTR *taskstorage = FindTask(NULL)->tc_UnionETask.tc_TaskStorage,
        *tsout;
    int size = (int)taskstorage[__TS_FIRSTSLOT];

    tsout = AllocMem(size, MEMF_PUBLIC);
    if (tsout)
        CopyMemQuick(taskstorage, tsout, size);

    D(bug("SaveTaskStorage: taskstorage=%x, size=%d, tsout=%x\n",
          taskstorage, size, tsout
    ));

    return tsout;

    AROS_LIBFUNC_EXIT
}
