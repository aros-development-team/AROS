/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    BCM2835 SDHOST controller initialization.

    This driver uses the non-standard Broadcom SDHOST controller at
    peripheral offset 0x202000, instead of the Arasan SDHCI controller
    at 0x300000. The Arasan SDHCI is freed for WiFi (SDIO) use.

    GPIO pin muxing:
    - SD card signals are on GPIO 48-53
    - ALT0 = SDHOST controller
    - ALT3 = Arasan SDHCI controller
    - By default, firmware routes them to Arasan (ALT3)
    - We switch to ALT0 for SDHOST
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/mbox.h>
#include <proto/kernel.h>

#include <hardware/mmc.h>
#include <hardware/sdhc.h>

#include "sdcard_sdhost_intern.h"
#include "sdcard_bus.h"
#include "timer.h"

#include <hardware/videocore.h>

#define VCMB_PROPCHAN                   8

APTR            MBoxBase;
IPTR            __arm_periiobase __attribute__((used)) = 0;


/* GPIO Function Select: 3 bits per pin, 10 pins per register */
#define GPFSEL_ALT0     4   /* ALT0 function select value */
#define GPFSEL_BITS     3   /* Bits per pin */
#define GPFSEL_PINS     10  /* Pins per FSEL register */

/*
 * Set GPIO pin to ALT0 function (SDHOST).
 * GPIO 48-53 are used for SDHOST:
 *   GPIO 48 = SD_CLK
 *   GPIO 49 = SD_CMD
 *   GPIO 50 = SD_DATA0
 *   GPIO 51 = SD_DATA1
 *   GPIO 52 = SD_DATA2
 *   GPIO 53 = SD_DATA3
 */
static void sdhost_gpio_init(void)
{
    volatile ULONG *gpfsel4 = (volatile ULONG *)GPFSEL4;
    volatile ULONG *gpfsel5 = (volatile ULONG *)GPFSEL5;
    ULONG val;

    D(bug("[SDHost] %s: Muxing GPIO 48-53 to ALT0 (SDHOST)\n", __PRETTY_FUNCTION__));

    /* GPIO 48-49: GPFSEL4, pins 8 and 9 (bits 24-26 and 27-29) */
    val = AROS_LE2LONG(*gpfsel4);
    val &= ~((7 << 24) | (7 << 27));           /* Clear bits for GPIO 48, 49 */
    val |= (GPFSEL_ALT0 << 24) | (GPFSEL_ALT0 << 27);  /* Set ALT0 */
    *gpfsel4 = AROS_LONG2LE(val);

    /* GPIO 50-53: GPFSEL5, pins 0-3 (bits 0-2, 3-5, 6-8, 9-11) */
    val = AROS_LE2LONG(*gpfsel5);
    val &= ~((7 << 0) | (7 << 3) | (7 << 6) | (7 << 9));  /* Clear bits for GPIO 50-53 */
    val |= (GPFSEL_ALT0 << 0) | (GPFSEL_ALT0 << 3) |
            (GPFSEL_ALT0 << 6) | (GPFSEL_ALT0 << 9);       /* Set ALT0 */
    *gpfsel5 = AROS_LONG2LE(val);

    /*
     * Configure pull-ups for data/cmd lines, no pull for clock.
     * GPIO pull-up/down sequence for BCM2835/BCM2836:
     * 1. Write control signal to GPPUD
     * 2. Wait 150 cycles
     * 3. Write clock to GPPUDCLK0/1
     * 4. Wait 150 cycles
     * 5. Clear GPPUD
     * 6. Clear GPPUDCLK0/1
     */

    /* Pull-up for CMD and DATA lines (GPIO 49-53) */
    *(volatile ULONG *)GPPUD = AROS_LONG2LE(2);  /* 2 = pull-up */
    sdcard_Udelay(10);
    *(volatile ULONG *)GPPUDCLK1 = AROS_LONG2LE(
        (1 << (49 - 32)) | (1 << (50 - 32)) | (1 << (51 - 32)) |
        (1 << (52 - 32)) | (1 << (53 - 32)));
    sdcard_Udelay(10);
    *(volatile ULONG *)GPPUD = 0;
    *(volatile ULONG *)GPPUDCLK1 = 0;

    /* No pull for CLK line (GPIO 48) */
    *(volatile ULONG *)GPPUD = 0;  /* 0 = no pull */
    sdcard_Udelay(10);
    *(volatile ULONG *)GPPUDCLK1 = AROS_LONG2LE(1 << (48 - 32));
    sdcard_Udelay(10);
    *(volatile ULONG *)GPPUD = 0;
    *(volatile ULONG *)GPPUDCLK1 = 0;
}

static void FNAME_SDHOST(LEDCtrl)(int lvl)
{
    if (lvl > 0)
    {
        /* Activity LED ON */
        if (__arm_periiobase == BCM2835_PERIPHYSBASE)
            *(volatile unsigned int *)GPCLR0 = AROS_LONG2LE(1 << 16);
        else
            *(volatile unsigned int *)GPSET1 = AROS_LONG2LE(1 << (47 - 32));
    }
    else
    {
        /* Activity LED OFF */
        if (__arm_periiobase == BCM2835_PERIPHYSBASE)
            *(volatile unsigned int *)GPSET0 = AROS_LONG2LE(1 << 16);
        else
            *(volatile unsigned int *)GPCLR1 = AROS_LONG2LE(1 << (47 - 32));
    }
}

static int FNAME_SDHOST(SDHostInit)(struct SDCardBase *SDCardBase)
{
    struct sdcard_Bus       *bus;
    struct sdhost_private   *priv;
    int                     retVal = FALSE;
    unsigned int *MBoxMessage_ = AllocMem(8*4+16, MEMF_PUBLIC | MEMF_CLEAR);
    unsigned int *MBoxMessage = (unsigned int *)((((IPTR)MBoxMessage_) + 15) & ~15);

    D(bug("[SDHost] %s()\n", __PRETTY_FUNCTION__));

    __arm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    if ((MBoxBase = OpenResource("mbox.resource")) == NULL)
    {
        bug("[SDHost] %s: Failed to open mbox.resource\n", __PRETTY_FUNCTION__);
        goto sdhost_fail;
    }

    /*
     * Query core clock rate.
     * SDHOST uses the core clock (VCCLOCK_CORE = 4), not the SDHCI clock.
     */
    MBoxMessage[0] = AROS_LONG2LE(8 * 4);
    MBoxMessage[1] = AROS_LONG2LE(VCTAG_REQ);
    MBoxMessage[2] = AROS_LONG2LE(VCTAG_GETCLKRATE);
    MBoxMessage[3] = AROS_LONG2LE(8);
    MBoxMessage[4] = AROS_LONG2LE(4);
    MBoxMessage[5] = AROS_LONG2LE(VCCLOCK_CORE);
    MBoxMessage[6] = 0;
    MBoxMessage[7] = 0;

    MBoxWrite((APTR)VCMB_BASE, VCMB_PROPCHAN, MBoxMessage);
    if (MBoxRead((APTR)VCMB_BASE, VCMB_PROPCHAN) != MBoxMessage)
    {
        bug("[SDHost] %s: Failed to get core clock rate\n", __PRETTY_FUNCTION__);
        goto sdhost_fail;
    }

    D(bug("[SDHost] %s: Core clock rate: %u Hz\n", __PRETTY_FUNCTION__,
        AROS_LE2LONG(MBoxMessage[6])));

    /* Switch GPIO pins 48-53 from Arasan SDHCI to SDHOST */
    sdhost_gpio_init();

    /* Allocate bus and private data */
    bus = AllocPooled(SDCardBase->sdcard_MemPool, sizeof(struct sdcard_Bus));
    priv = AllocPooled(SDCardBase->sdcard_MemPool, sizeof(struct sdhost_private));

    if (bus && priv)
    {
        bus->sdcb_DeviceBase = SDCardBase;
        bus->sdcb_IOBase = (APTR)SDHOST_BASE;
        bus->sdcb_BusIRQ = IRQ_VC_SDHOST;

        priv->max_clk = AROS_LE2LONG(MBoxMessage[6]);
        priv->cdiv = 0;
        priv->hcfg = 0;
        priv->dma_active = FALSE;

        /* Allocate DMA control block (must be 32-byte aligned) */
        {
            ULONG cb_alloc_size = sizeof(struct SDHostDMACB) + 31;
            APTR cb_raw = AllocMem(cb_alloc_size, MEMF_PUBLIC | MEMF_CLEAR);
            if (cb_raw)
            {
                ULONG dma_enable;

                priv->dma_cb_raw = cb_raw;
                priv->dma_cb_raw_size = cb_alloc_size;
                priv->dma_cb = (struct SDHostDMACB *)(((IPTR)cb_raw + 31) & ~31);

                /* Enable DMA channel and reset it */
                dma_enable = AROS_LE2LONG(*(volatile ULONG *)DMA_ENABLE_REG);
                *(volatile ULONG *)DMA_ENABLE_REG = AROS_LONG2LE(
                    dma_enable | (1 << SDHOST_DMA_CHANNEL));
                *(volatile ULONG *)DMA_CS(SDHOST_DMA_CHANNEL) =
                    AROS_LONG2LE(DMA_CS_RESET);

                /* Allocate cache-line-aligned bounce buffer for DMA reads.
                 * CacheClearE on unaligned buffers corrupts adjacent memory. */
                {
                    /* Sized to hold one full SD_CHUNK_BLOCKS transfer
                     * (128 sectors * 512 B = 64 KB).  Caller buffers in
                     * high virtual memory must bounce through here. */
                    ULONG bounce_alloc = 65536 + 31;
                    APTR bounce_raw = AllocMem(bounce_alloc, MEMF_PUBLIC | MEMF_CLEAR);
                    if (bounce_raw)
                    {
                        priv->dma_bounce_raw = bounce_raw;
                        priv->dma_bounce = (APTR)(((IPTR)bounce_raw + 31) & ~31);
                        priv->dma_bounce_size = 65536;
                    }
                    else
                    {
                        priv->dma_bounce = NULL;
                        priv->dma_bounce_size = 0;
                    }
                }

                D(bug("[SDHost] %s: DMA channel %d enabled (bounce=%p)\n",
                    __PRETTY_FUNCTION__, SDHOST_DMA_CHANNEL, priv->dma_bounce));
            }
            else
            {
                bug("[SDHost] %s: DMA CB alloc failed — driver requires DMA\n",
                    __PRETTY_FUNCTION__);
                goto sdhost_fail;
            }
        }

        bus->sdcb_Private = (IPTR)priv;

        bus->sdcb_ClockMax = priv->max_clk;
        bus->sdcb_ClockMin = 400000;  /* 400 kHz for card identification */

        /* LED control (same as Arasan) */
        bus->sdcb_LEDCtrl = (BYTE (*)(int))FNAME_SDHOST(LEDCtrl);

        /*
         * IO function pointers for generic bus code compatibility.
         * SDHOST doesn't use these for its own operations (it uses
         * sdhost_read/sdhost_write directly), but they're needed
         * for any generic code that still calls through them.
         */
        bus->sdcb_IOReadByte = NULL;
        bus->sdcb_IOReadWord = NULL;
        bus->sdcb_IOReadLong = NULL;
        bus->sdcb_IOWriteByte = NULL;
        bus->sdcb_IOWriteWord = NULL;
        bus->sdcb_IOWriteLong = NULL;

        /* SDHOST controller-specific vtable */
        bus->sdcb_SoftReset = FNAME_SDHOSTBUS(SoftReset);
        bus->sdcb_SetClock = FNAME_SDHOSTBUS(SetClock);
        bus->sdcb_SetPowerLevel = FNAME_SDHOSTBUS(SetPowerLevel);
        bus->sdcb_SendCmd = FNAME_SDHOSTBUS(SendCmd);
        bus->sdcb_WaitCmd = FNAME_SDHOSTBUS(WaitCmd);
        bus->sdcb_FinishCmd = FNAME_SDHOSTBUS(FinishCmd);
        bus->sdcb_FinishData = FNAME_SDHOSTBUS(FinishData);
        bus->sdcb_BusIRQHandler = (void (*)(struct sdcard_Bus *, void *))FNAME_SDHOSTBUS(BusIRQ);
        bus->sdcb_SetBusWidth = FNAME_SDHOSTBUS(SetBusWidth);
        bus->sdcb_BusInit = FNAME_SDHOST(BusInit);
        bus->sdcb_BusPostIRQInit = FNAME_SDHOST(BusPostIRQInit);

        if ((bus->sdcb_BusUnits = AllocPooled(SDCardBase->sdcard_MemPool, sizeof(struct sdcard_BusUnits))) != NULL)
        {
            ObtainSemaphore(&SDCardBase->sdcard_BusSem);
            bus->sdcb_BusUnits->sdcbu_UnitBase = SDCardBase->sdcard_TotalBusUnits;
            bus->sdcb_BusUnits->sdcbu_UnitMax = 1;  /* One SD card slot */
            SDCardBase->sdcard_TotalBusUnits += 1;
            bus->sdcb_BusNum = SDCardBase->sdcard_BusCnt++;
            ReleaseSemaphore(&SDCardBase->sdcard_BusSem);

            D(bug("[SDHost] %s: Bus #%02u - 1 Unit(s) starting from %02u\n",
                __PRETTY_FUNCTION__, bus->sdcb_BusNum,
                bus->sdcb_BusUnits->sdcbu_UnitBase));

#if defined(__AROSEXEC_SMP__)
            KrnSpinInit(&bus->sdcb_Lock);
#endif
            bus->sdcb_SectorShift = 9;

            /* No capabilities register on SDHOST — hardcode */
            bus->sdcb_Version = 0;
            bus->sdcb_Capabilities = 0;
            bus->sdcb_Quirks = 0;
            bus->sdcb_Power = MMC_VDD_320_330 | MMC_VDD_330_340;

            D(bug("[SDHost] %s: Core Clock: %u Hz\n", __PRETTY_FUNCTION__, priv->max_clk));
            D(bug("[SDHost] %s: Min Clock: %u Hz\n", __PRETTY_FUNCTION__, bus->sdcb_ClockMin));

            /* Initial reset */
            FNAME_SDHOSTBUS(SoftReset)(0, bus);

            /* Register bus with sdcard.device */
            FNAME_SDC(RegisterBus)(bus, SDCardBase);

            retVal = TRUE;
        }
        else
        {
            if (priv) FreePooled(SDCardBase->sdcard_MemPool, priv, sizeof(struct sdhost_private));
            if (bus) FreePooled(SDCardBase->sdcard_MemPool, bus, sizeof(struct sdcard_Bus));
        }
    }

sdhost_fail:
    if (MBoxMessage_)
        FreeMem(MBoxMessage_, 8*4+16);

    return retVal;
}

ADD2INITLIB(FNAME_SDHOST(SDHostInit), SDCARD_BUSINITPRIO)
