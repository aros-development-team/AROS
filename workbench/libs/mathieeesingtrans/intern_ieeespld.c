/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/mathieeesp.h>
#include <aros/libcall.h>
#include <proto/mathieeesingbas.h>
#include <proto/mathieeesingtrans.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeesingtrans_intern.h"

LONG intern_IEEESPLd(ULONG fnum)
{
    ULONG i = 0, Res = 0;
    ULONG Mask = 0;
    
    if (onehalf == fnum) return minusone;
    
    while (Mask == 0 && i <= 23)
    {
        /* if (sqrtone < fnum) */
        if ( 0x00b504f3 < ((fnum & IEEESPMantisse_Mask) | 0x00800000) )
        {
            i++;
            fnum = IEEESPMul(fnum, fnum);
        }
        else
        {
            Mask = 0x40000000;
            fnum = IEEESPMul(fnum, fnum);
            fnum+= 0x00800000;
        }
    }
    
    while ((char) Mask != 0x40)
    {
        if ( 0x00b504f3 < ((fnum & IEEESPMantisse_Mask) | 0x00800000) )
        {
            fnum = IEEESPMul(fnum, fnum);
        }
        else
        {
            Res |= Mask;
            fnum = IEEESPMul(fnum, fnum);
            fnum+= 0x00800000;
        }
        Mask >>= 1;
    }
    
    /* for precision */
    if ((char) Res < 0) Res += 0x100;
    
    Res >>= 8;
    
    return (Res | ( (0x7e - i) << 23 ) | IEEESPSign_Mask );
} /* intern_SPLd  */
