/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CardProgramVoltage() function.
    Lang: english
*/

#include "card_intern.h"

AROS_LH2(LONG, CardProgramVoltage,
        AROS_LHA(struct CardHandle*, handle, A1),
        AROS_LHA(ULONG, voltage, D0),
        struct CardResource*, CardResource, 10, Cardres)
{
    AROS_LIBFUNC_INIT

    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;
    UBYTE v;

    CARDDEBUG(bug("CardProgramVoltage(%p,%d)\n", handle, voltage));

    if (!ISMINE)
        return 0;
        
    if (voltage != GAYLE_CFG_0V && voltage != GAYLE_CFG_5V)
        return -1; /* Don't want 12V at this point.. */
    
    Disable();
    v = gio->config & ~GAYLE_CFG_VOLTAGE;
    v |= voltage;
    gio->config = v;
    Enable();

    return 1;

    AROS_LIBFUNC_EXIT
}
