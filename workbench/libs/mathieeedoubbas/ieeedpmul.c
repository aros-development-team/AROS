/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <aros/debug.h>
#include <proto/utility.h>
#include "mathieeedoubbas_intern.h"

static void add128(ULONG *s, ULONG *d)
{
    ULONG ovl = 0;
    WORD i;

    for (i = 3; i >= 0; i--) {
       ULONG o = d[i];
       d[i] += s[i] + ovl;
       ovl = 0;
       if (o > d[i])
       	   ovl = 1;
    }
}

/*****************************************************************************

    NAME */

        AROS_LHQUAD2(double, IEEEDPMul,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),
        AROS_LHAQUAD(double, z, D2, D3),

/*  LOCATION */
        struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 13, MathIeeeDoubBas)

/*  FUNCTION
	Multiplies two IEEE double precision numbers

    INPUTS

    RESULT
        +1 : y > z
	 0 : y = z
        -1 : y < z

	Flags:
	  zero	   : y = z
	  negative : y < z
	  overflow : 0

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL sign;
    QUAD res;
    QUAD Qtmp1, Qtmp2;
    WORD exp1, exp2, exp;
    double *Dres = (double *)&res;

    QUAD * Qy = (QUAD *)&y;
    QUAD * Qz = (QUAD *)&z;

    D(bug("%08x %08x * %08x %08x\n",
        (ULONG)Get_High32of64(*Qy), (ULONG)Get_Low32of64(*Qy), 
        (ULONG)Get_High32of64(*Qz), (ULONG)Get_Low32of64(*Qz)));

    sign = ((Get_High32of64(*Qy) ^ Get_High32of64(*Qz)) & IEEEDPSign_Mask_Hi) != 0;
 
    AND64C(Qtmp1, *Qy, IEEEDPExponent_Mask_Hi, IEEEDPExponent_Mask_Lo);
    AND64C(Qtmp2, *Qz, IEEEDPExponent_Mask_Hi, IEEEDPExponent_Mask_Lo);

    SHRU32(exp1, Qtmp1, 52);
    SHRU32(exp2, Qtmp2, 52);

    exp1 = (exp1 & 0x7ff) - 1023;
    exp2 = (exp2 & 0x7ff) - 1023;
    exp = exp1 + exp2;

    D(bug("EXP %d + EXP %d = %d (%08x)\n", exp1, exp2, exp, (exp + 1023) << 20));

    AND64C(Qtmp1, *Qy, IEEEDPMantisse_Mask_Hi, IEEEDPMantisse_Mask_Lo);
    AND64C(Qtmp2, *Qz, IEEEDPMantisse_Mask_Hi, IEEEDPMantisse_Mask_Lo);

    /* (53 bit) * (53 bit) multiplication */

    ULONG x1, x2, y1, y2;
    ULONG t1[4], t2[4];

    x1 = Get_High32of64(Qtmp1);
    x1 |= 0x00100000; /* Set "hidden" 53th mantissa bit */
    x2 = Get_Low32of64(Qtmp1);
    y1 = Get_High32of64(Qtmp2);
    y1 |= 0x00100000; /* Set "hidden" 53th mantissa bit */
    y2 = Get_Low32of64(Qtmp2);

    D(bug("%08x %08x %08x %08x\n", x1, x2, y1, y2));

    Qtmp1 = UMult64(x1, y1);
    t1[0] = Get_High32of64(Qtmp1);
    t1[1] = Get_Low32of64(Qtmp1);

    Qtmp1 = UMult64(x2, y2);
    t1[2] = Get_High32of64(Qtmp1);
    t1[3] = Get_Low32of64(Qtmp1);

    Qtmp1 = UMult64(x1, y2);
    t2[3] = 0;
    t2[2] = Get_High32of64(Qtmp1);
    t2[1] = Get_Low32of64(Qtmp1);
    t2[0] = 0;

    add128(t2, t1);

    Qtmp1 = UMult64(x2, y1);
    t2[3] = 0;
    t2[2] = Get_High32of64(Qtmp1);
    t2[1] = Get_Low32of64(Qtmp1);
    t2[0] = 0;

    add128(t2, t1);

    D(bug("%08x %08x %08x %08x\n", t1[0], t1[1], t1[2], t1[3]));

    /* Normalize. Pobably could be more optimal.. */
    if (t1[0] || t1[1] || t1[2] || t1[3]) {
        while (t1[0] & 0xfffffe00) {
            t1[3] >>= 1;
            t1[3] |= (t1[2] & 1) ? 0x80000000 : 0;
            t1[2] >>= 1;
            t1[2] |= (t1[1] & 1) ? 0x80000000 : 0;
            t1[1] >>= 1;
            t1[1] |= (t1[0] & 1) ? 0x80000000 : 0;
            t1[0] >>= 1;
            exp++;
        }
        while (!(t1[0] & 0x00000100)) {
            t1[0] <<= 1;
            t1[0] |= (t1[1] & 0x80000000) ? 1 : 0;
            t1[1] <<= 1;
            t1[1] |= (t1[2] & 0x80000000) ? 1 : 0;
            t1[2] <<= 1;
            t1[2] |= (t1[3] & 0x80000000) ? 1 : 0;
            t1[3] <<= 1;
            exp--;
        }
    }

    D(bug("%08x %08x %08x %08x\n", t1[0], t1[1], t1[2], t1[3]));

    if (exp <= -1023) {
        if (sign)
            Set_Value64C(res, 0x0, 0x0);
        else
            Set_Value64C(res, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
        return *Dres;
    }

    if (exp >= 1023) {
        if (sign)
            Set_Value64C(res, IEEEDPPInfty_Hi, IEEEDPPInfty_Lo);
        else
            Set_Value64C(res, IEEEDPPInfty_Hi | IEEEDPSign_Mask_Hi, IEEEDPPInfty_Lo | IEEEDPSign_Mask_Lo);
        return *Dres;
    }

    Set_Value64C(res, (t1[0] << 12) | (t1[1] >> 20), (t1[1] << 12) | (t1[2] >> 20));
    AND64QC(res, IEEEDPMantisse_Mask_Hi, IEEEDPMantisse_Mask_Lo);
    SHL64(Qtmp1, (QUAD)(exp + 1023), 52);
    OR64Q(res, Qtmp1);

    if (sign)
        OR64(res, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);

    return *Dres;

    AROS_LIBFUNC_EXIT
}
