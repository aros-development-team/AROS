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
  struct ETask *et;

  ThisTask = FindTask(NULL);
  eThisTask = GetETask(ThisTask);
  assert(eThisTask);
  ParentTask = (struct Task *)eThisTask->et_Parent;
  if(!ParentTask)
    return (pid_t) 1;
  et = GetETask(ParentTask);
  assert(et); 
  return (pid_t) et->et_UniqueID;
}

