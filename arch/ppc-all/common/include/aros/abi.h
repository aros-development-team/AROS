#ifndef AROS_ABI_H
#define AROS_ABI_H

/*
    Copyright C 2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS/PPC Application Binary Interface
    Lang: English
*/

// vim:ts=8:sts=4:sw=4

#include <exec/types.h>

/*
    Note:

        This file *HAS TO BE INCLUDED* either directly or indirectly by
        every single library function. They will never work otherwise. 
	Hopefully this is probably always done by aros/libcall.h file
*/

/*
    Per-Task global structure holding function parameters passed through
    M68K-like register frame. The structure will grow in future in order to
    allow mixing PPC with M68K code.
*/

#ifdef __KERNEL__

/*
    Define return() macro which not only returns given value. It also stores
    it in CallOS structure in virtual register D0. Usefull by emulation
*/

#define return(val) do {return CallOS->D[0] = (ULONG)(val);}while(0)

#endif /* __KERNEL__ */

struct CallOS {
// public:
    ULONG D[8];	    // Data registers
    ULONG A[8];	    // Address registers
};

/*
    Define global register pointer. This way we may take for granted, that r2
    will always point to the register frame pointer. It would have been so
    anyway, since according to PPC EAbi the r2 register is reserved for local
    data pointer purpouses.
*/
register struct CallOS *CallOS asm("r2");

/* Usefull macros making code more human-readable */
#define __D0 (CallOS->D[0])
#define __D1 (CallOS->D[1])
#define __D2 (CallOS->D[2])
#define __D3 (CallOS->D[3])
#define __D4 (CallOS->D[4])
#define __D5 (CallOS->D[5])
#define __D6 (CallOS->D[6])
#define __D7 (CallOS->D[7])

#define __A0 (CallOS->A[0])
#define __A1 (CallOS->A[1])
#define __A2 (CallOS->A[2])
#define __A3 (CallOS->A[3])
#define __A4 (CallOS->A[4])
#define __A5 (CallOS->A[5])
#define __A6 (CallOS->A[6])
#define __A7 (CallOS->A[7])

#endif /* AROS_ABI_H */

