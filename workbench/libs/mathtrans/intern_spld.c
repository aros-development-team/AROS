/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <libraries/mathffp.h>
#include <aros/libcall.h>
#include <proto/mathffp.h>
#include <proto/mathtrans.h>
#include <exec/types.h>
#include "mathtrans_intern.h"

LONG intern_SPLd(ULONG fnum)
{
    ULONG i = 0, Res = 0, c;
    ULONG Mask = 0x80000000;
    
    if (onehalf == fnum) return minusone;
    
    while (Res == 0 && i <= 23)
    {
        if ( sqrtonehalf < fnum)
        {
            i++;
            fnum = SPMul(fnum, fnum);
        }
        else
        {
            Res = Mask;
            Mask >>= 1;
            fnum = SPMul(fnum, fnum);
            fnum++;
        }
    }
    
    c = i;
    
    while ((char) Mask != 0x40 && c < 24 )
    {
        if ( sqrtonehalf < fnum )
        {
            fnum = SPMul(fnum, fnum);
        }
        else
        {
            Res |= Mask;
            fnum = SPMul(fnum, fnum);
            fnum++;
        }
        c++;
        Mask >>= 1;
    }
    
    /* for precision */
    if ((char) Res < 0)
    {
        Res += 0x100;
        Res &= FFPMantisse_Mask;
    }
    
    return (Res | (0x40-i) | FFPSign_Mask);
} /* intern_SPLd  */
