/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include "power_intern.h"

/*** Power::New() **************************************************************/

OOP_Object *Power__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug("[Power] Root__New()\n"));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if(o)
    {
    }
    D(bug ("[Power] Root__New: Instance @ 0x%p\n", o);)
    return o;
}

/*** Power::Dispose() **********************************************************/
VOID Power__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[Power] Root__Dispose(0x%p)\n", o));
    OOP_DoSuperMethod(cl, o, msg);
}
