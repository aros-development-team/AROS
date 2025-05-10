/*
  Copyright (C) 2014 Szilard Biro
  Copyright (C) 2018 Harry Sintonen
  Copyright (C) 2019 Stefan "Bebbo" Franke - AmigaOS 3 port

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

#include <string.h>

#include "pthread_intern.h"
#include "debug.h"

int pthread_attr_init(pthread_attr_t *attr)
{
    struct Task *task;

    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL)
        return EINVAL;

    memset(attr, 0, sizeof(pthread_attr_t));
    // inherit the priority and stack size of the parent thread

    task = GET_THIS_TASK;

    attr->param.sched_priority = task->tc_Node.ln_Pri;
#ifdef __MORPHOS__
    NewGetTaskAttrs(task, &attr->stacksize68k, sizeof(attr->stacksize68k), TASKINFOTYPE_STACKSIZE_M68K, TAG_DONE);
    NewGetTaskAttrs(task, &attr->stacksize, sizeof(attr->stacksize), TASKINFOTYPE_STACKSIZE, TAG_DONE);
#else
    attr->stacksize = (UBYTE *)task->tc_SPUpper - (UBYTE *)task->tc_SPLower;
#endif

    return 0;
}
