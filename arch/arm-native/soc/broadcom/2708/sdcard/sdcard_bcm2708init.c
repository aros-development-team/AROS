/*
    Copyright � 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/mbox.h>
#include <proto/kernel.h>

#include <hardware/mmc.h>
#include <hardware/sdhc.h>

#include "sdcard_intern.h"
#include "timer.h"

#include <hardware/arasan.h>
#include <hardware/videocore.h>

APTR            MBoxBase;
IPTR		__arm_periiobase __attribute__((used)) = 0 ;

static int FNAME_BCMSDC(BCM2708Init)(struct SDCardBase *SDCardBase)
{
    struct sdcard_Bus   *__BCM2708Bus;
    int                 retVal = FALSE;
    unsigned int *MBoxMessage_ = AllocMem(8*4+16, MEMF_PUBLIC | MEMF_CLEAR);
    unsigned int *MBoxMessage = (unsigned int *)((((IPTR)MBoxMessage_) + 15) & ~15);

    DINIT(bug("[SDCard--] %s()\n", __PRETTY_FUNCTION__));

    __arm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    if ((MBoxBase = OpenResource("mbox.resource")) == NULL)
    {
        bug("[SDCard--] %s: Failed to open mbox.resource\n", __PRETTY_FUNCTION__);
        goto bcminit_fail;
    }

    MBoxMessage[0] = AROS_LONG2LE(8 * 4);
    MBoxMessage[1] = AROS_LONG2LE(VCTAG_REQ);
    MBoxMessage[2] = AROS_LONG2LE(VCTAG_GETPOWER);
    MBoxMessage[3] = AROS_LONG2LE(8);
    MBoxMessage[4] = AROS_LONG2LE(4);
    MBoxMessage[5] = AROS_LONG2LE(VCPOWER_SDHCI);
    MBoxMessage[6] = 0;

    MBoxMessage[7] = 0; // terminate tag

    MBoxWrite((APTR)VCMB_BASE, VCMB_PROPCHAN, MBoxMessage);
    if (MBoxRead((APTR)VCMB_BASE, VCMB_PROPCHAN) != MBoxMessage)
    {
        DINIT(bug("[SDCard--] %s: Failed to read controller's Power state\n", __PRETTY_FUNCTION__));
        goto bcminit_fail;
    }

    if (!(AROS_LE2LONG(MBoxMessage[6]) & VCPOWER_STATE_ON))
    {
        DINIT(bug("[SDCard--] %s: Powering on Arasan SDHCI controller...\n", __PRETTY_FUNCTION__));

        MBoxMessage[0] = AROS_LONG2LE(8 * 4);
        MBoxMessage[1] = AROS_LONG2LE(VCTAG_REQ);
        MBoxMessage[2] = AROS_LONG2LE(VCTAG_SETPOWER);
        MBoxMessage[3] = AROS_LONG2LE(8);
        MBoxMessage[4] = AROS_LONG2LE(8);
        MBoxMessage[5] = AROS_LONG2LE(VCPOWER_SDHCI);
        MBoxMessage[6] = AROS_LONG2LE(VCPOWER_STATE_ON | VCPOWER_STATE_WAIT);

        MBoxMessage[7] = 0; // terminate tag

        MBoxWrite((APTR)VCMB_BASE, VCMB_PROPCHAN, MBoxMessage);
        if ((MBoxRead((APTR)VCMB_BASE, VCMB_PROPCHAN) != MBoxMessage) || (!(AROS_LE2LONG(MBoxMessage[6]) & VCPOWER_STATE_ON)))
        {
            DINIT(bug("[SDCard--] %s: Failed to power on controller\n", __PRETTY_FUNCTION__));
            goto bcminit_fail;
        }
    }

    MBoxMessage[0] = AROS_LONG2LE(8 * 4);
    MBoxMessage[1] = AROS_LONG2LE(VCTAG_REQ);
    MBoxMessage[2] = AROS_LONG2LE(VCTAG_GETCLKRATE);
    MBoxMessage[3] = AROS_LONG2LE(8);
    MBoxMessage[4] = AROS_LONG2LE(4);
    MBoxMessage[5] = AROS_LONG2LE(VCCLOCK_SDHCI);
    MBoxMessage[6] = 0;

    MBoxMessage[7] = 0; // terminate tag

    MBoxWrite((APTR)VCMB_BASE, VCMB_PROPCHAN, MBoxMessage);
    if (MBoxRead((APTR)VCMB_BASE, VCMB_PROPCHAN) != MBoxMessage)
    {
        DINIT(bug("[SDCard--] %s: Failed to determine Max SDHC Clock\n", __PRETTY_FUNCTION__));
        goto bcminit_fail;
    }

    if ((__BCM2708Bus = AllocPooled(SDCardBase->sdcard_MemPool, sizeof(struct sdcard_Bus))) != NULL)
    {
        __BCM2708Bus->sdcb_DeviceBase = SDCardBase;
        __BCM2708Bus->sdcb_IOBase = (APTR)ARASAN_BASE;
        __BCM2708Bus->sdcb_BusIRQ = IRQ_VC_ARASANSDIO;

        __BCM2708Bus->sdcb_ClockMax = AROS_LE2LONG(MBoxMessage[6]);
        __BCM2708Bus->sdcb_ClockMin = BCM2708SDCLOCK_MIN;

        __BCM2708Bus->sdcb_LEDCtrl = (BYTE (*)(int))FNAME_BCMSDCBUS(BCMLEDCtrl);
        __BCM2708Bus->sdcb_IOReadByte = FNAME_BCMSDCBUS(BCMMMIOReadByte);
        __BCM2708Bus->sdcb_IOReadWord = FNAME_BCMSDCBUS(BCMMMIOReadWord);
        __BCM2708Bus->sdcb_IOReadLong = FNAME_BCMSDCBUS(BCMMMIOReadLong);

        __BCM2708Bus->sdcb_IOWriteByte = FNAME_BCMSDCBUS(BCMMMIOWriteByte);
        __BCM2708Bus->sdcb_IOWriteWord = FNAME_BCMSDCBUS(BCMMMIOWriteWord);
        __BCM2708Bus->sdcb_IOWriteLong = FNAME_BCMSDCBUS(BCMMMIOWriteLong);

        if ((__BCM2708Bus->sdcb_BusUnits = AllocPooled(SDCardBase->sdcard_MemPool, sizeof(struct sdcard_BusUnits))) != NULL)
        {
            ObtainSemaphore(&SDCardBase->sdcard_BusSem);
            __BCM2708Bus->sdcb_BusUnits->sdcbu_UnitBase = SDCardBase->sdcard_TotalBusUnits;
            __BCM2708Bus->sdcb_BusUnits->sdcbu_UnitMax = BCM2708SDUNIT_MAX;
            SDCardBase->sdcard_TotalBusUnits += __BCM2708Bus->sdcb_BusUnits->sdcbu_UnitMax;
            __BCM2708Bus->sdcb_BusNum = SDCardBase->sdcard_BusCnt++;
            ReleaseSemaphore(&SDCardBase->sdcard_BusSem);

            DINIT(bug("[SDCard--] %s: Bus #%02u - %u Unit(s) starting from %02u\n", __PRETTY_FUNCTION__,
                            __BCM2708Bus->sdcb_BusNum,
                            __BCM2708Bus->sdcb_BusUnits->sdcbu_UnitMax,
                            __BCM2708Bus->sdcb_BusUnits->sdcbu_UnitBase));

            __BCM2708Bus->sdcb_SectorShift = 9;

            DINIT(bug("[SDCard--] %s: Reseting SDHCI...\n", __PRETTY_FUNCTION__));

            FNAME_SDCBUS(SoftReset)(SDHCI_RESET_ALL, __BCM2708Bus);

            DINIT(bug("[SDCard--] %s: SDHC Max Clock Rate : %dMHz\n", __PRETTY_FUNCTION__, __BCM2708Bus->sdcb_ClockMax / 1000000));
            DINIT(bug("[SDCard--] %s: SDHC Min Clock Rate : %dHz (hardcoded)\n", __PRETTY_FUNCTION__, __BCM2708Bus->sdcb_ClockMin));

            __BCM2708Bus->sdcb_Version = FNAME_BCMSDCBUS(BCMMMIOReadWord)(SDHCI_HOST_VERSION, __BCM2708Bus);
            __BCM2708Bus->sdcb_Capabilities = FNAME_BCMSDCBUS(BCMMMIOReadLong)(SDHCI_CAPABILITIES, __BCM2708Bus);
            __BCM2708Bus->sdcb_Quirks = AB_Quirk_MissingCapabilities|AF_Quirk_AtomicTMAndCMD;
            __BCM2708Bus->sdcb_Power = MMC_VDD_165_195 | MMC_VDD_320_330 | MMC_VDD_330_340;

            DINIT(bug("[SDCard--] %s: SDHCI Host Vers      : %d [SD Host Spec %d]\n", __PRETTY_FUNCTION__, ((__BCM2708Bus->sdcb_Version & 0xFF00) >> 8), (__BCM2708Bus->sdcb_Version & 0xFF) + 1));
            DINIT(bug("[SDCard--] %s: SDHCI Capabilities   : 0x%08x\n", __PRETTY_FUNCTION__, __BCM2708Bus->sdcb_Capabilities));
            DINIT(bug("[SDCard--] %s: SDHCI Voltages       : 0x%08x (hardcoded)\n", __PRETTY_FUNCTION__, __BCM2708Bus->sdcb_Power));

            __BCM2708Bus->sdcb_Private = (IPTR)sdcard_CurrentTime();

            FNAME_SDC(RegisterBus)(__BCM2708Bus, SDCardBase);

            retVal = TRUE;
        }
        else
        {
            FreePooled(SDCardBase->sdcard_MemPool, __BCM2708Bus, sizeof(struct sdcard_Bus));
        }
    }
bcminit_fail:

    if (MBoxMessage_)
        FreeMem(MBoxMessage_, 8*4+16);

    return retVal;
}

ADD2INITLIB(FNAME_BCMSDC(BCM2708Init), SDCARD_BUSINITPRIO)
