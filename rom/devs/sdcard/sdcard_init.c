/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/bootloader.h>
#include <aros/symbolsets.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <utility/utility.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/bootloader.h>
#include <proto/expansion.h>
#include <proto/kernel.h>

#include <hardware/mmc.h>
#include <hardware/sdhc.h>

#include <string.h>

#include "sdcard_base.h"
#include "sdcard_bus.h"
#include "sdcard_unit.h"
#include "timer.h"

#include LC_LIBDEFS_FILE

BOOL FNAME_SDC(RegisterBus)(struct sdcard_Bus *bus, LIBBASETYPEPTR LIBBASE)
{
    DINIT(bug("[SDCard--] %s(0x%p)\n", __PRETTY_FUNCTION__, bus));

    AddTail(&LIBBASE->sdcard_Buses, (struct Node *)bus);
    return TRUE;
}

/*
 * Libinit functions -:
 * 0:   FNAME_SDC(CommonInit) -> common libbase init.
 * 10:  FNAME_SDC(XXXInit) -> chipset/implementation specific bus init.
 * 120: FNAME_SDC(Scan) -> Scan registered buses for units.
 */
static int FNAME_SDC(Scan)(LIBBASETYPEPTR LIBBASE)
{
    struct sdcard_Bus   *busCurrent;
    unsigned int        sdcReg;

    DINIT(bug("[SDCard--] %s()\n", __PRETTY_FUNCTION__));

    ForeachNode(&LIBBASE->sdcard_Buses, busCurrent)
    {
        if (busCurrent->sdcb_LEDCtrl)
            busCurrent->sdcb_LEDCtrl(LED_OFF);

        busCurrent->sdcb_IntrMask = SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
                SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_INDEX |
                SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
                SDHCI_INT_CARD_REMOVE | SDHCI_INT_CARD_INSERT |
                SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL |
                SDHCI_INT_DATA_END | SDHCI_INT_RESPONSE;

        FNAME_SDCBUS(SetClock)(busCurrent->sdcb_ClockMin, busCurrent);

        FNAME_SDCBUS(SetPowerLevel)(busCurrent->sdcb_Power, FALSE, busCurrent);

        sdcReg = busCurrent->sdcb_IOReadByte(SDHCI_HOST_CONTROL, busCurrent);
        DINIT(bug("[SDCard--] %s: Setting Min Buswidth... [%x -> %x]\n", __PRETTY_FUNCTION__, sdcReg, sdcReg & ~(SDHCI_HCTRL_8BITBUS|SDHCI_HCTRL_4BITBUS|SDHCI_HCTRL_HISPD)));
        sdcReg &= ~(SDHCI_HCTRL_8BITBUS|SDHCI_HCTRL_4BITBUS|SDHCI_HCTRL_HISPD);
        busCurrent->sdcb_IOWriteByte(SDHCI_HOST_CONTROL, sdcReg, busCurrent);

        /* Install IRQ handler */
        if ((busCurrent->sdcb_IRQHandle = KrnAddIRQHandler(busCurrent->sdcb_BusIRQ, FNAME_SDCBUS(BusIRQ), busCurrent, NULL)) != NULL)
        {
            DINIT(bug("[SDCard--] %s: IRQHandle @ 0x%p for IRQ#%ld\n", __PRETTY_FUNCTION__, busCurrent->sdcb_IRQHandle, busCurrent->sdcb_BusIRQ));

            DINIT(bug("[SDCard--] %s: Masking chipset Interrupts...\n", __PRETTY_FUNCTION__));

            busCurrent->sdcb_IOWriteLong(SDHCI_INT_ENABLE, busCurrent->sdcb_IntrMask, busCurrent);
            busCurrent->sdcb_IOWriteLong(SDHCI_SIGNAL_ENABLE, busCurrent->sdcb_IntrMask, busCurrent);

            DINIT(bug("[SDCard--] %s: Launching Bus Task...\n", __PRETTY_FUNCTION__));

            NewCreateTask(
                TASKTAG_PC         , FNAME_SDCBUS(BusTask),
                TASKTAG_NAME       , "SDCard Subsystem",
                TASKTAG_STACKSIZE  , SDCARD_BUSTASKSTACK,
                TASKTAG_PRI        , SDCARD_BUSTASKPRI,
                TASKTAG_TASKMSGPORT, &busCurrent->sdcb_MsgPort,
                TASKTAG_ARG1       , busCurrent,
                TAG_DONE);
        }
    }

    return TRUE;
}

static int FNAME_SDC(CommonInit)(LIBBASETYPEPTR LIBBASE)
{
    DINIT(bug("[SDCard--] %s()\n", __PRETTY_FUNCTION__));

    if ((ExpansionBase = OpenLibrary("expansion.library", 40L)) == NULL)
    {
        bug("[SDCard--] %s: Failed to open expansion.library\n", __PRETTY_FUNCTION__);
        goto libinit_fail;
    }

    if ((KernelBase = OpenResource("kernel.resource")) == NULL)
    {
        bug("[SDCard--] %s: Failed to open kernel.resource\n", __PRETTY_FUNCTION__);
        goto libinit_fail;
    }

    if ((LIBBASE->sdcard_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED , 8192, 4096)) == NULL)
    {
        bug("[SDCard--] %s: Failed to Allocate MemPool\n", __PRETTY_FUNCTION__);
        goto libinit_fail;
    }

    DINIT(bug("[SDCard--] %s: MemPool @ %p\n", __PRETTY_FUNCTION__, LIBBASE->sdcard_MemPool));

    InitSemaphore(&LIBBASE->sdcard_BusSem);
    NEWLIST(&LIBBASE->sdcard_Buses);

    return TRUE;

libinit_fail:
    if (ExpansionBase) CloseLibrary(ExpansionBase);

    return FALSE;
}

static int FNAME_SDC(Open)
(
    LIBBASETYPEPTR LIBBASE,
    struct IORequest *iorq,
    ULONG unitnum,
    ULONG flags
)
{
    struct sdcard_Bus   *busCurrent;

    DDEV(bug("[SDCard--] %s()\n", __PRETTY_FUNCTION__));

    /* Assume it failed */
    iorq->io_Error = IOERR_OPENFAIL;

    ForeachNode(&LIBBASE->sdcard_Buses, busCurrent)
    {
        if ((unitnum < (busCurrent->sdcb_BusUnits->sdcbu_UnitBase + busCurrent->sdcb_BusUnits->sdcbu_UnitCnt)) && ((&busCurrent->sdcb_BusUnits->sdcbu_Units)[unitnum] != NULL))
        {
            iorq->io_Unit = (&busCurrent->sdcb_BusUnits->sdcbu_Units)[unitnum];
            ((struct sdcard_Unit *)iorq->io_Unit)->sdcu_Unit.unit_OpenCnt++;

            iorq->io_Error = 0;

            if (!(((struct sdcard_Unit *)iorq->io_Unit)->sdcu_Flags & AF_Card_Active))
            {
                if (FNAME_SDCBUS(StartUnit)((struct sdcard_Unit *)iorq->io_Unit))
                {
                    DDEV(bug("[SDCard%02ld] %s: Unit @ 0x%p configured for operation\n", ((struct sdcard_Unit *)iorq->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__, iorq->io_Unit));
                    ((struct sdcard_Unit *)iorq->io_Unit)->sdcu_Flags |= AF_Card_Active;
                }
                else
                {
                    DDEV(bug("[SDCard%02ld] %s: Failed to configure unit\n", ((struct sdcard_Unit *)iorq->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__));
                }
            }
        }
    }
    return iorq->io_Error ? FALSE : TRUE;
}

/* Close given device */
static int FNAME_SDC(Close)
(
    LIBBASETYPEPTR LIBBASE,
    struct IORequest *iorq
)
{
    struct sdcard_Unit *unit = (struct sdcard_Unit *)iorq->io_Unit;

    DDEV(bug("[SDCard--] %s()\n", __PRETTY_FUNCTION__));

    /* First of all make the important fields of struct IORequest invalid! */
    iorq->io_Unit = (struct Unit *)~0;
    
    /* Decrease use counters of unit */
    unit->sdcu_Unit.unit_OpenCnt--;

    return TRUE;
}

ADD2INITLIB(FNAME_SDC(CommonInit), 0)
ADD2INITLIB(FNAME_SDC(Scan), 127)
ADD2OPENDEV(FNAME_SDC(Open), 0)
ADD2CLOSEDEV(FNAME_SDC(Close), 0)
