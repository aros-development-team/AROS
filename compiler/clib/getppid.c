/*
    Copyright � 2004-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <unistd.h>
#include <sys/types.h>

#include <exec/tasks.h>
#include <proto/exec.h>

#include <assert.h>

#include "__vfork.h"
#include "__arosc_privdata.h"

/*****************************************************************************

    NAME */

	pid_t getppid(

/*  SYNOPSIS */
	void)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
  struct aroscbase *aroscbase = __get_aroscbase();
  struct Task *ParentTask;
  struct ETask *eThisTask;
  struct ETask *et;

  if(aroscbase->acb_flags & PRETEND_CHILD)
  {
    struct vfork_data *udata = aroscbase->acb_vfork_data;
    eThisTask = GetETask(udata->child);
  }
  else
    eThisTask = GetETask(FindTask(NULL));
  assert(eThisTask);
  ParentTask = (struct Task *)eThisTask->et_Parent;
  if(!ParentTask)
    return (pid_t) 1;
  et = GetETask(ParentTask);
  assert(et); 
  return (pid_t) et->et_UniqueID;
}

