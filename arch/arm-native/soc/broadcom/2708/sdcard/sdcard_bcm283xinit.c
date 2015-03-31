/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/vcmbox.h>
#include <proto/kernel.h>

#include <hardware/mmc.h>
#include <hardware/sdhc.h>

#include "sdcard_intern.h"
#include "timer.h"

#include <hardware/arasan.h>
#include <hardware/videocore.h>

APTR            VCMBoxBase;
unsigned int    VCMBoxMessage[8] __attribute__((used, aligned(16)));
IPTR		__arm_periiobase __attribute__((used)) = 0 ;

static int FNAME_BCMSDC(BCM283xInit)(struct SDCardBase *SDCardBase)
{
    struct sdcard_Bus   *__BCM283xBus;
    int                 retVal = FALSE;

    DINIT(bug("[SDCard--] %s()\n", __PRETTY_FUNCTION__));

    __arm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    if ((VCMBoxBase = OpenResource("vcmbox.resource")) == NULL)
    {
        bug("[SDCard--] %s: Failed to open vcmbox.resource\n", __PRETTY_FUNCTION__);
        goto bcminit_fail;
    }

    VCMBoxMessage[0] = 8 * 4;
    VCMBoxMessage[1] = VCTAG_REQ;
    VCMBoxMessage[2] = VCTAG_GETPOWER;
    VCMBoxMessage[3] = 8;
    VCMBoxMessage[4] = 4;
    VCMBoxMessage[5] = VCPOWER_SDHCI;
    VCMBoxMessage[6] = 0;

    VCMBoxMessage[7] = 0; // terminate tag

    VCMBoxWrite((APTR)VCMB_BASE, VCMB_PROPCHAN, VCMBoxMessage);
    if (VCMBoxRead((APTR)VCMB_BASE, VCMB_PROPCHAN) != VCMBoxMessage)
    {
        DINIT(bug("[SDCard--] %s: Failed to read controller's Power state\n", __PRETTY_FUNCTION__));
        goto bcminit_fail;
    }
    
    if (!(VCMBoxMessage[6] & VCPOWER_STATE_ON))
    {
        DINIT(bug("[SDCard--] %s: Powering on Arasan SDHCI controller...\n", __PRETTY_FUNCTION__));

        VCMBoxMessage[0] = 8 * 4;
        VCMBoxMessage[1] = VCTAG_REQ;
        VCMBoxMessage[2] = VCTAG_SETPOWER;
        VCMBoxMessage[3] = 8;
        VCMBoxMessage[4] = 8;
        VCMBoxMessage[5] = VCPOWER_SDHCI;
        VCMBoxMessage[6] = VCPOWER_STATE_ON | VCPOWER_STATE_WAIT;

        VCMBoxMessage[7] = 0; // terminate tag

        VCMBoxWrite((APTR)VCMB_BASE, VCMB_PROPCHAN, VCMBoxMessage);
        if ((VCMBoxRead((APTR)VCMB_BASE, VCMB_PROPCHAN) != VCMBoxMessage) || (!(VCMBoxMessage[6] & VCPOWER_STATE_ON)))
        {
            DINIT(bug("[SDCard--] %s: Failed to power on controller\n", __PRETTY_FUNCTION__));
            goto bcminit_fail;
        }
    }

    VCMBoxMessage[0] = 8 * 4;
    VCMBoxMessage[1] = VCTAG_REQ;
    VCMBoxMessage[2] = VCTAG_GETCLKRATE;
    VCMBoxMessage[3] = 8;
    VCMBoxMessage[4] = 4;
    VCMBoxMessage[5] = VCCLOCK_SDHCI;
    VCMBoxMessage[6] = 0;

    VCMBoxMessage[7] = 0; // terminate tag

    VCMBoxWrite((APTR)VCMB_BASE, VCMB_PROPCHAN, VCMBoxMessage);
    if (VCMBoxRead((APTR)VCMB_BASE, VCMB_PROPCHAN) != VCMBoxMessage)
    {
        DINIT(bug("[SDCard--] %s: Failed to determine Max SDHC Clock\n", __PRETTY_FUNCTION__));
        goto bcminit_fail;
    }

    if ((__BCM283xBus = AllocPooled(SDCardBase->sdcard_MemPool, sizeof(struct sdcard_Bus))) != NULL)
    {
        __BCM283xBus->sdcb_DeviceBase = SDCardBase;
        __BCM283xBus->sdcb_IOBase = (APTR)ARASAN_BASE;
        __BCM283xBus->sdcb_BusIRQ = IRQ_VC_ARASANSDIO;

        __BCM283xBus->sdcb_ClockMax = VCMBoxMessage[6];
        __BCM283xBus->sdcb_ClockMin = BCM283xSDCLOCK_MIN;        

        __BCM283xBus->sdcb_LEDCtrl = FNAME_BCMSDCBUS(BCMLEDCtrl);
        __BCM283xBus->sdcb_IOReadByte = FNAME_BCMSDCBUS(BCMMMIOReadByte);
        __BCM283xBus->sdcb_IOReadWord = FNAME_BCMSDCBUS(BCMMMIOReadWord);
        __BCM283xBus->sdcb_IOReadLong = FNAME_BCMSDCBUS(BCMMMIOReadLong);

        __BCM283xBus->sdcb_IOWriteByte = FNAME_BCMSDCBUS(BCMMMIOWriteByte);
        __BCM283xBus->sdcb_IOWriteWord = FNAME_BCMSDCBUS(BCMMMIOWriteWord);
        __BCM283xBus->sdcb_IOWriteLong = FNAME_BCMSDCBUS(BCMMMIOWriteLong);

        if ((__BCM283xBus->sdcb_BusUnits = AllocPooled(SDCardBase->sdcard_MemPool, sizeof(struct sdcard_BusUnits))) != NULL)
        {
            ObtainSemaphore(&SDCardBase->sdcard_BusSem);
            __BCM283xBus->sdcb_BusUnits->sdcbu_UnitBase = SDCardBase->sdcard_TotalBusUnits;
            __BCM283xBus->sdcb_BusUnits->sdcbu_UnitMax = BCM283xSDUNIT_MAX;
            SDCardBase->sdcard_TotalBusUnits += __BCM283xBus->sdcb_BusUnits->sdcbu_UnitMax;
            __BCM283xBus->sdcb_BusNum = SDCardBase->sdcard_BusCnt++;
            ReleaseSemaphore(&SDCardBase->sdcard_BusSem);

            DINIT(bug("[SDCard--] %s: Bus #%02u - %u Unit(s) starting from %02u\n", __PRETTY_FUNCTION__,
                            __BCM283xBus->sdcb_BusNum,
                            __BCM283xBus->sdcb_BusUnits->sdcbu_UnitMax,
                            __BCM283xBus->sdcb_BusUnits->sdcbu_UnitBase));

            __BCM283xBus->sdcb_SectorShift = 9;

            DINIT(bug("[SDCard--] %s: Reseting SDHCI...\n", __PRETTY_FUNCTION__));

            FNAME_SDCBUS(SoftReset)(SDHCI_RESET_ALL, __BCM283xBus);

            DINIT(bug("[SDCard--] %s: SDHC Max Clock Rate : %dMHz\n", __PRETTY_FUNCTION__, __BCM283xBus->sdcb_ClockMax / 1000000));
            DINIT(bug("[SDCard--] %s: SDHC Min Clock Rate : %dHz (hardcoded)\n", __PRETTY_FUNCTION__, __BCM283xBus->sdcb_ClockMin));

            __BCM283xBus->sdcb_Version = FNAME_BCMSDCBUS(BCMMMIOReadWord)(SDHCI_HOST_VERSION, __BCM283xBus);
            __BCM283xBus->sdcb_Capabilities = FNAME_BCMSDCBUS(BCMMMIOReadLong)(SDHCI_CAPABILITIES, __BCM283xBus);
            __BCM283xBus->sdcb_Quirks = AB_Quirk_MissingCapabilities|AF_Quirk_AtomicTMAndCMD;
            __BCM283xBus->sdcb_Power = MMC_VDD_165_195 | MMC_VDD_320_330 | MMC_VDD_330_340;

            DINIT(bug("[SDCard--] %s: SDHCI Host Vers      : %d [SD Host Spec %d]\n", __PRETTY_FUNCTION__, ((__BCM283xBus->sdcb_Version & 0xFF00) >> 8), (__BCM283xBus->sdcb_Version & 0xFF) + 1));
            DINIT(bug("[SDCard--] %s: SDHCI Capabilities   : 0x%08x\n", __PRETTY_FUNCTION__, __BCM283xBus->sdcb_Capabilities));
            DINIT(bug("[SDCard--] %s: SDHCI Voltages       : 0x%08x (hardcoded)\n", __PRETTY_FUNCTION__, __BCM283xBus->sdcb_Power));

            __BCM283xBus->sdcb_Private = (IPTR)sdcard_CurrentTime();

            FNAME_SDC(RegisterBus)(__BCM283xBus, SDCardBase);
            
            retVal = TRUE;
        }
        else
        {
            FreePooled(SDCardBase->sdcard_MemPool, __BCM283xBus, sizeof(struct sdcard_Bus));
        }
    }
bcminit_fail:

    return retVal;
}

ADD2INITLIB(FNAME_BCMSDC(BCM283xInit), SDCARD_BUSINITPRIO)
