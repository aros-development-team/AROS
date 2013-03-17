/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/vcmbox.h>

#include <asm/bcm2835.h>
#include <hardware/arasan.h>
#include <hardware/videocore.h>

#include <hardware/mmc.h>
#include <hardware/sdhc.h>

#include "sdcard_intern.h"
#include "timer.h"

#define SDHCI_READONLY

#define VCPOWER_SDHCI		0
#define VCPOWER_STATE_ON	(1 << 0)
#define VCPOWER_STATE_WAIT	(1 << 1)
#define VCCLOCK_SDHCI           1

APTR                VCMBoxBase;
unsigned int        VCMBoxMessage[8] __attribute__((used, aligned(16)));

static int FNAME_SDC(BCM2835Init)(struct SDCardBase *SDCardBase)
{
    struct sdcard_Bus   *__BCM2835Bus;
    int                 retVal = FALSE;

    DINIT(bug("[SDCard--] %s()\n", __PRETTY_FUNCTION__));

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

    if ((__BCM2835Bus = AllocPooled(SDCardBase->sdcard_MemPool, sizeof(struct sdcard_Bus))) != NULL)
    {
        __BCM2835Bus->sdcb_DeviceBase = SDCardBase;
        __BCM2835Bus->sdcb_IOBase = (APTR)ARASAN_BASE;
        __BCM2835Bus->sdcb_BusIRQ = IRQ_VC_ARASANSDIO;

        __BCM2835Bus->sdcb_ClockMax = VCMBoxMessage[6];
        __BCM2835Bus->sdcb_ClockMin = BCM2835SDCLOCK_MIN;        

        __BCM2835Bus->sdcb_LEDCtrl = FNAME_SDCBUS(BCMLEDCtrl);
        __BCM2835Bus->sdcb_IOReadByte = FNAME_SDCBUS(BCMMMIOReadByte);
        __BCM2835Bus->sdcb_IOReadWord = FNAME_SDCBUS(BCMMMIOReadWord);
        __BCM2835Bus->sdcb_IOReadLong = FNAME_SDCBUS(BCMMMIOReadLong);

        __BCM2835Bus->sdcb_IOWriteByte = FNAME_SDCBUS(BCMMMIOWriteByte);
        __BCM2835Bus->sdcb_IOWriteWord = FNAME_SDCBUS(BCMMMIOWriteWord);
        __BCM2835Bus->sdcb_IOWriteLong = FNAME_SDCBUS(BCMMMIOWriteLong);

        if ((__BCM2835Bus->sdcb_BusUnits = AllocPooled(SDCardBase->sdcard_MemPool, sizeof(struct sdcard_BusUnits))) != NULL)
        {
            ObtainSemaphore(&SDCardBase->sdcard_BusSem);
            __BCM2835Bus->sdcb_BusUnits->sdcbu_UnitBase = SDCardBase->sdcard_TotalBusUnits;
            __BCM2835Bus->sdcb_BusUnits->sdcbu_UnitMax = BCM2835SDUNIT_MAX;
            SDCardBase->sdcard_TotalBusUnits += __BCM2835Bus->sdcb_BusUnits->sdcbu_UnitMax;
            ReleaseSemaphore(&SDCardBase->sdcard_BusSem);

            DINIT(bug("[SDCard--] %s: Bus Unit Range %02ld -> %02ld\n", __PRETTY_FUNCTION__,
                            __BCM2835Bus->sdcb_BusUnits->sdcbu_UnitBase,
                            __BCM2835Bus->sdcb_BusUnits->sdcbu_UnitBase + __BCM2835Bus->sdcb_BusUnits->sdcbu_UnitMax));

            __BCM2835Bus->sdcb_SectorShift = 9;

            DINIT(bug("[SDCard--] %s: Reseting SDHCI...\n", __PRETTY_FUNCTION__));

            FNAME_SDCBUS(SoftReset)(SDHCI_RESET_ALL, __BCM2835Bus);

            DINIT(bug("[SDCard--] %s: SDHC Max Clock Rate : %dMHz\n", __PRETTY_FUNCTION__, __BCM2835Bus->sdcb_ClockMax / 1000000));
            DINIT(bug("[SDCard--] %s: SDHC Min Clock Rate : %dHz (hardcoded)\n", __PRETTY_FUNCTION__, __BCM2835Bus->sdcb_ClockMin));

            __BCM2835Bus->sdcb_Version = FNAME_SDCBUS(BCMMMIOReadWord)(SDHCI_HOST_VERSION, __BCM2835Bus);
            __BCM2835Bus->sdcb_Capabilities = FNAME_SDCBUS(BCMMMIOReadLong)(SDHCI_CAPABILITIES, __BCM2835Bus);
            __BCM2835Bus->sdcb_Quirks = AB_Quirk_MissingCapabilities;
            __BCM2835Bus->sdcb_Power = MMC_VDD_165_195 | MMC_VDD_320_330 | MMC_VDD_330_340;

            DINIT(bug("[SDCard--] %s: SDHCI Host Vers      : %d [SD Host Spec %d]\n", __PRETTY_FUNCTION__, ((__BCM2835Bus->sdcb_Version & 0xFF00) >> 8), (__BCM2835Bus->sdcb_Version & 0xFF) + 1));
            DINIT(bug("[SDCard--] %s: SDHCI Capabilities   : 0x%08x\n", __PRETTY_FUNCTION__, __BCM2835Bus->sdcb_Capabilities));
            DINIT(bug("[SDCard--] %s: SDHCI Voltages       : 0x%08x (hardcoded)\n", __PRETTY_FUNCTION__, __BCM2835Bus->sdcb_Power));

            __BCM2835Bus->sdcb_Private = (IPTR)sdcard_CurrentTime();

            FNAME_SDC(RegisterBus)(__BCM2835Bus, SDCardBase);
            
            retVal = TRUE;
        }
        else
        {
            FreePooled(SDCardBase->sdcard_MemPool, __BCM2835Bus, sizeof(struct sdcard_Bus));
        }
    }
bcminit_fail:

    return retVal;
}

ADD2INITLIB(FNAME_SDC(BCM2835Init), SDCARD_BUSINITPRIO)
