/*
    Copyright (C) 2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <oop/oop.h>

/*** Bus::New() **************************************************************/

OOP_Object *Bus__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug("[Bus] Root__New()\n"));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if(o)
    {
    }
    D(bug ("[Bus] Root__New: Instance @ 0x%p\n", o);)
    return o;
}

/*** Bus::Dispose() **********************************************************/
VOID Bus__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[Bus] Root__Dispose(0x%p)\n", o));
    OOP_DoSuperMethod(cl, o, msg);
}
