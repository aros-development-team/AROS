/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <proto/exec.h>
#include <exec/tasks.h>
#include <exec/lists.h>
#include <exec/memory.h>

#include "parallel_intern.h"

#define DEBUG 0
#include <aros/debug.h>


struct ParallelUnit * findUnit(struct parallelbase * ParallelDevice, 
                             ULONG unitnum)
{
  struct ParallelUnit * pu;
  ForeachNode(&ParallelDevice->UnitList, pu)
  {
    if (pu->pu_UnitNum == unitnum)
      return pu;
  }
  return NULL;
}

