/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>

#include <proto/cia.h>

#include <exec/types.h>
#include <libraries/lowlevel.h>
#include <hardware/cia.h>
#include <resources/cia.h>

#include "lowlevel_intern.h"
#include "cia_intern.h"
#include "cia_timer.h"

AROS_LH1(VOID, StopTimerInt,
      AROS_LHA(APTR , intHandle, A1),
      struct LowLevelBase *, LowLevelBase, 15, LowLevel)
{
  AROS_LIBFUNC_INIT

    struct CIABase *CiaBase = (struct CIABase *)LowLevelBase->ll_CIA.llciat_Base;

    if (LowLevelBase->ll_CIA.llciat_iCRBit == CIAICRB_TA)
    {
        CiaBase->hw->ciacra &= CIASTOP_A;
    }
    else
    {
        CiaBase->hw->ciacrb &= CIASTOP_B;
    }
    return;

  AROS_LIBFUNC_EXIT
}
