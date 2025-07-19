/*
    Copyright (C) 2015-2025, The AROS Development Team. All rights reserved.
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
#define TaskTag_REG_PC                  (TAG_USER + 0x00000020)
#define TaskTag_REG_SP                  (TAG_USER + 0x00000021)
#define TaskTag_REG_GeneralCount        (TAG_USER + 0x00000030)
#define TaskTag_REG_GeneralSize         (TAG_USER + 0x00000031)
#define TaskTag_REG_General0            (TAG_USER + 0x00000032)
#define TaskTag_REG_FloatCount          (TAG_USER + 0x00000050)
#define TaskTag_REG_FloatSize           (TAG_USER + 0x00000051)
#define TaskTag_REG_Float0              (TAG_USER + 0x00000052)
#define TaskTag_REG_VecCount            (TAG_USER + 0x00000070)
#define TaskTag_REG_VecSize             (TAG_USER + 0x00000071)
#define TaskTag_REG_Vec0                (TAG_USER + 0x00000072)

#define TaskTag_REGID_f                 0x00001000

#define TaskTag_REGID_General0          (TaskTag_REG_General0   | TaskTag_REGID_f)
#define TaskTag_REGID_Float0            (TaskTag_REG_Float0     | TaskTag_REGID_f)
#define TaskTag_REGID_Vec0              (TaskTag_REG_Vec0       | TaskTag_REGID_f)

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
