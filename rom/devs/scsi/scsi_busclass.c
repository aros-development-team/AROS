/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/oop.h>
#include <proto/utility.h>

#include <hidd/storage.h>
#include <hidd/bus.h>
#include <hidd/scsi.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "scsi.h"
#include "scsi_Bus.h"

#define DIRQ(x)
#define DATTR(x)

static void Hidd_SCSIBus_HandleIRQ(UBYTE status, struct scsi_Bus *bus)
{
    struct scsi_Unit *unit = bus->sb_SelectedUnit;

    /*
     * don't waste your time on checking other devices.
     * pass irq ONLY if task is expecting one;
     */
    if (unit && bus->sb_HandleIRQ)
    {
        /* ok, we have a routine to handle any form of transmission etc. */
        DIRQ(bug("[SCSI%02d] IRQ: Calling dedicated handler 0x%p... \n",
                 unit->su_UnitNum, bus->sb_HandleIRQ));
        bus->sb_HandleIRQ(unit, status);

        return;
    }

    DIRQ({
        /*
         * if we got *here* then device is most likely not expected to have an irq.
         */
        bug("[SCSI%02d] Spurious IRQ\n", unit ? unit->su_UnitNum : -1);

        if (0 == (ATAF_BUSY & status))
        {
            bug("[SCSI  ] STATUS: %02lx\n"     , status);
            bug("[SCSI  ] ALT STATUS: %02lx\n" , PIO_InAlt(bus, scsi_AltStatus));
            bug("[SCSI  ] ERROR: %02lx\n"      , PIO_In(bus, scsi_Error));
            bug("[SCSI  ] IRQ: REASON: %02lx\n", PIO_In(bus, atapi_Reason));
        }
    });
}

static AROS_INTH1(ataBus_Reset, struct scsi_Bus *, bus)
{
    AROS_INTFUNC_INIT

    struct scsiBase *SCSIBase = bus->sb_Base;
    OOP_Object *obj = (void *)bus - SCSIBase->busClass->InstOffset;

    D(bug("[SCSI:Bus] %s()\n", __func__));

    HIDD_SCSIBus_Shutdown(obj);

    return FALSE;

    AROS_INTFUNC_EXIT
}

/*****************************************************************************************

    NAME
        --background--

    LOCATION
        CLID_Hidd_SCSIBus

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
        CLID_Hidd_SCSIBus

    NOTES
        PIO interface is responsible for accessing I/O registers on the IDE
        bus, as well as performing PIO-mode 16- and 32-bit data transfers.
        This interface is mandatory and must be implemented by the driver,
        however some functions are optional. They can be either omitted
        entirely from the function table, or set to NULL pointers.

        Control functions table for the interface consists of the following
        functions (listed in their order in the array):

        VOID scsi_out(void *obj, UBYTE val, UWORD offset)
        - Write byte into primary register bank with the given offset.

        UBYTE scsi_in(void *obj, UWORD offset)
        - Read byte from primary register bank with the given offset.

        VOID scsi_out_alt(void *obj, UBYTE val, UWORD offset)
        - Write byte into alternate register bank with the given offset.
          This function is optional.

        UBYTE scsi_in_alt(void *obj, UWORD offset)
        - Read byte from alternate register bank with the given offset.
          This function is optional.

        Transfer functions table for the interface consists of the following
        functions (listed in their order in the array):

        VOID scsi_outsw(void *obj, APTR address, ULONG count)
        - Perform 16-bit PIO data write operation from the given memory
          region of the given size.

        VOID scsi_insw(void *obj, APTR address, ULONG count)
        - Perform 16-bit PIO data read operation into the given memory
          region of the given size.

        VOID scsi_outsl(void *obj, APTR address, ULONG count)
        - Perform 32-bit PIO data write operation from the given memory
          region of the given size. This function is optional.

        UBYTE scsi_insl(void *obj, APTR address, ULONG count)
        - Perform 32-bit PIO data read operation into the given memory
          region of the given size. This function is optional.

*****************************************************************************************/
/*****************************************************************************************

    NAME
        --DMA interface--

    LOCATION
        CLID_Hidd_SCSIBus

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
        aoHidd_SCSIBus_Use80Wire

    SYNOPSIS
        [..G], BOOL

    LOCATION
        CLID_Hidd_SCSIBus

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
        aoHidd_SCSIBus_Use32Bit

    SYNOPSIS
        [.SG], BOOL

    LOCATION
        CLID_Hidd_SCSIBus

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
        aoHidd_SCSIBus_UseDMA

    SYNOPSIS
        [..G], BOOL

    LOCATION
        CLID_Hidd_SCSIBus

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
        aoHidd_SCSIBus_PIODataSize

    SYNOPSIS
        [I..], BOOL

    LOCATION
        CLID_Hidd_SCSIBus

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
        aoHidd_SCSIBus_DMADataSize

    SYNOPSIS
        [I..], BOOL

    LOCATION
        CLID_Hidd_SCSIBus

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
        aoHidd_SCSIBus_BusVectors

    SYNOPSIS
        [I..], APTR *

    LOCATION
        CLID_Hidd_SCSIBus

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
        aoHidd_SCSIBus_PIOVectors

    SYNOPSIS
        [I..], APTR *

    LOCATION
        CLID_Hidd_SCSIBus

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
        aoHidd_SCSIBus_DMAVectors

    SYNOPSIS
        [I..], APTR *

    LOCATION
        CLID_Hidd_SCSIBus

    FUNCTION
        Specifies function table for building DMA interface object. If not supplied,
        the bus is considered not DMA-capable.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_SCSIBus_PIOVectors

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_SCSIBus_UseIOAlt

    SYNOPSIS
        [..G], BOOL

    LOCATION
        CLID_Hidd_SCSIBus

    FUNCTION
        Tells whether the bus supports alternate registers bank
        (scsi_AltControl and scsi_AltStatus).

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
        aoHidd_SCSIBus_Master

    SYNOPSIS
        [..G], OOP_Object *

    LOCATION
        CLID_Hidd_SCSIBus

    FUNCTION
        Returns a pointer to OOP object of private unit class, representing
        a master drive on the bus, or NULL if there's no master device.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_SCSIBus_Slave

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_SCSIBus_Slave

    SYNOPSIS
        [..G], OOP_Object *

    LOCATION
        CLID_Hidd_SCSIBus

    FUNCTION
        Returns a pointer to OOP object of private unit class, representing
        a slave drive on the bus, or NULL if there's no master device.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_SCSIBus_Master

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_SCSIBus_CanSetXferMode

    SYNOPSIS
        [..G], BOOL

    LOCATION
        CLID_Hidd_SCSIBus

    FUNCTION
        Tells whether the bus driver implements moHidd_SCSIBus_SetXferMode method.

    NOTES

    EXAMPLE

    BUGS
        Current version of ata.device does not use this attribute, and it is
        considered reserved.

    SEE ALSO
        moHidd_SCSIBus_SetXferMode

    INTERNALS

*****************************************************************************************/

OOP_Object *SCSIBus__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct scsiBase *SCSIBase = cl->UserData;
    D(bug("[SCSI:Bus] %s()\n", __func__));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &msg->mID);
    if (o)
    {
        struct scsi_Bus *data = OOP_INST_DATA(cl, o);
        struct TagItem *tstate = msg->attrList;
        struct TagItem *tag;

        D(
          bug("[SCSI:Bus] %s: instance @ 0x%p\n", __func__, o);
          bug("[SCSI:Bus] %s: scsi_Bus @ 0x%p\n", __func__, data);
         )

        /* Defaults */
        data->keepEmpty = TRUE;

        while ((tag = NextTagItem(&tstate)))
        {
            ULONG idx;

            Hidd_SCSIBus_Switch(tag->ti_Tag, idx)
            {
            case aoHidd_SCSIBus_PIODataSize:
                data->pioDataSize = tag->ti_Data;
                D(bug("[SCSI:Bus] %s: PIODataSize = %d\n", __func__, data->pioDataSize);)
                break;

            case aoHidd_SCSIBus_DMADataSize:
                data->dmaDataSize = tag->ti_Data;
                DATTR(bug("[SCSI:Bus] %s: DMADataSize = %d\n", __func__, data->dmaDataSize);)
                break;

            case aoHidd_SCSIBus_BusVectors:
                data->busVectors = (struct SCSI_BusInterface *)tag->ti_Data;
                DATTR(bug("[SCSI:Bus] %s: BusVectors @ 0x%p\n", __func__, data->busVectors);)
                break;

            case aoHidd_SCSIBus_PIOVectors:
                data->pioVectors = (struct SCSI_PIOInterface *)tag->ti_Data;
                DATTR(bug("[SCSI:Bus] %s: PIOVectors @ 0x%p\n", __func__, data->pioVectors);)
                break;

            case aoHidd_SCSIBus_DMAVectors:
                data->dmaVectors = (APTR *)tag->ti_Data;
                DATTR(bug("[SCSI:Bus] %s: DMAVectors @ 0x%p\n", __func__, data->dmaVectors);)
                break;
            }
            Hidd_Bus_Switch(tag->ti_Tag, idx)
            {
            case aoHidd_Bus_KeepEmpty:
                data->keepEmpty = tag->ti_Data;
                break;
            }
        }

        /* Cache device base pointer. Useful. */
        data->sb_Base = SCSIBase;

        /* Install reset callback */
        data->sb_ResetInt.is_Node.ln_Name = SCSIBase->scsi_Device.dd_Library.lib_Node.ln_Name;
        data->sb_ResetInt.is_Code         = (VOID_FUNC)ataBus_Reset;
        data->sb_ResetInt.is_Data         = data;
        AddResetCallback(&data->sb_ResetInt);
    }
    return o;
}

void SCSIBus__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct scsi_Bus *data = OOP_INST_DATA(cl, o);

    RemResetCallback(&data->sb_ResetInt);

    if (data->dmaInterface)
    {
        void *ptr = data->dmaInterface - sizeof(struct SCSI_DMAInterface);

        FreeMem(ptr, sizeof(struct SCSI_DMAInterface) + data->dmaDataSize);
    }
    if (data->pioInterface)
    {
        void *ptr = data->pioInterface - sizeof(struct SCSI_BusInterface);

        FreeMem(ptr, sizeof(struct SCSI_BusInterface) + data->pioDataSize);
    }

    OOP_DoSuperMethod(cl, o, msg);
}

/*
 * Here we take into account that the table can be either
 * terminated early, or have NULL entries.
 */
#define HAVE_VECTOR(x) (x && (x != (APTR)-1))

void SCSIBus__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct scsiBase *SCSIBase = cl->UserData;
    struct scsi_Bus *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_Bus_Switch (msg->attrID, idx)
    {
    case aoHidd_Bus_MaxUnits:
        *msg->storage = MAX_BUSUNITS;
        return;
    }

    Hidd_SCSIBus_Switch (msg->attrID, idx)
    {
    case aoHidd_SCSIBus_Use80Wire:
        *msg->storage = FALSE;
        return;

    case aoHidd_SCSIBus_Use32Bit:
        *msg->storage = (HAVE_VECTOR(data->pioVectors->scsi_outsl) &&
                         HAVE_VECTOR(data->pioVectors->scsi_insl)) ?
                         TRUE : FALSE;
        return;

    case aoHidd_SCSIBus_UseDMA:
        *msg->storage = data->dmaVectors ? TRUE : FALSE;
        return;
        
    case aoHidd_SCSIBus_UseIOAlt:
        *msg->storage = (HAVE_VECTOR(data->busVectors->scsi_out_alt) &&
                         HAVE_VECTOR(data->busVectors->scsi_in_alt)) ?
                         TRUE : FALSE;
        return;

    case aoHidd_SCSIBus_CanSetXferMode:
        *msg->storage = FALSE;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

void SCSIBus__Hidd_StorageBus__EnumUnits(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageBus_EnumUnits *msg)
{
    struct scsi_Bus *data = OOP_INST_DATA(cl, o);
    BOOL stop = FALSE;

    D(bug ("[SCSI:Bus] Hidd_StorageBus__EnumUnits()\n");)

    if (data->sb_Units[0])
	stop = CALLHOOKPKT(msg->callback, data->sb_Units[0], msg->hookMsg);
    if ((!stop) && (data->sb_Units[1]))
         stop = CALLHOOKPKT(msg->callback, data->sb_Units[1], msg->hookMsg);
}

/* Default scsi_out_alt does nothing */
static void default_out_alt(void *obj, UBYTE val, UWORD offset)
{

}

/* Default scsi_in_alt wraps AltStatus to status */
static UBYTE default_in_alt(void *obj, UWORD offset)
{
    struct SCSI_BusInterface *vec = obj - sizeof(struct SCSI_BusInterface);

    return vec->scsi_in(obj, scsi_Status);
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
        moHidd_SCSIBus_GetPIOInterface

    SYNOPSIS
        APTR OOP_DoMethod(OOP_Object *obj, struct pHidd_SCSIBus_GetPIOInterface *Msg);

        APTR HIDD_SCSIBus_GetPIOInterface(void);

    LOCATION
        CLID_Hidd_SCSIBus

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
        moHidd_SCSIBus_GetDMAInterface

    INTERNALS
        Interface objects contain not only driver-specific data, but also
        a private vector table. Because of this you cannot just AllocMem()
        the necessary structure in your driver. Always call OOP_DoSuperMethod()
        in order for the base class to instantiate the interface correctly.

*****************************************************************************************/

APTR SCSIBus__Hidd_SCSIBus__GetPIOInterface(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct scsi_Bus *data = OOP_INST_DATA(cl, o);
    struct SCSI_BusInterface *vec;

    D(
      bug("[SCSI:Bus] %s()\n", __func__);
      bug("[SCSI:Bus] %s: scsi_Bus @ 0x%p\n", __func__, data);
     )

    vec = AllocMem(sizeof(struct SCSI_BusInterface) + data->pioDataSize,
                   MEMF_PUBLIC|MEMF_CLEAR);
    if (vec)
    {
        D(bug("[SCSI:Bus] %s: SCSI_BusInterface @ 0x%p (%d bytes + %d)\n", __func__, vec, sizeof(struct SCSI_BusInterface), data->pioDataSize);)

        /* Some default vectors for simplicity */
        vec->scsi_out_alt = default_out_alt;
        vec->scsi_in_alt  = default_in_alt;

        CopyVectors((APTR *)vec, (APTR *)data->busVectors,
                    sizeof(struct SCSI_BusInterface) / sizeof(APTR));

        data->pioInterface = &vec[1];
        return data->pioInterface;
    }

    return NULL;
}

/*****************************************************************************************

    NAME
        moHidd_SCSIBus_GetDMAInterface

    SYNOPSIS
        APTR OOP_DoMethod(OOP_Object *obj, struct pHidd_SCSIBus_GetDMAInterface *Msg);

        APTR HIDD_SCSIBus_GetDMAInterface(void);

    LOCATION
        CLID_Hidd_SCSIBus

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
        moHidd_SCSIBus_GetPIOInterface

    INTERNALS
        Interface objects contain not only driver-specific data, but also
        a private vector table. Because of this you cannot just AllocMem()
        the necessary structure in your driver. Always call OOP_DoSuperMethod()
        in order for the base class to instantiate the interface correctly.

*****************************************************************************************/

APTR SCSIBus__Hidd_SCSIBus__GetDMAInterface(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct scsi_Bus *data = OOP_INST_DATA(cl, o);
    struct SCSI_DMAInterface *vec;

    D(bug("[SCSI:Bus] %s()\n", __func__));

    if (!data->dmaVectors)
        return NULL;

    vec = AllocMem(sizeof(struct SCSI_DMAInterface) + data->dmaDataSize,
                   MEMF_PUBLIC|MEMF_CLEAR);
    if (vec)
    {
        CopyVectors((APTR *)vec, data->dmaVectors,
                    sizeof(struct SCSI_DMAInterface) / sizeof(APTR));

        data->dmaInterface = &vec[1];
        return data->dmaInterface;
    }

    return NULL;
}

/*****************************************************************************************

    NAME
        moHidd_SCSIBus_SetXferMode

    SYNOPSIS
        APTR OOP_DoMethod(OOP_Object *obj, struct pHidd_SCSIBus_SetXferMode *Msg);

        APTR HIDD_SCSIBus_SetXferMode(UBYTE unit, scsi_XferMode mode);

    LOCATION
        CLID_Hidd_SCSIBus

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
        aoHidd_SCSIBus_CanSetXferMode

    INTERNALS

*****************************************************************************************/

BOOL SCSIBus__Hidd_SCSIBus__SetXferMode(OOP_Class *cl, OOP_Object *o, struct pHidd_SCSIBus_SetXferMode *msg)
{
    D(bug("[SCSI:Bus] %s()\n", __func__));

    if ((msg->mode >= AB_XFER_MDMA0) && (msg->mode <= AB_XFER_UDMA6))
    {
        /* DMA is not supported, we cannot set DMA modes */
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************************

    NAME
        moHidd_SCSIBus_Shutdown

    SYNOPSIS
        APTR OOP_DoMethod(OOP_Object *obj, struct pHidd_SCSIBus_Shutdown *Msg);

        APTR HIDD_SCSIBus_Shutdown(void);

    LOCATION
        CLID_Hidd_SCSIBus

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

void SCSIBus__Hidd_SCSIBus__Shutdown(OOP_Class *cl, OOP_Object *o, OOP_Msg *msg)
{
    struct scsi_Bus *data = OOP_INST_DATA(cl, o);

    D(bug("[SCSI:Bus] %s()\n", __func__));

    if (data->pioInterface)
    {
        struct SCSI_BusInterface *vec = data->pioInterface - sizeof(struct SCSI_BusInterface);

        vec->scsi_out_alt(data->pioInterface, SCSICTLF_INT_DISABLE, scsi_AltControl);
    }
}

/***************** Private nonvirtual methods follow *****************/

BOOL Hidd_SCSIBus_Start(OOP_Object *o, struct scsiBase *SCSIBase)
{
    struct scsi_Bus *sb = OOP_INST_DATA(SCSIBase->busClass, o);

    D(bug("[SCSI:Bus] %s()\n", __func__));

    /* Attach IRQ handler */
    OOP_SetAttrsTags(o, aHidd_Bus_IRQHandler, Hidd_SCSIBus_HandleIRQ,
                        aHidd_Bus_IRQData   , sb,
                        TAG_DONE);
    
    /* scan bus - try to locate all devices (disables irq) */    
    scsi_InitBus(sb);

    if ((sb->sb_Dev[0] == DEV_NONE) && (sb->sb_Dev[1] == DEV_NONE) &&
        (!sb->keepEmpty))
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
    sb->sb_BusNum = SCSIBase->scsi__buscount++;
    Permit();

    if ((sb->sb_Dev[0] < DEV_ATA) && (sb->sb_Dev[1] < DEV_ATA))
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
    ObtainSemaphoreShared(&SCSIBase->DetectionSem);
    
    /*
     * Start up bus task. It will perform scanning asynchronously, and
     * then, if successful, insert units. This allows to keep things parallel.
     */
    D(bug("[SCSI>>] Start: Bus %u: Unit 0 - %d, Unit 1 - %d\n", sb->sb_BusNum, sb->sb_Dev[0], sb->sb_Dev[1]));
    return NewCreateTask(TASKTAG_PC         , BusTaskCode,
                         TASKTAG_NAME       , "ATA[PI] Subsystem",
                         TASKTAG_STACKSIZE  , STACK_SIZE,
                         TASKTAG_PRI        , TASK_PRI,
                         TASKTAG_TASKMSGPORT, &sb->sb_MsgPort,
                         TASKTAG_ARG1       , sb,
                         TASKTAG_ARG2       , SCSIBase,
                         TAG_DONE) ? TRUE : FALSE;
}

AROS_UFH3(BOOL, Hidd_SCSIBus_Open,
          AROS_UFHA(struct Hook *, h, A0),
          AROS_UFHA(OOP_Object *, obj, A2),
          AROS_UFHA(IPTR, reqUnit, A1))
{
    AROS_USERFUNC_INIT

    struct IORequest *req = h->h_Data;
    struct scsiBase *SCSIBase = (struct scsiBase *)req->io_Device;
    struct scsi_Bus *sb = (struct scsi_Bus *)OOP_INST_DATA(SCSIBase->busClass, obj);
    ULONG bus = reqUnit >> 1;
    UBYTE dev = reqUnit & 1;

    D(bug("[SCSI:Bus] %s()\n", __func__));
    D(bug("[SCSI%02ld] Checking bus %u dev %u\n", reqUnit, bus, dev));
    
    if ((sb->sb_BusNum == bus) && sb->sb_Units[dev])
    {
        struct scsi_Unit *unit = (struct scsi_Unit *)OOP_INST_DATA(SCSIBase->unitClass, sb->sb_Units[dev]);

        /* Got the unit */
        req->io_Unit  = &unit->su_Unit;
        req->io_Error = 0;

        unit->su_Unit.unit_OpenCnt++;
        return TRUE;
    }
    
    return FALSE;

    AROS_USERFUNC_EXIT
}
