/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>

#include <proto/cia.h>

#include <exec/types.h>
#include <libraries/lowlevel.h>

#include "lowlevel_intern.h"

AROS_LH1(VOID, RemTimerInt,
      AROS_LHA(APTR, intHandle, A1),
      struct LowLevelBase *, LowLevelBase, 14, LowLevel)
{
  AROS_LIBFUNC_INIT

    if (LowLevelBase->ll_CIA.llciat_Base && intHandle)
    {
        RemICRVector(LowLevelBase->ll_CIA.llciat_Base, LowLevelBase->ll_CIA.llciat_iCRBit, intHandle);
        LowLevelBase->ll_CIA.llciat_Int.is_Code = NULL;
        LowLevelBase->ll_CIA.llciat_iCRBit = -1;
    }

    return;

  AROS_LIBFUNC_EXIT
}
