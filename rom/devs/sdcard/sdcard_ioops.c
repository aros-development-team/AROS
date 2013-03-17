/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <oop/oop.h>

#include <proto/exec.h>
#include <devices/cd.h>
#include <devices/timer.h>

#include <hardware/mmc.h>
#include <hardware/sdhc.h>

#include "sdcard_base.h"
#include "sdcard_unit.h"

static BOOL sdcard_WaitBusyTO(struct sdcard_Unit *unit, UWORD tout, BOOL irq, UBYTE *stout);

#define DEVHEAD_VAL 0x40

static void sdcard_strcpy(const UBYTE *str1, UBYTE *str2, ULONG size)
{
    register int i = size;

    while (size--)
        str2[size ^ 1] = str1[size];

    while (i > 0 && str2[--i] <= ' ')
        str2[i] = '\0';
}

/*
 * a STUB function for commands not supported by this particular device
 */
static BYTE sdcard_STUB(struct sdcard_Unit *unit)
{
    DERROR(bug("[SDCard%02ld] CALLED STUB FUNCTION (GENERIC). OPERATION IS NOT SUPPORTED BY DEVICE\n", unit->sdcu_UnitNum));
    return CDERR_NOCMD;
}

static BYTE sdcard_STUB_IO32(struct sdcard_Unit *unit, ULONG blk, ULONG len,
    APTR buf, ULONG* act)
{
    DERROR(bug("[SDCard%02ld] CALLED STUB FUNCTION (IO32). OPERATION IS NOT SUPPORTED BY DEVICE\n", unit->sdcu_UnitNum));
    DERROR(bug("[SDCard%02ld] -- IO ACCESS TO BLOCK %08lx, LENGTH %08lx\n", unit->sdcu_UnitNum, blk, len));
    return CDERR_NOCMD;
}

static BYTE sdcard_STUB_IO64(struct sdcard_Unit *unit, UQUAD blk, ULONG len,
    APTR buf, ULONG* act)
{
    DERROR(bug("[SDCard%02ld] CALLED STUB FUNCTION (IO64). OPERATION IS NOT SUPPORTED BY DEVICE\n", unit->sdcu_UnitNum));
    DERROR(bug("[SDCard%02ld] -- IO ACCESS TO BLOCK %08lx:%08lx, LENGTH %08lx\n", unit->sdcu_UnitNum, (blk >> 32), (blk & 0xffffffff), len));
    return CDERR_NOCMD;
}

static BYTE sdcard_STUB_SCSI(struct sdcard_Unit *unit, struct SCSICmd* cmd)
{
    DERROR(bug("[SDCard%02ld] CALLED STUB FUNCTION (IOSCSI). OPERATION IS NOT SUPPORTED BY DEVICE\n", unit->sdcu_UnitNum));
    return CDERR_NOCMD;
}

static inline BOOL sdcard_SelectUnit(struct sdcard_Unit* unit)
{
    return TRUE;
}

void sdcard_IRQSetHandler(struct sdcard_Unit *unit, void (*handler)(struct sdcard_Unit*, UBYTE), APTR piomem, ULONG blklen, ULONG piolen)
{

}

void sdcard_IRQNoData(struct sdcard_Unit *unit, UBYTE status)
{

}

void sdcard_IRQPIORead(struct sdcard_Unit *unit, UBYTE status)
{

}

void sdcard_PIOWriteBlk(struct sdcard_Unit *unit)
{

}

void sdcard_IRQPIOWrite(struct sdcard_Unit *unit, UBYTE status)
{

}

void sdcard_IRQDMAReadWrite(struct sdcard_Unit *unit, UBYTE status)
{

}

/*
 * wait for timeout or drive ready
 */
BOOL sdcard_WaitBusyTO(struct sdcard_Unit *unit, UWORD tout, BOOL irq, UBYTE *stout)
{
    BYTE err = 0;
    return err;
}

/*
 * 32bit Read operations
 */
BYTE FNAME_SDCIO(ReadSector32)(struct sdcard_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    struct TagItem sdcReadTags[] =
    {
        {SDCARD_TAG_CMD,         MMC_CMD_READ_SINGLE_BLOCK},
        {SDCARD_TAG_ARG,         block},
        {SDCARD_TAG_RSPTYPE,     MMC_RSP_R1},
        {SDCARD_TAG_RSP,         0},
        {SDCARD_TAG_DATA,        (IPTR)buffer},
        {SDCARD_TAG_DATALEN,     count * (1 << unit->sdcu_Bus->sdcb_SectorShift)},
        {SDCARD_TAG_DATAFLAGS,   MMC_DATA_READ},
        {TAG_DONE,            0}
    };

    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    *act = 0;

    if (count > 1)
        sdcReadTags[0].ti_Data = MMC_CMD_READ_MULTIPLE_BLOCK;

    if (!(unit->sdcu_Flags & AF_Card_HighCapacity))
        sdcReadTags[1].ti_Data <<= unit->sdcu_Bus->sdcb_SectorShift;

    D(bug("[SDCard%02ld] %s: Sending CMD %d, block 0x%p, len %d [buffer @ 0x%p]\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__,
        sdcReadTags[0].ti_Data, sdcReadTags[1].ti_Data, sdcReadTags[5].ti_Data, sdcReadTags[4].ti_Data));

    if (FNAME_SDCBUS(SendCmd)(sdcReadTags, unit->sdcu_Bus) != -1)
    {
        if (FNAME_SDCBUS(WaitCmd)(SDHCI_INT_DATA_AVAIL|SDHCI_INT_DATA_END, 1000, unit->sdcu_Bus) != -1)
        {
            if (count > 1)
            {
                DTRANS(bug("[SDCard%02ld] %s: Finishing transaction ..\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));
                sdcReadTags[0].ti_Data = MMC_CMD_STOP_TRANSMISSION;
                sdcReadTags[1].ti_Data = 0;
                sdcReadTags[2].ti_Data = MMC_RSP_R1b;
                sdcReadTags[4].ti_Tag = TAG_DONE;
                if (FNAME_SDCBUS(SendCmd)(sdcReadTags, unit->sdcu_Bus) == -1)
                {
                    bug("[SDCard%02ld] %s: Failed to terminate Read operation\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__);
                }
            }
        }
        else
        {
            D(bug("[SDCard%02ld] %s: Transfer error\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));
            count = 0;
        }
    }
    else
    {
        bug("[SDCard%02ld] %s: Error ..\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__);
        count = 0;
    }

    *act = count << unit->sdcu_Bus->sdcb_SectorShift;

    DTRANS(bug("[SDCard%02ld] %s: %d bytes Read\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__, *act));

    return 0;
}

BYTE FNAME_SDCIO(ReadMultiple32)(struct sdcard_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{

    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    return 0;
}

BYTE FNAME_SDCIO(ReadDMA32)(struct sdcard_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    return 0;
}

/*
 * 64bit Read operations
 */
BYTE FNAME_SDCIO(ReadSector64)(struct sdcard_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    return 0;
}

BYTE FNAME_SDCIO(ReadMultiple64)(struct sdcard_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    return 0;
}

BYTE FNAME_SDCIO(ReadDMA64)(struct sdcard_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    return 0;
}

/*
 * 32bit Write operations
 */
BYTE FNAME_SDCIO(WriteSector32)(struct sdcard_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    return 0;
}

BYTE FNAME_SDCIO(WriteMultiple32)(struct sdcard_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    return 0;
}

BYTE FNAME_SDCIO(WriteDMA32)(struct sdcard_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    return 0;
}

/*
 * 64bit Write operations
 */
BYTE FNAME_SDCIO(WriteSector64)(struct sdcard_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    return 0;
}

BYTE FNAME_SDCIO(WriteMultiple64)(struct sdcard_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    return 0;
}

BYTE FNAME_SDCIO(WriteDMA64)(struct sdcard_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    return 0;
}

/*
 * miscellaneous
 */
BYTE FNAME_SDCIO(Eject)(struct sdcard_Unit *unit)
{
    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    return 0;
}
