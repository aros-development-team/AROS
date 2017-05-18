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

/* Attempt to open the specified cia resource, and add the timer interrupt ... */
WORD FindFreeTimer(char *timeres, struct LowLevelBase *LowLevelBase)
{
    if ((LowLevelBase->ll_CIA.llciat_Base = OpenResource(timeres)) != NULL)
    {
        if (!(AddICRVector(LowLevelBase->ll_CIA.llciat_Base, CIAICRB_TA, &LowLevelBase->ll_CIA.llciat_Int)))
            return CIAICRB_TA;
        else if (!(AddICRVector(LowLevelBase->ll_CIA.llciat_Base, CIAICRB_TB, &LowLevelBase->ll_CIA.llciat_Int)))
            return CIAICRB_TB;
    }
    LowLevelBase->ll_CIA.llciat_Base = NULL;
    return -1;
}

/* Public Implementation ... */
AROS_LH2(APTR, AddTimerInt,
      AROS_LHA(APTR, intRoutine, A0),
      AROS_LHA(APTR, intData, A1),
      struct LowLevelBase *, LowLevelBase, 13, LowLevel)
{
    AROS_LIBFUNC_INIT

    APTR result = NULL;

    if (intRoutine)
    {
        ObtainSemaphore(&LowLevelBase->ll_Lock);

        if (LowLevelBase->ll_CIA.llciat_Int.is_Code == NULL)
        {
            LowLevelBase->ll_CIA.llciat_Int.is_Code = intRoutine;
            LowLevelBase->ll_CIA.llciat_Int.is_Data = intData;
            if ((LowLevelBase->ll_CIA.llciat_iCRBit = FindFreeTimer(CIAANAME, LowLevelBase)) == -1)
            {
                LowLevelBase->ll_CIA.llciat_iCRBit = FindFreeTimer(CIABNAME, LowLevelBase);
            }
            if (LowLevelBase->ll_CIA.llciat_Base && (LowLevelBase->ll_CIA.llciat_iCRBit != -1))
                result = (APTR)&LowLevelBase->ll_CIA.llciat_Int;
        }

        ReleaseSemaphore(&LowLevelBase->ll_Lock);
    }
    return result;

    AROS_LIBFUNC_EXIT
}
