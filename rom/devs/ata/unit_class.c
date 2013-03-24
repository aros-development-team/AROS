#include <aros/debug.h>
#include <hidd/ata.h>
#include <oop/oop.h>
#include <proto/exec.h>

#include "ata.h"

/*****************************************************************************************

    NAME
        --background--

    LOCATION
        IID_Hidd_ATAUnit

    NOTES
        Unit class is private to ata.device. Instances of this class represent
        devices connected to IDE buses, and can be used to obtain information
        about these devices.

*****************************************************************************************/

/*
 * a STUB function for commands not supported by this particular device
 */
static BYTE ata_STUB(struct ata_Unit *au)
{
    D(bug("[ATA%02ld] CALLED STUB FUNCTION (GENERIC). THIS OPERATION IS NOT "
        "SUPPORTED BY DEVICE\n", au->au_UnitNum));
    return CDERR_NOCMD;
}

static BYTE ata_STUB_IO32(struct ata_Unit *au, ULONG blk, ULONG len,
    APTR buf, ULONG* act)
{
    D(bug("[ATA%02ld] CALLED STUB FUNCTION (IO32). THIS OPERATION IS NOT "
        "SUPPORTED BY DEVICE\n", au->au_UnitNum));
    return CDERR_NOCMD;
}

static BYTE ata_STUB_IO64(struct ata_Unit *au, UQUAD blk, ULONG len,
    APTR buf, ULONG* act)
{
    D(bug("[ATA%02ld] CALLED STUB FUNCTION -- IO ACCESS TO BLOCK %08lx:%08lx, LENGTH %08lx. THIS OPERATION IS NOT SUPPORTED BY DEVICE\n", au->au_UnitNum, (blk >> 32), (blk & 0xffffffff), len));
    return CDERR_NOCMD;
}

static BYTE ata_STUB_SCSI(struct ata_Unit *au, struct SCSICmd* cmd)
{
    D(bug("[ATA%02ld] CALLED STUB FUNCTION. THIS OPERATION IS NOT SUPPORTED BY DEVICE\n", au->au_UnitNum));
    return CDERR_NOCMD;
}

OOP_Object *ATAUnit__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &msg->mID);
    if (o)
    {
        struct ataBase *ATABase = cl->UserData;
        struct ata_Unit *unit = OOP_INST_DATA(cl, o);

        unit->au_Drive = AllocPooled(ATABase->ata_MemPool, sizeof(struct DriveIdent));
        if (!unit->au_Drive)
        {
            OOP_MethodID disp_msg = msg->mID - moRoot_New + moRoot_Dispose;
            
            OOP_DoSuperMethod(cl, o, &disp_msg);
            return NULL;
        }

        unit->au_SectorShift = 9; /* this really has to be set here. */

        NEWLIST(&unit->au_SoftList);

        /*
         * since the stack is always handled by caller
         * it's safe to stub all calls with one function
         */
        unit->au_Read32     = ata_STUB_IO32;
        unit->au_Read64     = ata_STUB_IO64;
        unit->au_Write32    = ata_STUB_IO32;
        unit->au_Write64    = ata_STUB_IO64;
        unit->au_Eject      = ata_STUB;
        unit->au_DirectSCSI = ata_STUB_SCSI;
        unit->au_Identify   = ata_STUB;
    }
    return o;
}

void ATAUnit__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ataBase *ATABase = cl->UserData;
    struct ata_Unit *unit = OOP_INST_DATA(cl, o);

    FreePooled(ATABase->ata_MemPool, unit->au_Drive, sizeof(struct DriveIdent));
    OOP_DoSuperMethod(cl, o, msg);
}

/*****************************************************************************************

    NAME
        aoHidd_ATAUnit_Number

    SYNOPSIS
        [..G], ULONG

    LOCATION
        IID_Hidd_ATAUnit

    FUNCTION
        Returns number of ata.device unit corresponding to this device.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATAUnit_Model

    SYNOPSIS
        [..G], CONST_STRPTR

    LOCATION
        IID_Hidd_ATAUnit

    FUNCTION
        Returns model ID string for this device.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_ATAUnit_Revision, aoHidd_ATAUnit_Serial

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATAUnit_Revision

    SYNOPSIS
        [..G], CONST_STRPTR

    LOCATION
        IID_Hidd_ATAUnit

    FUNCTION
        Returns revision ID string for this device.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_ATAUnit_Model, aoHidd_ATAUnit_Serial

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATAUnit_Serial

    SYNOPSIS
        [..G], CONST_STRPTR

    LOCATION
        IID_Hidd_ATAUnit

    FUNCTION
        Returns serial number string for this device.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_ATAUnit_Model, aoHidd_ATAUnit_Revision

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATAUnit_XferModes

    SYNOPSIS
        [..G], ULONG

    LOCATION
        IID_Hidd_ATAUnit

    FUNCTION
        Tells which transfer modes are supported by this device. The returned value
        is a bitwise combination of the following flags (see include/hidd/ata.h):

        AF_XFER_PIO(x)  - PIO mode number x (0 - 4)
        AF_XFER_MDMA(x) - multiword DMA mode number x (0 - 2)
        AF_XFER_UDMA(x) - Ultra DMA mode number x (0 - 6)
        AF_XFER_48BIT   - LBA48 block addressing
        AF_XFER_RWMILTI - Multisector PIO
        AF_XFER_PACKET  - ATAPI
        AF_XFER_LBA     - LBA28 block addressing
        AF_XFER_PIO32   - 32-bit PIO

    NOTES

    EXAMPLE

    BUGS
        32-bit PIO is actually controller's property and not drive's property.
        Because of this AF_XFER_PIO32 flag can never be returned by this attribute.
        Nevertheless, it can be returned by aoHidd_ATAUnit_ConfiguredModes
        attribute.

    SEE ALSO
        aoHidd_ATAUnit_ConfiguredModes

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATAUnit_Removable

    SYNOPSIS
        [..G], BOOL

    LOCATION
        IID_Hidd_ATAUnit

    FUNCTION
        Tells if this drive has removable media.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATAUnit_MultiSector

    SYNOPSIS
        [..G], UBYTE

    LOCATION
        IID_Hidd_ATAUnit

    FUNCTION
        Tells maximum allowed number of sectors for multisector transfer.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*****************************************************************************************

    NAME
        aoHidd_ATAUnit_ConfiguredModes

    SYNOPSIS
        [..G], ULONG

    LOCATION
        IID_Hidd_ATAUnit

    FUNCTION
        Tells which transfer modes are currently configured for use with the drive.
        The returned value is a bitmask of the same flags as for
        aoHidd_ATAUnit_XferModes attribute.

    NOTES

    EXAMPLE

    BUGS
        Currently ata.device does not distinguish between PIO modes and does not
        set any bit for them. Absence of DMA mode flags automatically means that
        PIO mode is used.

    SEE ALSO
        aoHidd_ATAUnit_XferModes

    INTERNALS

*****************************************************************************************/

void ATAUnit__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct ataBase *ATABase = cl->UserData;
    struct ata_Unit *unit = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_ATAUnit_Switch (msg->attrID, idx)
    {
    case aoHidd_ATAUnit_Number:
        *msg->storage = unit->au_UnitNum;
        return;

    case aoHidd_ATAUnit_Model:
        *msg->storage = (IPTR)unit->au_Model;
        return;

    case aoHidd_ATAUnit_Revision:
        *msg->storage = (IPTR)unit->au_FirmwareRev;
        return;

    case aoHidd_ATAUnit_Serial:
        *msg->storage = (IPTR)unit->au_SerialNumber;
        return;

    case aoHidd_ATAUnit_XferModes:
        *msg->storage = unit->au_XferModes;
        return;

    case aoHidd_ATAUnit_Removable:
        *msg->storage = (unit->au_Flags & AF_Removable) ? TRUE : FALSE;
        return;

    case aoHidd_ATAUnit_MultiSector:
        *msg->storage = unit->au_Drive->id_RWMultipleSize & 0xFF;
        return;

    case aoHidd_ATAUnit_ConfiguredModes:
        *msg->storage = unit->au_UseModes;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}
