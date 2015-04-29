/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef TASKRES_H
#define TASKRES_H

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#define TaskTag_CPUNumber       (TAG_USER + 0x00000001) // CPU Number task is currently running on
#define TaskTag_CPUAffinity     (TAG_USER + 0x00000002) // CPU Affinity mask
#define TaskTag_CPUTime         (TAG_USER + 0x00000003) // Amount of CPU time spent running
#define TaskTag_StartTime       (TAG_USER + 0x00000004) // Time the task was started

#endif /* TASKRES_H */
