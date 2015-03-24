/*
  Copyright (C) 2014 Szilard Biro

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

#include "sched.h"
#include "debug.h"

#define PRIO_MAX 2
#define PRIO_MIN -2

int sched_get_priority_max(int policy)
{
    D(bug("%s(%d)\n", __FUNCTION__, policy));

    return PRIO_MAX;
}

int sched_get_priority_min(int policy)
{
    D(bug("%s(%d)\n", __FUNCTION__, policy));

    return PRIO_MIN;
}

int sched_yield(void)
{
    BYTE oldpri;
    struct Task *task;

    D(bug("%s()\n", __FUNCTION__));

    task = FindTask(NULL);
    // changing the priority will trigger a reschedule
    oldpri = SetTaskPri(task, -10);
    SetTaskPri(task, oldpri);

    return 0;
}
