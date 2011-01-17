/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id:$

    Desc: AbleICR() function.
    Lang: english
*/

#include <exec/libraries.h>
#include <proto/cia.h>

AROS_LH1(WORD, AbleICR,
	 AROS_LHA(WORD, mask, D0),
	 struct Library *, resource, 8, Cia)
{
    AROS_LIBFUNC_INIT

    /* Not implemented */
    return 0;

    AROS_LIBFUNC_EXIT
}
