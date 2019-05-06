/*
    Copyright © 2015-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef TASKRES_H
#define TASKRES_H

#ifndef EXEC_TASKS_H
#   include <exec/tasks.h>
#endif

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#define LTF_RUNNING                     (1 << 0)
#define LTF_READY                       (1 << 1)
#define LTF_WAITING                     (1 << 2)                // N.B: includes spinning tasks
#define LTF_ALL                         (LTF_RUNNING|LTF_READY|LTF_WAITING)
#define LTF_WRITE                       (1 << 31)

#define TaskTag_CPUNumber               (TAG_USER + 0x00000001) // CPU Number task is currently running on
#define TaskTag_CPUAffinity             (TAG_USER + 0x00000002) // CPU Affinity mask
#define TaskTag_CPUTime                 (TAG_USER + 0x00000003) // Amount of CPU time spent running
#define TaskTag_StartTime               (TAG_USER + 0x00000004) // Time the task was started
#define TaskTag_CPUUsage                (TAG_USER + 0x00000005) // CPU Usage of this task

/* Task Hook Flags */
#define THF_ROA                         1
#define THF_IAR                         2

// The contents of the tasklist are private to task.resource
struct TaskList
{
    void *tl_Private;
};

typedef int (*TaskResHookDispatcher)(struct Hook *);

#define TASKHOOK_TYPE_LATEINIT          (1 << 0)

#endif /* TASKRES_H */
