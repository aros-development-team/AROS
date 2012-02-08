/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/nodes.h>
#include <exec/lists.h>
#include <clib/macros.h>

#include "exec_intern.h"
#include "taskstorage.h"

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH1(void, RestoreTaskStorage,

/*  LOCATION */
        AROS_LHA(APTR, id, A0),
        struct ExecBase *, SysBase, 178, Exec)

/*  FUNCTION
        The will remmeber the current state of the task storage slots

    INPUTS
        id - id returned from SaveTaskStorage() referring to the state

    RESULT
        -

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        SaveTaskStorage()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    IPTR *tsrestore = id, *taskstorage;
    struct Task *thistask = FindTask(NULL);
    ULONG size1, size2, size;

    if (!tsrestore)
        return;

    /* Be sure no other tasks access the information when it is in an
       inconsistent state
    */
    Forbid();

    /* Restore content of TaskStorage */
    size1 = (ULONG)tsrestore[__TS_FIRSTSLOT];
    taskstorage = thistask->tc_UnionETask.tc_TaskStorage;
    size2 = (ULONG)taskstorage[__TS_FIRSTSLOT];

    size = MIN(size1, size2);
    if (((size&3) == 0))
        CopyMemQuick(tsrestore, taskstorage, size);
    else
        CopyMem(tsrestore, taskstorage, size);
    taskstorage[__TS_FIRSTSLOT] = size2;
    D(bug("RestoreTaskStorage: Freeing tsrestore=%x of size %d\n",
          tsrestore, size1
    ));
    FreeMem(tsrestore, size1);

    Permit();

    AROS_LIBFUNC_EXIT
}
