/*
    Copyright (C) 2018-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <oop/oop.h>

/*****************************************************************************************

    NAME
        aoHidd_Bus_IRQHandler

    SYNOPSIS
        [.S.], APTR

    LOCATION
        CLID_Hidd_Bus

    FUNCTION
        Specifies IRQ handler function to be called when bus interrupt arrives.
        The function should be called using "C" calling convention and has the
        following prototype:
        
            void bus_HandleIRQ(UBYTE status, APTR userdata);
        
        Your driver should pass the following arguments to this function:
            status   - value read from bus status register.
            userdata - value of aoHidd_Bus_IRQData attribute.

    NOTES
        Reading the status register is part of the interrupt acknowledge
        process, and therefore has to be done by the driver.

        It is driver's job to check whether the interrupt really belongs to
        the bus.

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_Bus_IRQData

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_Bus_IRQData

    SYNOPSIS
        [.S.], APTR

    LOCATION
        CLID_Hidd_Bus

    FUNCTION
        Caller's private data to be supplied to IRQ handler function.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_Bus_IRQData

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_Bus_KeepEmpty

    SYNOPSIS
        [I..], BOOL

    LOCATION
        CLID_Hidd_Bus

    FUNCTION
        If this attribute is set to FALSE during object creation, the object
        will be destroyed if no devices are detected on the bus.

    NOTES
        This can be useful for optional buses like legacy ISA controllers,
        which have no other way to detect their presence.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

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
