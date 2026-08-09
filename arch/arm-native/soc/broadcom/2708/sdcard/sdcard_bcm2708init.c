/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
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

extern APTR     MBoxBase;
extern IPTR     __arm_periiobase;

/* Does the controller in this window report a card in the slot? */
static BOOL FNAME_BCMSDC(CardPresent)(IPTR base)
{
    return (AROS_LE2LONG(*(volatile ULONG *)(base + SDHCI_PRESENT_STATE)) &
            SDHCI_PS_CARD_PRESENT) ? TRUE : FALSE;
}

/* SDHCI-specific scan-time init: set interrupt mask, clock, power, bus width */
static void FNAME_BCMSDC(SDBusInit)(struct sdcard_Bus *bus)
{
    unsigned int sdcReg;

    bus->sdcb_IntrMask = SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
            SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_INDEX |
            SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
            SDHCI_INT_CARD_REMOVE | SDHCI_INT_CARD_INSERT |
            SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL |
            SDHCI_INT_DATA_END | SDHCI_INT_RESPONSE |
            SDHCI_INT_ADMA_ERROR;

    FNAME_SDCBUS(SetClock)(bus->sdcb_ClockMin, bus);
    FNAME_SDCBUS(SetPowerLevel)(bus->sdcb_Power, FALSE, bus);

    sdcReg = bus->sdcb_IOReadByte(SDHCI_HOST_CONTROL, bus);
    sdcReg &= ~(SDHCI_HCTRL_8BITBUS|SDHCI_HCTRL_4BITBUS|SDHCI_HCTRL_HISPD);
    bus->sdcb_IOWriteByte(SDHCI_HOST_CONTROL, sdcReg, bus);
}

/* SDHCI-specific post-IRQ init: enable interrupts, detect card */
static void FNAME_BCMSDC(SDBusPostIRQInit)(struct sdcard_Bus *bus)
{
    if (bus->sdcb_IRQHandle)
    {
        bus->sdcb_IOWriteLong(SDHCI_INT_ENABLE, bus->sdcb_IntrMask, bus);
        bus->sdcb_IOWriteLong(SDHCI_SIGNAL_ENABLE, bus->sdcb_IntrMask, bus);
    }

    /* Detect card already present at boot */
    if (bus->sdcb_IOReadLong(SDHCI_PRESENT_STATE, bus) & SDHCI_PS_CARD_PRESENT)
    {
        FNAME_SDCBUS(RegisterUnit)(bus);
    }
}

/* SDHCI-specific 4-bit bus width enable */
static void FNAME_BCMSDC(SDSetBusWidth)(UBYTE width, struct sdcard_Bus *bus)
{
    UBYTE sdcHostCtrl = bus->sdcb_IOReadByte(SDHCI_HOST_CONTROL, bus);
    if (width == 4)
        sdcHostCtrl |= SDHCI_HCTRL_4BITBUS;
    else
        sdcHostCtrl &= ~SDHCI_HCTRL_4BITBUS;
    bus->sdcb_IOWriteByte(SDHCI_HOST_CONTROL, sdcHostCtrl, bus);
}

static int FNAME_BCMSDC(BCM2708Init)(struct SDCardBase *SDCardBase)
{
    struct sdcard_Bus   *__BCM2708Bus;
    int                 retVal = FALSE;
    IPTR                ctrlBase;
    ULONG               ctrlClock, ctrlIRQ;
    BOOL                isEMMC2;
    unsigned int *MBoxMessage_ = AllocMem(8*4+16, MEMF_PUBLIC | MEMF_CLEAR);
    unsigned int *MBoxMessage = (unsigned int *)((((IPTR)MBoxMessage_) + 15) & ~15);

    DINIT(bug("[SDCard--] %s()\n", __PRETTY_FUNCTION__));

    __arm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    isEMMC2 = (__arm_periiobase == BCM2711_PERIIOBASE);
    if (isEMMC2)
    {
        /*
         * The card slot sits on EMMC2, but the legacy window is still wired
         * up and carries the card on some implementations, so fall back to it
         * when EMMC2 reports an empty slot.
         */
        if (!FNAME_BCMSDC(CardPresent)(EMMC2_BASE) &&
             FNAME_BCMSDC(CardPresent)(ARASAN_BASE))
        {
            DINIT(bug("[SDCard--] %s: card is on the legacy window\n", __PRETTY_FUNCTION__));
            isEMMC2 = FALSE;
        }
    }

    if (isEMMC2)
    {
        ctrlBase  = EMMC2_BASE;
        ctrlClock = VCCLOCK_EMMC2;
        ctrlIRQ   = IRQ_BCM2711_SDHCI;
    }
    else if (__arm_periiobase == BCM2711_PERIIOBASE)
    {
        ctrlBase  = ARASAN_BASE;
        ctrlClock = VCCLOCK_EMMC;
        ctrlIRQ   = IRQ_BCM2711_SDHCI;
    }
    else
    {
        /*
         * Earlier SoCs drive the card slot through SDHOST, which keeps this
         * controller free for SDIO. Report success so the remaining init
         * functions still run.
         */
        D(bug("[SDCard--] %s: card slot is on SDHOST for this SoC\n", __PRETTY_FUNCTION__));
        if (MBoxMessage_)
            FreeMem(MBoxMessage_, 8*4+16);
        return TRUE;
    }

    if ((MBoxBase = OpenResource("mbox.resource")) == NULL)
    {
        bug("[SDCard--] %s: Failed to open mbox.resource\n", __PRETTY_FUNCTION__);
        goto bcminit_fail;
    }

    /* EMMC2 is powered by the firmware; only the Arasan block is switchable. */
    if (isEMMC2)
        goto bcminit_clock;

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

bcminit_clock:
    MBoxMessage[0] = AROS_LONG2LE(8 * 4);
    MBoxMessage[1] = AROS_LONG2LE(VCTAG_REQ);
    MBoxMessage[2] = AROS_LONG2LE(VCTAG_GETCLKRATE);
    MBoxMessage[3] = AROS_LONG2LE(8);
    MBoxMessage[4] = AROS_LONG2LE(4);
    MBoxMessage[5] = AROS_LONG2LE(ctrlClock);
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
        __BCM2708Bus->sdcb_IOBase = (APTR)ctrlBase;
        __BCM2708Bus->sdcb_BusIRQ = ctrlIRQ;

        __BCM2708Bus->sdcb_ClockMax = AROS_LE2LONG(MBoxMessage[6]);

        /*
         * GETCLKRATE answers with the rate the clock is running at, and on
         * BCM2711 the EMMC2 clock is parked until something asks for it, so
         * the answer is zero and the card never gets clocked. Ask what the
         * clock can do instead.
         */
        if (__BCM2708Bus->sdcb_ClockMax == 0)
        {
            MBoxMessage[0] = AROS_LONG2LE(8 * 4);
            MBoxMessage[1] = AROS_LONG2LE(VCTAG_REQ);
            MBoxMessage[2] = AROS_LONG2LE(VCTAG_GETMAXCLKRATE);
            MBoxMessage[3] = AROS_LONG2LE(8);
            MBoxMessage[4] = AROS_LONG2LE(4);
            MBoxMessage[5] = AROS_LONG2LE(ctrlClock);
            MBoxMessage[6] = 0;
            MBoxMessage[7] = 0;

            MBoxWrite((APTR)VCMB_BASE, VCMB_PROPCHAN, MBoxMessage);
            if (MBoxRead((APTR)VCMB_BASE, VCMB_PROPCHAN) == MBoxMessage)
                __BCM2708Bus->sdcb_ClockMax = AROS_LE2LONG(MBoxMessage[6]);

            DINIT(bug("[SDCard--] %s: clock was parked, max rate %d Hz\n",
                      __PRETTY_FUNCTION__, __BCM2708Bus->sdcb_ClockMax));
        }
        __BCM2708Bus->sdcb_ClockMin = BCM2708SDCLOCK_MIN;

        __BCM2708Bus->sdcb_LEDCtrl = (BYTE (*)(int))FNAME_BCMSDCBUS(BCMLEDCtrl);
        __BCM2708Bus->sdcb_IOReadByte = FNAME_BCMSDCBUS(BCMMMIOReadByte);
        __BCM2708Bus->sdcb_IOReadWord = FNAME_BCMSDCBUS(BCMMMIOReadWord);
        __BCM2708Bus->sdcb_IOReadLong = FNAME_BCMSDCBUS(BCMMMIOReadLong);

        if (isEMMC2)
        {
            /* Only the earlier controller needs a delay between writes. */
            __BCM2708Bus->sdcb_IOWriteByte = FNAME_BCMSDCBUS(BCMMMIODirectWriteByte);
            __BCM2708Bus->sdcb_IOWriteWord = FNAME_BCMSDCBUS(BCMMMIODirectWriteWord);
            __BCM2708Bus->sdcb_IOWriteLong = FNAME_BCMSDCBUS(BCMMMIODirectWriteLong);
        }
        else
        {
            __BCM2708Bus->sdcb_IOWriteByte = FNAME_BCMSDCBUS(BCMMMIOWriteByte);
            __BCM2708Bus->sdcb_IOWriteWord = FNAME_BCMSDCBUS(BCMMMIOWriteWord);
            __BCM2708Bus->sdcb_IOWriteLong = FNAME_BCMSDCBUS(BCMMMIOWriteLong);
        }

        /* SDHCI controller-specific vtable */
        __BCM2708Bus->sdcb_SoftReset = FNAME_SDCBUS(SoftReset);
        __BCM2708Bus->sdcb_SetClock = FNAME_SDCBUS(SetClock);
        __BCM2708Bus->sdcb_SetPowerLevel = FNAME_SDCBUS(SetPowerLevel);
        __BCM2708Bus->sdcb_SendCmd = FNAME_SDCBUS(SendCmd);
        __BCM2708Bus->sdcb_WaitCmd = FNAME_SDCBUS(WaitCmd);
        __BCM2708Bus->sdcb_FinishCmd = FNAME_SDCBUS(FinishCmd);
        __BCM2708Bus->sdcb_FinishData = FNAME_SDCBUS(FinishData);
        __BCM2708Bus->sdcb_BusIRQHandler = (void (*)(struct sdcard_Bus *, void *))FNAME_SDCBUS(BusIRQ);
        __BCM2708Bus->sdcb_SetBusWidth = FNAME_BCMSDC(SDSetBusWidth);
        __BCM2708Bus->sdcb_BusInit = FNAME_BCMSDC(SDBusInit);
        __BCM2708Bus->sdcb_BusPostIRQInit = FNAME_BCMSDC(SDBusPostIRQInit);

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

    #if defined(__AROSEXEC_SMP__)
            KrnSpinInit(&__BCM2708Bus->sdcb_Lock);
#endif
            __BCM2708Bus->sdcb_SectorShift = 9;

            DINIT(bug("[SDCard--] %s: Reseting SDHCI...\n", __PRETTY_FUNCTION__));

            FNAME_SDCBUS(SoftReset)(SDHCI_RESET_ALL, __BCM2708Bus);

            __BCM2708Bus->sdcb_Version = FNAME_BCMSDCBUS(BCMMMIOReadWord)(SDHCI_HOST_VERSION, __BCM2708Bus);
            __BCM2708Bus->sdcb_Capabilities = FNAME_BCMSDCBUS(BCMMMIOReadLong)(SDHCI_CAPABILITIES, __BCM2708Bus);
            __BCM2708Bus->sdcb_Quirks = AB_Quirk_MissingCapabilities|AF_Quirk_AtomicTMAndCMD;
            __BCM2708Bus->sdcb_Power = MMC_VDD_165_195 | MMC_VDD_320_330 | MMC_VDD_330_340;

            DINIT(bug("[SDCard--] %s: SDHCI Host Vers      : %d [SD Host Spec %d]\n", __PRETTY_FUNCTION__, ((__BCM2708Bus->sdcb_Version & 0xFF00) >> 8), (__BCM2708Bus->sdcb_Version & 0xFF) + 1));
            DINIT(bug("[SDCard--] %s: SDHCI Capabilities   : 0x%08x\n", __PRETTY_FUNCTION__, __BCM2708Bus->sdcb_Capabilities));
            DINIT(bug("[SDCard--] %s: SDHCI Voltages       : 0x%08x (hardcoded)\n", __PRETTY_FUNCTION__, __BCM2708Bus->sdcb_Power));

            /*
             * The mailbox reports what the clock could be turned up to, which
             * is not what the divider divides. The controller knows its own
             * base clock, so believe that instead: on the BCM2711 the mailbox
             * says 500MHz while the block actually runs off 100MHz, and using
             * the wrong one clocks the card five times too slowly.
             */
            {
                ULONG sdcClockBase;

                if ((__BCM2708Bus->sdcb_Version & SDHCI_HVERS_SPEC_MASK) >= 2)
                    sdcClockBase = (__BCM2708Bus->sdcb_Capabilities & SDHCI_CLOCK_V3_BASE_MASK) >> SDHCI_CLOCK_BASE_SHIFT;
                else
                    sdcClockBase = (__BCM2708Bus->sdcb_Capabilities & SDHCI_CLOCK_BASE_MASK) >> SDHCI_CLOCK_BASE_SHIFT;

                if (sdcClockBase)
                    __BCM2708Bus->sdcb_ClockMax = sdcClockBase * 1000000;
                else
                    DINIT(bug("[SDCard--] %s: controller reports no base clock, keeping the mailbox rate\n", __PRETTY_FUNCTION__));
            }

            DINIT(bug("[SDCard--] %s: SDHC Base Clock Rate : %dMHz\n", __PRETTY_FUNCTION__, __BCM2708Bus->sdcb_ClockMax / 1000000));
            DINIT(bug("[SDCard--] %s: SDHC Min Clock Rate : %dHz (hardcoded)\n", __PRETTY_FUNCTION__, __BCM2708Bus->sdcb_ClockMin));

            FNAME_SDCBUS(ADMAAlloc)(__BCM2708Bus);

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
