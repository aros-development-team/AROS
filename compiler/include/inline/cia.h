/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_CIA_H
#define _INLINE_CIA_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#define AbleICR(resource, mask) \
	LP2UB(0x12, WORD, AbleICR, struct Library *, resource, a6, long, mask, d0)

#define AddICRVector(resource, iCRBit, interrupt) \
	LP3UB(0x6, struct Interrupt *, AddICRVector, struct Library *, resource, a6, long, iCRBit, d0, struct Interrupt *, interrupt, a1)

#define RemICRVector(resource, iCRBit, interrupt) \
	LP3NRUB(0xc, RemICRVector, struct Library *, resource, a6, long, iCRBit, d0, struct Interrupt *, interrupt, a1)

#define SetICR(resource, mask) \
	LP2UB(0x18, WORD, SetICR, struct Library *, resource, a6, long, mask, d0)

#endif /* _INLINE_CIA_H */
