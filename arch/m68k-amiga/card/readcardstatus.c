/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: readcardstatus.c $

    Desc: ReadCardStatus() function.
    Lang: english
*/

#include "card_intern.h"

AROS_LH0(UBYTE, ReadCardStatus,
	 struct CardResource*, CardResource, 6, Cardres)
{
    AROS_LIBFUNC_INIT

    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;
    return gio->status;

    AROS_LIBFUNC_EXIT
}
