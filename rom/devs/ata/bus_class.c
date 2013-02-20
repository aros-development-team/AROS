#include <aros/debug.h>
#include <hidd/ata.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "bus_class.h"
#include "interface_pio.h"
#include "interface_dma.h"

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
        order to curcumvent this limitation, additionally to normal OOP API
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
        This interface is mandatory and must be implemented by the driver.

        Function table for the interface consists of the following functions:

        VOID ata_out(void *obj, UBYTE val, UWORD offset)
        - Write byte into primary register bank with the given offset.

        UBYTE ata_in(void *obj, UWORD offset)
        - Read byte from primary register bank with the given offset.

        VOID ata_out_alt(void *obj, UBYTE val, UWORD offset)
        - Write byte into alternate register bank with the given offset.

        UBYTE ata_in_alt(void *obj, UWORD offset)
        - Read byte from alternate register bank with the given offset.

        VOID ata_outsw(void *obj, APTR address, ULONG count)
        - Perform 16-bit PIO data write operation from the given memory
          region of the given size.

        UBYTE ata_insw(void *obj, APTR address, ULONG count)
        - Perform 16-bit PIO data read operation into the given memory
          region of the given size.

        VOID ata_ackInt(void *obj)
        - Acknowledge PIO interrupt. This routine is optional.

        The interface implementation in the driver may include also second
        function table for 32-bit PIO implementation:
        
        VOID ata_outsl(void *obj, APTR address, ULONG count)
        - Perform 32-bit PIO data write operation from the given memory
          region of the given size.

        UBYTE ata_insl(void *obj, APTR address, ULONG count)
        - Perform 32-bit PIO data read operation into the given memory
          region of the given size.

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

        VOID dma_Cleanup(void *obj, APTR buffer, IPTR size, BOOL read)
        - Perform post-transfer cleanup of the given region.

        VOID dma_Start(void *obj)
        - Start DMA transfer.

        VOID dma_Stop(void *obj)
        - Stop DMA transfer.

        BOOL dma_IntStatus(void *obj)
        - To be documented, the design may change.

        ULONG dma_Result(void *obj)
        - Get resulting status of the operation. The function should return 0
          for succesful completion or error code to be passed up to ata.device
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
        aoHidd_ATABus_PIOVectors

    SYNOPSIS
        [I..], APTR *

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Specifies function table for building PIO interface object.
        The function table is an array of function pointers terminated
        by -1 value. The terminator must be present for purpose of
        binary compatibility with future extensions.

    NOTES
        This function table is mandatory to be implemented by the driver.

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_ATABus_PIO32Vectors

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATABus_PIO32Vectors

    SYNOPSIS
        [I..], APTR *

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Specifies function table for 32-bit PIO transfers. These functions
        are also part of PIO interface object, however this table is optional.
        If it is not supplied, the bus is considered 16-bit only.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_ATABus_PIOVectors

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
        [I..], APTR

    LOCATION
        CLID_Hidd_ATABus

    FUNCTION
        Specifies IRQ handler function to be called when bus interrupt arrives.
        The function shoule be called using "C" calling convention and has the
        following prototype:
        
            void ata_HandleIRQ(APTR IRQData);
        
        Your driver should pass value of aoHidd_ATABus_IRQData argument to this
        function.

    NOTES

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
        [I..], APTR

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

OOP_Object *ATABus__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    o = OOP_DoSuperMethod(cl, o, &msg->mID);
    if (o)
    {
        struct ATA_BusData *data = OOP_INST_DATA(cl, o);
        struct TagItem *tstate = msg->attrList;
        struct TagItem *tag;
 
        while ((tag = NextTagItem(&tstate)))
        {
            if (tag->ti_Tag == aHidd_ATABus_PIODataSize)
                data->pioDataSize = tag->ti_Data;
            else if (tag->ti_Tag == aHidd_ATABus_DMADataSize)
                data->dmaDataSize = tag->ti_Data;
            else if (tag->ti_Tag == aHidd_ATABus_PIOVectors)
                data->pioVectors = tag->ti_Data;
            else if (tag->ti_Tag == aHidd_ATABus_PIO32Vectors)
                data->pio32Vectors = tag->ti_Data;
            else if (tag->ti_Tag == aHidd_ATABus_DMAVectors)
                data->dmaVectors = tag->ti_Data;
        }
    }
    return o;
}

OOP_Object *ATABus__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);

    if (data->dmaInterface)
        FreeMem(data->dmaInterface, sizeof(struct ATA_DMAInterface) + data->pioDataSize);
    if (data->pioInterface)
        FreeMem(data->pioInterface, sizeof(struct ATA_PIOInterface) + data->dmaDataSize);
    
    OOP_DoSuperMethod(cl, o, msg);
}

void ATABus__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct ataBase *base = cl->cl_UserData;
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_HIDD_ATABUS_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_ATABus_Use80Wire:
            /* CHECKME: Is there any generic way to check this */
            *msg->storage = FALSE;
            return;

        case aoHidd_ATABus_Use32Bit:
            *msg->storage = data->pio32Vectors ? TRUE : FALSE;
            return;

        case aoHidd_ATABus_UseDMA:
            *msg->storage = data->dmaVectors ? TRUE : FALSE;
            return;
        }
    }

    OOP_DoSuperMethod(&msg->mID);
}

void ATABus__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    struct TagItem *tstate = msg->attrList;
    struct TagItem *tag;
    
    while ((tag = NextTagItem(&tstate)))
    {
        if (tag->ti_Tag == aHidd_ATABus_Use32Bit)
        {
            if (data->pio32Vectors)
            {
                /* Changing mode is done by patching PIO interface's vector table */
                if (tag->ti_Data)
                {
                    data->pioInterface->ata_outsw = data->pio32Vectors->ata_outsl;
                    data->pioInterface->ata_insw  = data->pio32Vectors->ata_insl;
                }
                else
                {
                    data->pioInterface->ata_outsw = data->pioVectors->ata_outsw;
                    data->pioInterface->ata_insw  = data->pioVectors->ata_insw;
                }
            }
        }
    }
}

static void default_AckInt(void *obj)
{
    /* Nothing to do here in most cases */
}

static void CopyVectors(APTR *dest, APTR *src, int num)
{
    int i;
    
    for (i = 0; i < num; i++)
    {
        if (src[i] == (APTR *)-1)
            return;
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

APTR ATABus__Hidd_ATABus__GetPIOInterface(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    struct ATA_PIOInterface *vec;
    
    vec = AllocMem(sizeof(struct ATA_PIOInterface) + data->pioDataSize);
    if (vec)
    {
        vec->ata_AckInt = default_AckInt;
        CopyVectors((APTR *)vec, data->pioVectors,
                    sizeof(struct ATA_PIOInterface) / sizeof(APTR));

        data->pioInterface = vec;
        return &vec[1];
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

APTR ATABus__Hidd_ATABus__GetDMAInterface(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    struct ATA_DMAInterface *vec;

    if (!data-dmaVectors)
        return NULL;
    
    vec = AllocMem(sizeof(struct ATA_DMAInterface) + data->dmaDataSize);
    if (vec)
    {
        CopyVectors((APTR *)vec, data->pioVectors,
                    sizeof(struct ATA_PIOInterface) / sizeof(APTR));

        data->dmaInterface = vec;
        return &vec[1];
    }

    return NULL;
}

/*****************************************************************************************

    NAME
	moHidd_ATABus_SetXferMode

    SYNOPSIS
	APTR OOP_DoMethod(OOP_Object *obj, struct pHidd_ATABus_SetXferMode *Msg);

	APTR HIDD_ATABus_SetXferMode(ata_XferMode mode);

    LOCATION
	CLID_Hidd_ATABus

    FUNCTION
        Sets the desired transfer mode on the bus controller.

    INPUTS
	Mode number (see hidd/ata.h)

    RESULT
	TRUE if succesful or FALSE if the desired mode is nut supported
        by the hardware.

    NOTES
        The default implementation is provided for drivers not supporting
        DMA and always returns FALSE if the caller attempts to set any of
        DMA modes.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

BOOL ATABus__Hidd_ATABus__SetXferMode(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);

    if ((msg->mode >= AB_XFER_MDMA0) && (msg->mode <= AB_XFER_UDMA6))
    {
        /* DMA is not supported, we cannot set DMA modes */
        return FALSE;
    }

    return TRUE;
}
