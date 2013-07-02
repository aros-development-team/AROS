/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadCardStatus() function.
    Lang: english
*/

#include "card_intern.h"

AROS_LH0(UBYTE, ReadCardStatus,
	 struct CardResource*, CardResource, 6, Cardres)
{
    AROS_LIBFUNC_INIT

    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;
    return gio->status & (GAYLE_CS_CCDET | GAYLE_CS_BVD1 | GAYLE_CS_BVD2 | GAYLE_CS_WR | GAYLE_CS_BSY);

    AROS_LIBFUNC_EXIT
}
