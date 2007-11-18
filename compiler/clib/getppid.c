/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <unistd.h>
#include <sys/types.h>

#include <exec/tasks.h>
#include <proto/exec.h>

#include <assert.h>

pid_t getppid(void)
{
  struct Task *ThisTask, *ParentTask;
  struct ETask *eThisTask;

  ThisTask = FindTask(NULL);
  eThisTask = GetETask(ThisTask);
  assert(eThisTask);
  ParentTask = (struct Task *)eThisTask->et_Parent;
  return (pid_t)ParentTask;
}

