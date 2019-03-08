/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/oop.h>
#include <proto/utility.h>

#include <hidd/storage.h>
#include <hidd/ata.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "ata.h"
#include "ata_bus.h"

#define DIRQ(x)

static void Hidd_ATABus_HandleIRQ(UBYTE status, struct ata_Bus *bus)
{
    struct ata_Unit *unit = bus->ab_SelectedUnit;

    /*
     * don't waste your time on checking other devices.
     * pass irq ONLY if task is expecting one;
     */
    if (unit && bus->ab_HandleIRQ)
    {
        /* ok, we have a routine to handle any form of transmission etc. */
        DIRQ(bug("[ATA%02d] IRQ: Calling dedicated handler 0x%p... \n",
                 unit->au_UnitNum, bus->ab_HandleIRQ));
        bus->ab_HandleIRQ(unit, status);

        return;
    }

    DIRQ({
        /*
         * if we got *here* then device is most likely not expected to have an irq.
         */
        bug("[ATA%02d] Spurious IRQ\n", unit ? unit->au_UnitNum : -1);

        if (0 == (ATAF_BUSY & status))
        {
            bug("[ATA  ] STATUS: %02lx\n"     , status);
            bug("[ATA  ] ALT STATUS: %02lx\n" , PIO_InAlt(bus, ata_AltStatus));
            bug("[ATA  ] ERROR: %02lx\n"      , PIO_In(bus, ata_Error));
            bug("[ATA  ] IRQ: REASON: %02lx\n", PIO_In(bus, atapi_Reason));
        }
    });
}

static AROS_INTH1(ataBus_Reset, struct ata_Bus *, bus)
{
    AROS_INTFUNC_INIT

    struct ataBase *ATABase = bus->ab_Base;
    OOP_Object *obj = (void *)bus - ATABase->busClass->InstOffset;

    D(bug("[ATA:Bus] %s()\n", __PRETTY_FUNCTION__));

    HIDD_ATABus_Shutdown(obj);

    return FALSE;

    AROS_INTFUNC_EXIT
}

/*****************************************************************************************

    NAME
        --background--

    LOCATION
        CLID_Hidd_ATABus

    NOTES
        This class serves as a base class for implementing IDE (ATA) bus drivers.
        One particularity of this class is that IDE bus is very speed-critical.
        At the other hand, the driver implements very lowlevel operations which
        are called quite often. OOP_DoMethod() call is not fast enough, and in
        order to circumvent this limitation, additionally to normal OOP API
        IDE bus drivers offer two additional non-standard interfaces. Internally
        they are implemented as library-alike function table plus driver-specific
        data. For the purpose of some performance optimizations the function
        table is private to ata.device and managed entirely by the base class.
        Driver classes have access only to data portion.

        These interfaces are documented below.

*****************************************************************************************/
/*****************************************************************************************

    NAME
        --PIO interface--

    LOCATION
        CLID_Hidd_ATABus

    NOTES
        PIO interface is responsible for accessing I/O registers on the IDE
        bus, as well as performing PIO-mode 16- and 32-bit data transfers.
        This interface is mandatory and must be implemented by the driver,
        however some functions are optional. They can be either omitted
        entirely from the function table, or set to NULL pointers.

        Control functions table for the interface consists of the following
        functions (listed in their order in the array):

        VOID ata_out(void *obj, UBYTE val, UWORD offset)
        - Write byte into primary register bank with the given offset.

        UBYTE ata_in(void *obj, UWORD offset)
        - Read byte from primary register bank with the given offset.

        VOID ata_out_alt(void *obj, UBYTE val, UWORD offset)
        - Write byte into alternate register bank with the given offset.
          This function is optional.

        UBYTE ata_in_alt(void *obj, UWORD offset)
        - Read byte from alternate register bank with the given offset.
          This function is optional.

        Transfer functions table for the interface consists of the following
        functions (listed in their order in the array):

        VOID ata_outsw(void *obj, APTR address, ULONG count)
        - Perform 16-bit PIO data write operation from the given memory
          region of the given size.

        VOID ata_insw(void *obj, APTR address, ULONG count)
        - Perform 16-bit PIO data read operation into the given memory
          region of the given size.

        VOID ata_outsl(void *obj, APTR address, ULONG count)
        - Perform 32-bit PIO data write operation from the given memory
          region of the given size. This function is optional.

        UBYTE ata_insl(void *obj, APTR address, ULONG count)
        - Perform 32-bit PIO data read operation into the given memory
          region of the given size. This function is optional.

*****************************************************************************************/
/*****************************************************************************************

    NAME
        --DMA interface--

    LOCATION
        CLID_Hidd_ATABus

    NOTES
        DMA interface is optional, and is needed in order to support DMA data
        transfers.

        Function table for the interface consists of the following functions:

        BOOL dma_Setup(void *obj, APTR buffer, IPTR size, BOOL read)
        - Prepare the controller to DMA data transfer. The last argument is
          TRUE for read operation and FALSE for write. The function should
          return TRUE for success or FALSE for failure.

        VOID dma_Start(void *obj)
        - Start DMA transfer.

        VOID dma_End(void *obj, APTR buffer, IPTR size, BOOL read)
        - End DMA transfer and perform post-transfer cleanup of the given region.

        ULONG dma_Result(void *obj)
        - Get resulting status of the operation. The function should return 0
          for successful completion or error code to be passed up to ata.device
          caller in io_Result field of the IORequest.

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_Use80Wire

    SYNOPSIS
        [..G], BOOL

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Tells whether the bus currently uses 80-conductor cable.

    NOTES
        This attribute actually makes difference only for DMA modes. If
        your bus driver returns FALSE, ata.device will not use modes
        higher than UDMA2 on the bus.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_Use32Bit

    SYNOPSIS
        [.SG], BOOL

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        When queried, tells whether the bus supports 32-bit PIO data transfers.
        When set, enables or disables 32-bit mode for PIO data transfers.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_UseDMA

    SYNOPSIS
        [..G], BOOL

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Tells whether the bus supports DMA transfers.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Default implementation in base class returns value depending on whether
        the subclass provided DMA interface function table during object creation.

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_PIODataSize

    SYNOPSIS
        [I..], BOOL

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Specifies size of PIO interface data structure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_DMADataSize

    SYNOPSIS
        [I..], BOOL

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Specifies size of DMA interface data structure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_BusVectors

    SYNOPSIS
        [I..], APTR *

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Specifies control functions table for building PIO interface object.
        The function table is an array of function pointers terminated
        by -1 value. The terminator must be present for purpose of
        binary compatibility with future extensions.

    NOTES
        This function table is mandatory to be implemented by the driver.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_PIOVectors

    SYNOPSIS
        [I..], APTR *

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Specifies transfers function table for building PIO interface object.
        The function table is an array of function pointers terminated
        by -1 value. The terminator must be present for purpose of
        binary compatibility with future extensions.

    NOTES
        This function table is mandatory to be implemented by the driver.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_DMAVectors

    SYNOPSIS
        [I..], APTR *

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Specifies function table for building DMA interface object. If not supplied,
        the bus is considered not DMA-capable.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_ATABus_PIOVectors

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_IRQHandler

    SYNOPSIS
        [.S.], APTR

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Specifies IRQ handler function to be called when bus interrupt arrives.
        The function should be called using "C" calling convention and has the
        following prototype:
        
            void ata_HandleIRQ(UBYTE status, APTR userdata);
        
        Your driver should pass the following arguments to this function:
            status   - value read from ATA main status register.
            userdata - value of aoHidd_ATABus_IRQData attribute.

    NOTES
        Reading the drive status register is part of the interrupt acknowledge
        process, and therefore has to be done by the driver.

        It is driver's job to check whether the interrupt really belongs to
        the IDE bus. A generic way to do this is to test ATAF_BUSY bit of
        the status register for being zero. However, this may not work
        reliably with IRQ sharing, so advanced IDE controllers may offer
        different, better way to do this.

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_ATABus_IRQData

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_IRQData

    SYNOPSIS
        [.S.], APTR

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Caller's private data to be supplied to IRQ handler function.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_ATABus_IRQData

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_UseIOAlt

    SYNOPSIS
        [..G], BOOL

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Tells whether the bus supports alternate registers bank
        (ata_AltControl and ata_AltStatus).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Default implementation in base class returns value depending on whether
        the subclass provided respective I/O functions in bus interface vector
        table during object creation.

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_KeepEmpty

    SYNOPSIS
        [I..], BOOL

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        If this attribute is set to FALSE during object creation, the object
        will be destroyed if no devices are detected on the bus.

        This can be useful for optional buses like legacy ISA controllers,
        which have no other way to detect their presence.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_Master

    SYNOPSIS
        [..G], OOP_Object *

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Returns a pointer to OOP object of private unit class, representing
        a master drive on the bus, or NULL if there's no master device.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_ATABus_Slave

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_Slave

    SYNOPSIS
        [..G], OOP_Object *

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Returns a pointer to OOP object of private unit class, representing
        a slave drive on the bus, or NULL if there's no master device.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_ATABus_Master

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_CanSetXferMode

    SYNOPSIS
        [..G], BOOL

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Tells whether the bus driver implements moHidd_ATABus_SetXferMode method.

    NOTES

    EXAMPLE

    BUGS
        Current version of ata.device does not use this attribute, and it is
        considered reserved.

    SEE ALSO
        moHidd_ATABus_SetXferMode

    INTERNALS

*****************************************************************************************/

OOP_Object *ATABus__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct ataBase *ATABase = cl->UserData;
    D(bug("[ATA:Bus] %s()\n", __PRETTY_FUNCTION__));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &msg->mID);
    if (o)
    {
        struct ata_Bus *data = OOP_INST_DATA(cl, o);
        struct TagItem *tstate = msg->attrList;
        struct TagItem *tag;

        D(bug("[ATA:Bus] %s: instance @ 0x%p\n", __PRETTY_FUNCTION__, o));

        /* Defaults */
        data->keepEmpty = TRUE;

        while ((tag = NextTagItem(&tstate)))
        {
            ULONG idx;

            Hidd_ATABus_Switch(tag->ti_Tag, idx)
            {
            case aoHidd_ATABus_PIODataSize:
                data->pioDataSize = tag->ti_Data;
                break;

            case aoHidd_ATABus_DMADataSize:
                data->dmaDataSize = tag->ti_Data;
                break;

            case aoHidd_ATABus_BusVectors:
                data->busVectors = (struct ATA_BusInterface *)tag->ti_Data;
                break;

            case aoHidd_ATABus_PIOVectors:
                data->pioVectors = (struct ATA_PIOInterface *)tag->ti_Data;
                break;

            case aoHidd_ATABus_DMAVectors:
                data->dmaVectors = (APTR *)tag->ti_Data;
                break;

            case aoHidd_ATABus_KeepEmpty:
                data->keepEmpty = tag->ti_Data;
                break;
            }
        }

        /* Cache device base pointer. Useful. */
        data->ab_Base = ATABase;

        /* Install reset callback */
        data->ab_ResetInt.is_Node.ln_Name = ATABase->ata_Device.dd_Library.lib_Node.ln_Name;
        data->ab_ResetInt.is_Code         = (VOID_FUNC)ataBus_Reset;
        data->ab_ResetInt.is_Data         = data;
        AddResetCallback(&data->ab_ResetInt);
    }
    return o;
}

void ATABus__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ata_Bus *data = OOP_INST_DATA(cl, o);

    RemResetCallback(&data->ab_ResetInt);

    if (data->dmaInterface)
    {
        void *ptr = data->dmaInterface - sizeof(struct ATA_DMAInterface);

        FreeMem(ptr, sizeof(struct ATA_DMAInterface) + data->dmaDataSize);
    }
    if (data->pioInterface)
    {
        void *ptr = data->pioInterface - sizeof(struct ATA_BusInterface);

        FreeMem(ptr, sizeof(struct ATA_BusInterface) + data->pioDataSize);
    }

    OOP_DoSuperMethod(cl, o, msg);
}

/*
 * Here we take into account that the table can be either
 * terminated early, or have NULL entries.
 */
#define HAVE_VECTOR(x) (x && (x != (APTR)-1))

void ATABus__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct ataBase *ATABase = cl->UserData;
    struct ata_Bus *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_Bus_Switch (msg->attrID, idx)
    {
    case aoHidd_Bus_MaxUnits:
        *msg->storage = MAX_BUSUNITS;
        return;
    }

    Hidd_ATABus_Switch (msg->attrID, idx)
    {
    case aoHidd_ATABus_Use80Wire:
        *msg->storage = FALSE;
        return;

    case aoHidd_ATABus_Use32Bit:
        *msg->storage = (HAVE_VECTOR(data->pioVectors->ata_outsl) &&
                         HAVE_VECTOR(data->pioVectors->ata_insl)) ?
                         TRUE : FALSE;
        return;

    case aoHidd_ATABus_UseDMA:
        *msg->storage = data->dmaVectors ? TRUE : FALSE;
        return;
        
    case aoHidd_ATABus_UseIOAlt:
        *msg->storage = (HAVE_VECTOR(data->busVectors->ata_out_alt) &&
                         HAVE_VECTOR(data->busVectors->ata_in_alt)) ?
                         TRUE : FALSE;
        return;

    case aoHidd_ATABus_CanSetXferMode:
        *msg->storage = FALSE;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

void ATABus__Hidd_StorageBus__EnumUnits(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageBus_EnumUnits *msg)
{
    struct ata_Bus *data = OOP_INST_DATA(cl, o);
    BOOL stop = FALSE;

    D(bug ("[ATA:Bus] Hidd_StorageBus__EnumUnits()\n");)

    if (data->ab_Units[0])
	stop = CALLHOOKPKT(msg->callback, data->ab_Units[0], msg->hookMsg);
    if ((!stop) && (data->ab_Units[1]))
         stop = CALLHOOKPKT(msg->callback, data->ab_Units[1], msg->hookMsg);
}

/* Default ata_out_alt does nothing */
static void default_out_alt(void *obj, UBYTE val, UWORD offset)
{

}

/* Default ata_in_alt wraps AltStatus to status */
static UBYTE default_in_alt(void *obj, UWORD offset)
{
    struct ATA_BusInterface *vec = obj - sizeof(struct ATA_BusInterface);

    return vec->ata_in(obj, ata_Status);
}

static void CopyVectors(APTR *dest, APTR *src, unsigned int num)
{
    unsigned int i;
    
    for (i = 0; i < num; i++)
    {
        if (src[i] == (APTR *)-1)
            return;
        if (src[i])
            dest[i] = src[i];
    }
}

/*****************************************************************************************

    NAME
        moHidd_ATABus_GetPIOInterface

    SYNOPSIS
        APTR OOP_DoMethod(OOP_Object *obj, struct pHidd_ATABus_GetPIOInterface *Msg);

        APTR HIDD_ATABus_GetPIOInterface(void);

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Instantiates encapsulated PIO interface object and returns its
        pointer.

    INPUTS
        None

    RESULT
        A pointer to opaque PIO interface object or NULL in case of failure.

    NOTES
        This method should be overloaded by driver subclasses in order to
        initialize data portion of the interface object.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_ATABus_GetDMAInterface

    INTERNALS
        Interface objects contain not only driver-specific data, but also
        a private vector table. Because of this you cannot just AllocMem()
        the necessary structure in your driver. Always call OOP_DoSuperMethod()
        in order for the base class to instantiate the interface correctly.

*****************************************************************************************/

APTR ATABus__Hidd_ATABus__GetPIOInterface(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ata_Bus *data = OOP_INST_DATA(cl, o);
    struct ATA_BusInterface *vec;

    D(bug("[ATA:Bus] %s()\n", __PRETTY_FUNCTION__));

    vec = AllocMem(sizeof(struct ATA_BusInterface) + data->pioDataSize,
                   MEMF_PUBLIC|MEMF_CLEAR);
    if (vec)
    {
        /* Some default vectors for simplicity */
        vec->ata_out_alt = default_out_alt;
        vec->ata_in_alt  = default_in_alt;

        CopyVectors((APTR *)vec, (APTR *)data->busVectors,
                    sizeof(struct ATA_BusInterface) / sizeof(APTR));

        data->pioInterface = &vec[1];
        return data->pioInterface;
    }

    return NULL;
}

/*****************************************************************************************

    NAME
        moHidd_ATABus_GetDMAInterface

    SYNOPSIS
        APTR OOP_DoMethod(OOP_Object *obj, struct pHidd_ATABus_GetDMAInterface *Msg);

        APTR HIDD_ATABus_GetDMAInterface(void);

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Instantiates encapsulated DMA interface object and returns its
        pointer.

    INPUTS
        None

    RESULT
        A pointer to opaque DMA interface object or NULL upon failure or
        if DMA is not supported by this bus.

    NOTES
        This method should be overloaded by driver subclasses in order to
        initialize data portion of the interface object.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_ATABus_GetPIOInterface

    INTERNALS
        Interface objects contain not only driver-specific data, but also
        a private vector table. Because of this you cannot just AllocMem()
        the necessary structure in your driver. Always call OOP_DoSuperMethod()
        in order for the base class to instantiate the interface correctly.

*****************************************************************************************/

APTR ATABus__Hidd_ATABus__GetDMAInterface(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ata_Bus *data = OOP_INST_DATA(cl, o);
    struct ATA_DMAInterface *vec;

    D(bug("[ATA:Bus] %s()\n", __PRETTY_FUNCTION__));

    if (!data->dmaVectors)
        return NULL;

    vec = AllocMem(sizeof(struct ATA_DMAInterface) + data->dmaDataSize,
                   MEMF_PUBLIC|MEMF_CLEAR);
    if (vec)
    {
        CopyVectors((APTR *)vec, data->dmaVectors,
                    sizeof(struct ATA_DMAInterface) / sizeof(APTR));

        data->dmaInterface = &vec[1];
        return data->dmaInterface;
    }

    return NULL;
}

/*****************************************************************************************

    NAME
        moHidd_ATABus_SetXferMode

    SYNOPSIS
        APTR OOP_DoMethod(OOP_Object *obj, struct pHidd_ATABus_SetXferMode *Msg);

        APTR HIDD_ATABus_SetXferMode(UBYTE unit, ata_XferMode mode);

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Sets the desired transfer mode for the given drive on the bus controller.

    INPUTS
        unit - drive number (0 for master and 1 for slave)
        mode - Mode number (see hidd/ata.h)

    RESULT
        TRUE if successful or FALSE if the desired mode is not supported
        by the hardware.

    NOTES
        The default implementation is provided for drivers not supporting
        DMA and always returns FALSE if the caller attempts to set any of
        DMA modes.

    EXAMPLE

    BUGS
        Current version of ata.device does not use this method, and it is
        considered reserved.

    SEE ALSO
        aoHidd_ATABus_CanSetXferMode

    INTERNALS

*****************************************************************************************/

BOOL ATABus__Hidd_ATABus__SetXferMode(OOP_Class *cl, OOP_Object *o, struct pHidd_ATABus_SetXferMode *msg)
{
    D(bug("[ATA:Bus] %s()\n", __PRETTY_FUNCTION__));

    if ((msg->mode >= AB_XFER_MDMA0) && (msg->mode <= AB_XFER_UDMA6))
    {
        /* DMA is not supported, we cannot set DMA modes */
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************************

    NAME
        moHidd_ATABus_Shutdown

    SYNOPSIS
        APTR OOP_DoMethod(OOP_Object *obj, struct pHidd_ATABus_Shutdown *Msg);

        APTR HIDD_ATABus_Shutdown(void);

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Instantly shutdown all activity on the bus.

    INPUTS
        None

    RESULT
        None

    NOTES
        This method is called by ata.device during system reset handler execution.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Default implementation disables interrupt using AltControl register.

*****************************************************************************************/

void ATABus__Hidd_ATABus__Shutdown(OOP_Class *cl, OOP_Object *o, OOP_Msg *msg)
{
    struct ata_Bus *data = OOP_INST_DATA(cl, o);

    D(bug("[ATA:Bus] %s()\n", __PRETTY_FUNCTION__));

    if (data->pioInterface)
    {
        struct ATA_BusInterface *vec = data->pioInterface - sizeof(struct ATA_BusInterface);

        vec->ata_out_alt(data->pioInterface, ATACTLF_INT_DISABLE, ata_AltControl);
    }
}

/***************** Private nonvirtual methods follow *****************/

BOOL Hidd_ATABus_Start(OOP_Object *o, struct ataBase *ATABase)
{
    struct ata_Bus *ab = OOP_INST_DATA(ATABase->busClass, o);

    D(bug("[ATA:Bus] %s()\n", __PRETTY_FUNCTION__));

    /* Attach IRQ handler */
    OOP_SetAttrsTags(o, aHidd_ATABus_IRQHandler, Hidd_ATABus_HandleIRQ,
                        aHidd_ATABus_IRQData   , ab,
                        TAG_DONE);
    
    /* scan bus - try to locate all devices (disables irq) */    
    ata_InitBus(ab);

    if ((ab->ab_Dev[0] == DEV_NONE) && (ab->ab_Dev[1] == DEV_NONE) &&
        (!ab->keepEmpty))
    {
        /*
         * If there are no devices, and KeepEmpty is not set
         * the bus will be thrown away.
         */
        return FALSE;
    }

    /*
     * Assign bus number.
     * TODO:
     * 1. This does not take into account possibility to
     * unload drivers. In this case existing units will disappear,
     * freeing up their numbers. These numbers should be reused.
     * 2. We REALLY need modify-and-fetch atomics.
     */
    Forbid();
    ab->ab_BusNum = ATABase->ata__buscount++;
    Permit();

    if ((ab->ab_Dev[0] < DEV_ATA) && (ab->ab_Dev[1] < DEV_ATA))
    {
        /* Do not start up task if there are no usable devices. */
        return TRUE;
    }

    /*
     * This small trick is based on the fact that shared semaphores
     * have no specific owner. You can obtain and release them from
     * within any task. It will block only on attempt to re-lock it
     * in exclusive mode.
     * So instead of complex handshake we obtain the semaphore before
     * starting bus task. It will release the semaphore when done.
     */
    ObtainSemaphoreShared(&ATABase->DetectionSem);
    
    /*
     * Start up bus task. It will perform scanning asynchronously, and
     * then, if successful, insert units. This allows to keep things parallel.
     */
    D(bug("[ATA>>] Start: Bus %u: Unit 0 - %d, Unit 1 - %d\n", ab->ab_BusNum, ab->ab_Dev[0], ab->ab_Dev[1]));
    return NewCreateTask(TASKTAG_PC         , BusTaskCode,
                         TASKTAG_NAME       , "ATA[PI] Subsystem",
                         TASKTAG_STACKSIZE  , STACK_SIZE,
                         TASKTAG_PRI        , TASK_PRI,
                         TASKTAG_TASKMSGPORT, &ab->ab_MsgPort,
                         TASKTAG_ARG1       , ab,
                         TASKTAG_ARG2       , ATABase,
                         TAG_DONE) ? TRUE : FALSE;
}

AROS_UFH3(BOOL, Hidd_ATABus_Open,
          AROS_UFHA(struct Hook *, h, A0),
          AROS_UFHA(OOP_Object *, obj, A2),
          AROS_UFHA(IPTR, reqUnit, A1))
{
    AROS_USERFUNC_INIT

    struct IORequest *req = h->h_Data;
    struct ataBase *ATABase = (struct ataBase *)req->io_Device;
    struct ata_Bus *b = (struct ata_Bus *)OOP_INST_DATA(ATABase->busClass, obj);
    ULONG bus = reqUnit >> 1;
    UBYTE dev = reqUnit & 1;

    D(bug("[ATA:Bus] %s()\n", __PRETTY_FUNCTION__));
    D(bug("[ATA%02ld] Checking bus %u dev %u\n", reqUnit, bus, dev));
    
    if ((b->ab_BusNum == bus) && b->ab_Units[dev])
    {
        struct ata_Unit *unit = (struct ata_Unit *)OOP_INST_DATA(ATABase->unitClass, b->ab_Units[dev]);

        /* Got the unit */
        req->io_Unit  = &unit->au_Unit;
        req->io_Error = 0;

        unit->au_Unit.unit_OpenCnt++;
        return TRUE;
    }
    
    return FALSE;

    AROS_USERFUNC_EXIT
}
