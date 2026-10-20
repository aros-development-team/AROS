/*
  Copyright (C) 2014 Szilard Biro
  Copyright (C) 2018 Harry Sintonen

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <proto/exec.h>
#include <exec/execbase.h>


#include "sched.h"
#include "debug.h"

#define PRIO_MAX 2
#define PRIO_MIN -2

/****************************************************************************
    NAME */
        int sched_get_priority_max(

/*  SYNOPSIS */
        int policy)

/*  FUNCTION
        Return the upper bound of the library's fixed scheduling-priority
        range.

    INPUTS
        policy - Scheduling policy identifier; not inspected.

    RESULT
        2 for every policy value.

    NOTES
        The result is a library constant, not a query of the current
        Exec scheduler.

    SEE ALSO
        sched_get_priority_min(),
        sched_yield()

*****************************************************************************/
{
    D(bug("%s(%d)\n", __FUNCTION__, policy));

    return PRIO_MAX;
}

/****************************************************************************
    NAME */
        int sched_get_priority_min(

/*  SYNOPSIS */
        int policy)

/*  FUNCTION
        Return the lower bound of the library's fixed scheduling-priority
        range.

    INPUTS
        policy - Scheduling policy identifier; not inspected.

    RESULT
        -2 for every policy value.

    NOTES
        The result is a library constant, not a query of the current
        Exec scheduler.

    SEE ALSO
        sched_get_priority_max(),
        sched_yield()

*****************************************************************************/
{
    D(bug("%s(%d)\n", __FUNCTION__, policy));

    return PRIO_MIN;
}

/****************************************************************************
    NAME */
        int sched_yield(

/*  SYNOPSIS */
        void)

/*  FUNCTION
        Give the scheduler an opportunity to run another task.

    RESULT
        Always returns 0. No error is reported.

    NOTES
        On AROS, temporarily sets the calling Exec task's priority to -10
        and then restores its previous priority. The call does not
        guarantee that another task runs before it returns.

    SEE ALSO
        sched_get_priority_max(),
        sched_get_priority_min()

*****************************************************************************/
{
#if defined(__MORPHOS__) || defined(__AMIGA__)
    D(bug("%s()\n", __FUNCTION__));
    // calling Permit() will trigger a reschedule
    Forbid();
#if !defined(__MORPHOS__)
    SysBase->SysFlags |= 1<<15; // trigger rescheduling on Permit();
#endif
    Permit();
#else
    BYTE oldpri;
    struct Task *task;

    D(bug("%s()\n", __FUNCTION__));

    task = FindTask(NULL);
    // changing the priority will trigger a reschedule
    oldpri = SetTaskPri(task, -10);
    SetTaskPri(task, oldpri);
#endif

    return 0;
}
