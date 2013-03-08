/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
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
#include <proto/utility.h>
#include <proto/vcmbox.h>

#include <asm/bcm2835.h>
#include <hardware/videocore.h>
#include <hardware/mmc.h>
#include <hardware/sdhc.h>

#include <string.h>

#include "sdcard_intern.h"
#include "timer.h"

#include LC_LIBDEFS_FILE

UBYTE FNAME_SDCBUS(MMIOReadByte)(ULONG reg, struct sdcard_Bus *bus)
{
    ULONG val = *(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3);

    return (val >> ((reg & 3) << 3)) & 0xff;
}

UWORD FNAME_SDCBUS(MMIOReadWord)(ULONG reg, struct sdcard_Bus *bus)
{
    ULONG val = *(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3);

    return (val >> (((reg >> 1) & 1) << 4)) & 0xffff;
}

ULONG FNAME_SDCBUS(MMIOReadLong)(ULONG reg, struct sdcard_Bus *bus)
{
    return *(volatile ULONG *)(bus->sdcb_IOBase + reg);
}

void FNAME_SDCBUS(ArasanWriteLong)(ULONG reg, ULONG val, struct sdcard_Bus *bus)
{
    /* Bug: two SDC clock cycle delay required between successive chipset writes */
    while (*((volatile ULONG *)(SYSTIMER_CLO)) < (bus->sdcb_LastWrite + 6))
        asm volatile("mov r0, r0\n");

    *(volatile ULONG *)(bus->sdcb_IOBase + reg) = val;
    bus->sdcb_LastWrite = *((volatile ULONG *)(SYSTIMER_CLO));
}

void FNAME_SDCBUS(MMIOWriteByte)(ULONG reg, UBYTE val, struct sdcard_Bus *bus)
{
    ULONG currval = *(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3);
    ULONG shift = (reg & 3) * 8;
    ULONG mask = 0xff << shift;
    ULONG newval = (currval & ~mask) | (val << shift);

    FNAME_SDCBUS(ArasanWriteLong)(reg & ~3, newval, bus);
}

void FNAME_SDCBUS(MMIOWriteWord)(ULONG reg, UWORD val, struct sdcard_Bus *bus)
{
    ULONG currval = *(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3);
    ULONG shift = ((reg >> 1) & 1) * 16;
    ULONG mask = 0xffff << shift;
    ULONG newval = (currval & ~mask) | (val << shift);

    FNAME_SDCBUS(ArasanWriteLong)(reg & ~3, newval, bus);
}

void FNAME_SDCBUS(MMIOWriteLong)(ULONG reg, ULONG val, struct sdcard_Bus *bus)
{
    FNAME_SDCBUS(ArasanWriteLong)(reg, val, bus);
}

void FNAME_SDCBUS(SoftReset)(UBYTE mask, struct sdcard_Bus *bus)
{
    ULONG timeout = 10000, timeout_udelay;

    FNAME_SDCBUS(MMIOWriteByte)(SDHCI_RESET, mask, bus);
    while (FNAME_SDCBUS(MMIOReadByte)(SDHCI_RESET, bus) & mask) {
        if (timeout == 0) {
            D(bug("[SDCard--] %s: Timeout\n", __PRETTY_FUNCTION__));
            break;
        }
        timeout_udelay = timeout - 1000;
        for (; timeout > timeout_udelay; timeout --) asm volatile("mov r0, r0\n");
    }
}

void FNAME_SDCBUS(SetClock)(ULONG speed, struct sdcard_Bus *bus)
{
    ULONG       sdcClkDiv, timeout, timeout_udelay;
    UWORD       sdcClkCtrlCur, sdcClkCtrl;

    sdcClkCtrlCur = FNAME_SDCBUS(MMIOReadWord)(SDHCI_CLOCK_CONTROL, bus);

    for (sdcClkDiv = 2; sdcClkDiv < V300_MAXCLKDIV; sdcClkDiv += 2) {
        if ((bus->sdcb_ClockMax / sdcClkDiv) <= speed)
                break;
    }
    sdcClkDiv >>= 1;

    sdcClkCtrl = (sdcClkDiv & SDHCI_DIV_MASK) << SDHCI_DIVIDER_SHIFT;
    sdcClkCtrl |= ((sdcClkDiv & SDHCI_DIV_HI_MASK) >> SDHCI_DIV_MASK_LEN) << SDHCI_DIVIDER_HI_SHIFT;

    if (sdcClkCtrl != (sdcClkCtrlCur & ~(SDHCI_CLOCK_INT_EN|SDHCI_CLOCK_INT_STABLE|SDHCI_CLOCK_CARD_EN)))
    {
        FNAME_SDCBUS(MMIOWriteWord)(SDHCI_CLOCK_CONTROL, 0, bus);

        D(bug("[SDCard--] %s: CLOCK_CONTROL (current) = 0x%04x\n", __PRETTY_FUNCTION__, sdcClkCtrlCur & ~(SDHCI_CLOCK_INT_EN|SDHCI_CLOCK_INT_STABLE|SDHCI_CLOCK_CARD_EN)));
        D(bug("[SDCard--] %s: CLOCK_CONTROL (new)     = 0x%04x (div %d)\n", __PRETTY_FUNCTION__, sdcClkCtrl, sdcClkDiv));

        FNAME_SDCBUS(MMIOWriteWord)(SDHCI_CLOCK_CONTROL, (sdcClkCtrl | SDHCI_CLOCK_INT_EN), bus);

        timeout = 20000;
        while (!((sdcClkCtrl = FNAME_SDCBUS(MMIOReadWord)(SDHCI_CLOCK_CONTROL, bus)) & SDHCI_CLOCK_INT_STABLE)) {
            if (timeout == 0) {
                D(bug("[SDCard--] %s: SDHCI Clock failed to stabilise\n", __PRETTY_FUNCTION__));
                break;
            }
            timeout_udelay = timeout - 1000;
            for (; timeout > timeout_udelay; timeout --) asm volatile("mov r0, r0\n");
        }
    }
    else
        if (sdcClkCtrlCur & SDHCI_CLOCK_CARD_EN)
            return;

    D(bug("[SDCard--] %s: Enabling clock...\n", __PRETTY_FUNCTION__));
    sdcClkCtrl |= SDHCI_CLOCK_CARD_EN;
    FNAME_SDCBUS(MMIOWriteWord)(SDHCI_CLOCK_CONTROL, sdcClkCtrl, bus);
}

void FNAME_SDCBUS(SetPowerLevel)(ULONG supportedlvls, struct sdcard_Bus *bus)
{
    UBYTE sdcReg, lvlCur;

    if (supportedlvls & (MMC_VDD_320_330|MMC_VDD_330_340))
        sdcReg = SDHCI_POWER_330;
    else if (supportedlvls & (MMC_VDD_290_300|MMC_VDD_300_310))
        sdcReg = SDHCI_POWER_300;
    else if (supportedlvls & MMC_VDD_165_195)
        sdcReg = SDHCI_POWER_180;
    else
        sdcReg = 0;

    lvlCur = FNAME_SDCBUS(MMIOReadByte)(SDHCI_POWER_CONTROL, bus);
    if ((lvlCur & ~SDHCI_POWER_ON) != sdcReg)
    {
        D(bug("[SDCard--] %s: Changing Power Lvl (0x%x)\n", __PRETTY_FUNCTION__, sdcReg));
        FNAME_SDCBUS(MMIOWriteByte)(SDHCI_POWER_CONTROL, sdcReg, bus);
        sdcReg |= SDHCI_POWER_ON;
        FNAME_SDCBUS(MMIOWriteByte)(SDHCI_POWER_CONTROL, sdcReg, bus);
    }
    else
    {
        if (lvlCur && (!(lvlCur & SDHCI_POWER_ON)))
        {
            D(bug("[SDCard--] %s: Enabling Power Lvl (0x%x)\n", __PRETTY_FUNCTION__, lvlCur));
            lvlCur |= SDHCI_POWER_ON;
            FNAME_SDCBUS(MMIOWriteByte)(SDHCI_POWER_CONTROL, lvlCur, bus);
        }
    }
}

ULONG FNAME_SDCBUS(SendCmd)(struct TagItem *CmdTags, struct sdcard_Bus *bus)
{
    struct TagItem *Response = NULL;

    UWORD sdCommand      = (UWORD)GetTagData(SDCARD_TAG_CMD, 0, CmdTags);
    ULONG sdArg          = GetTagData(SDCARD_TAG_ARG, 0, CmdTags);
    ULONG sdResponseType = GetTagData(SDCARD_TAG_RSPTYPE, MMC_RSP_NONE, CmdTags);
    ULONG sdData, sdDataLen = 0, sdDataFlags;

    UWORD sdcTransMode = 0, sdCommandFlags;
    ULONG sdStatus = 0, sdCommandMask = SDHCI_PS_CMD_INHIBIT;
    ULONG timeout = 10000, timeout_udelay;
    ULONG ret = 0;

    if (sdResponseType != MMC_RSP_NONE)
    {
        Response = FindTagItem(SDCARD_TAG_RSP, CmdTags);
    }

    if ((sdData = GetTagData(SDCARD_TAG_DATA, 0, CmdTags)) != 0)
    {
        sdDataLen = GetTagData(SDCARD_TAG_DATALEN, 0, CmdTags);
        sdDataFlags = GetTagData(SDCARD_TAG_DATAFLAGS, 0, CmdTags);
        if (!(sdDataLen))
            sdData = 0;
    };

    /* Dont wait for DATA inihibit for stop commands */
    if (sdCommand != MMC_CMD_STOP_TRANSMISSION)
        sdCommandMask |= SDHCI_PS_DATA_INHIBIT;

    while (FNAME_SDCBUS(MMIOReadLong)(SDHCI_PRESENT_STATE, bus) & sdCommandMask) {
        if (timeout == 0) {
            D(bug("[SDCard--] %s: Controller failed to release inhibited bit(s).\n", __PRETTY_FUNCTION__));
            return -1;
        }
        timeout_udelay = timeout - 1000;
        for (; timeout > timeout_udelay; timeout--)
            asm volatile("mov r0, r0\n");
    }

    sdCommandMask = SDHCI_INT_RESPONSE;
    if (!(sdResponseType & MMC_RSP_PRESENT))
        sdCommandFlags = SDHCI_CMD_RESP_NONE;
    else if (sdResponseType & MMC_RSP_136)
        sdCommandFlags = SDHCI_CMD_RESP_LONG;
    else if (sdResponseType & MMC_RSP_BUSY) {
        sdCommandFlags = SDHCI_CMD_RESP_SHORT_BUSY;
        sdCommandMask |= SDHCI_INT_DATA_END;
    } else
        sdCommandFlags = SDHCI_CMD_RESP_SHORT;

    if (sdResponseType & MMC_RSP_CRC)
        sdCommandFlags |= SDHCI_CMD_CRC;
    if (sdResponseType & MMC_RSP_OPCODE)
        sdCommandFlags |= SDHCI_CMD_INDEX;
    if (sdData)
        sdCommandFlags |= SDHCI_CMD_DATA;

    if (sdData != 0) {
        sdcTransMode = SDHCI_TRANSMOD_BLK_CNT_EN;
        D(bug("[SDCard--] %s: Configuring Data Transfer\n", __PRETTY_FUNCTION__));

        *(volatile ULONG *)GPCLR0 = 1<<16; // Turn Activity LED ON

        FNAME_SDCBUS(MMIOWriteByte)(SDHCI_TIMEOUT_CONTROL, SDHCI_TIMEOUT_MAX, bus);

        FNAME_SDCBUS(MMIOWriteWord)(SDHCI_BLOCK_SIZE, SDHCI_MAKE_BLCKSIZE(7, ((sdDataLen > (1 << bus->sdcb_SectorShift)) ? (1 << bus->sdcb_SectorShift) : sdDataLen)), bus);
        if ((sdDataLen >> bus->sdcb_SectorShift) > 1)
        {
            sdcTransMode |= SDHCI_TRANSMOD_MULTI;
            FNAME_SDCBUS(MMIOWriteWord)(SDHCI_BLOCK_COUNT, sdDataLen >> bus->sdcb_SectorShift, bus);
        }
        else
        {
            FNAME_SDCBUS(MMIOWriteWord)(SDHCI_BLOCK_COUNT, 1, bus);
        }

        if (sdDataFlags == MMC_DATA_READ)
            sdcTransMode |= SDHCI_TRANSMOD_READ;

        D(bug("[SDCard--] %s: Mode %08x, BlockSize %d, Count %d\n", __PRETTY_FUNCTION__, sdcTransMode, ((sdDataLen > (1 << bus->sdcb_SectorShift)) ? (1 << bus->sdcb_SectorShift) : sdDataLen), (((sdDataLen >> bus->sdcb_SectorShift) > 0) ? (sdDataLen >> bus->sdcb_SectorShift) : 1)));
    }

    FNAME_SDCBUS(MMIOWriteLong)(SDHCI_ARGUMENT, sdArg, bus);
    if (sdcTransMode)
        FNAME_SDCBUS(MMIOWriteLong)(SDHCI_TRANSFER_MODE, (sdcTransMode << 16) | SDHCI_MAKE_CMD(sdCommand, sdCommandFlags), bus);
    else
        FNAME_SDCBUS(MMIOWriteWord)(SDHCI_COMMAND, SDHCI_MAKE_CMD(sdCommand, sdCommandFlags), bus);

    timeout = 10000;
    do {
        sdStatus = FNAME_SDCBUS(MMIOReadLong)(SDHCI_INT_STATUS, bus);
        if (sdStatus & SDHCI_INT_ERROR)
            break;
        if (--timeout == 0)
            break;
    } while ((sdStatus & sdCommandMask) != sdCommandMask);

    if (timeout != 0) {
        if ((sdStatus & (sdCommandMask|SDHCI_INT_ERROR)) == sdCommandMask) {
            if (sdStatus & SDHCI_INT_ERROR)
            {
                D(bug("[SDCard--] %s: Error?", __PRETTY_FUNCTION__));
            }

            if (Response)
            {
                D(bug("[SDCard--] %s: Reading Response ", __PRETTY_FUNCTION__));
                if (sdResponseType & MMC_RSP_136)
                {
                    D(bug("[136bit]\n"));
                    if (Response->ti_Data)
                    {
                        ULONG i;
                        for (i = 0; i < 4; i ++)
                        {
                            ((ULONG *)Response->ti_Data)[i] = FNAME_SDCBUS(MMIOReadLong)(SDHCI_RESPONSE + (3 - i) * 4, bus) << 8;
                            if (i != 3)
                                ((ULONG *)Response->ti_Data)[i] |= FNAME_SDCBUS(MMIOReadByte)(SDHCI_RESPONSE + (3 - i) * 4 - 1, bus);
                        }
                    }
                }
                else
                {
                    D(bug("\n"));
                    Response->ti_Data = FNAME_SDCBUS(MMIOReadLong)(SDHCI_RESPONSE, bus);
                }
            }
            FNAME_SDCBUS(MMIOWriteLong)(SDHCI_INT_STATUS, sdCommandMask, bus);
        }
        else
        {
            D(bug("[SDCard--] %s: Failed? [   status = %08x]\n", __PRETTY_FUNCTION__, sdStatus));
            if (sdStatus & SDHCI_INT_ACMD12ERR)
            {
                D(bug("[SDCard--] %s:         [acmd12err = %04x    ]\n", __PRETTY_FUNCTION__, FNAME_SDCBUS(MMIOReadWord)(SDHCI_ACMD12_ERR, bus)));
            }
            ret = -1;
        }

        if (!ret && sdData)
        {
            D(bug("[SDCard--] %s: Transfering Data..\n", __PRETTY_FUNCTION__));
            timeout = 1000000;
            sdCommand = SDHCI_INT_SPACE_AVAIL | SDHCI_INT_DATA_AVAIL;
            sdCommandMask = SDHCI_PS_DATA_AVAILABLE | SDHCI_PS_SPACE_AVAILABLE;
            do {
                sdStatus = FNAME_SDCBUS(MMIOReadLong)(SDHCI_INT_STATUS, bus);
                if (sdStatus & SDHCI_INT_ERROR) {
                    D(bug("[SDCard--] %s:    Error [status 0x%X]!\n", __PRETTY_FUNCTION__, sdStatus));
                    ret = -1;
                    break;
                }
                if (sdStatus & sdCommand) {
                    if (!(FNAME_SDCBUS(MMIOReadLong)(SDHCI_PRESENT_STATE, bus) & sdCommandMask))
                        continue;
                    FNAME_SDCBUS(MMIOWriteLong)(SDHCI_INT_STATUS, sdCommand, bus);
#warning "TODO: Transfer bytes ;)"
                }
                if (timeout-- <= 0)
                {
                    D(bug("[SDCard--] %s:    Timeout!\n", __PRETTY_FUNCTION__));
                    ret = -1;
                    break;
                }
            } while (!(sdStatus & SDHCI_INT_DATA_END));
        }
    }
    else
    {
        D(bug("[SDCard--] %s: Error - command timed out [status = %08x]\n", __PRETTY_FUNCTION__, sdStatus));
        ret = -1;
    }
    timeout = 1000;
    timeout_udelay = 0;
    for (; timeout > timeout_udelay; timeout --) asm volatile("mov r0, r0\n");

    *(volatile ULONG *)GPSET0 = (1 << 16); // Turn Activity LED OFF

    sdStatus = FNAME_SDCBUS(MMIOReadLong)(SDHCI_INT_STATUS, bus);
    FNAME_SDCBUS(MMIOWriteLong)(SDHCI_INT_STATUS, SDHCI_INT_ALL_MASK, bus);
    if (!ret) {
        return 0;
    }

    D(bug("[SDCard--] %s: Reseting SDHCI CMD/DATA\n", __PRETTY_FUNCTION__));

    FNAME_SDCBUS(SoftReset)(SDHCI_RESET_CMD, bus);
    FNAME_SDCBUS(SoftReset)(SDHCI_RESET_DATA, bus);
    if (sdStatus & SDHCI_INT_TIMEOUT)
        return -1;
    else
        return -1;
}

ULONG FNAME_SDCBUS(Rsp136Unpack)(ULONG *buf, ULONG offset, const ULONG len)
{
    const ULONG mask = ((len < 32) ? (1 << len) : 0) - 1;
    const ULONG shift = (offset) & 0x1F;
    ULONG retval;

    retval = buf[3 - (offset >> 5)] >> shift;
    if (len + shift > 32)
        retval |= buf[3 - (offset >> 5) - 1] << ((32 - shift) % 32);

    return (retval & mask);
}


int FNAME_SDCBUS(SDUnitSwitch)(BOOL test, int group, UBYTE value, APTR buf, struct sdcard_Unit *sdcUnit)
{
    struct TagItem sdcSwitchTags[] =
    {
        {SDCARD_TAG_CMD,        SD_CMD_SWITCH_FUNC},
        {SDCARD_TAG_ARG,        0},
        {SDCARD_TAG_RSPTYPE,    MMC_RSP_R1},
        {SDCARD_TAG_RSP,        0},
        {SDCARD_TAG_DATA,       buf},
        {SDCARD_TAG_DATALEN,    64},
        {SDCARD_TAG_DATAFLAGS,  MMC_DATA_READ},
        {TAG_DONE,              0}
    };

    D(bug("[SDCard%02ld] %s()\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    sdcSwitchTags[1].ti_Data = ((test) ? 0 : (1 << 31)) | 0xffffff;
    sdcSwitchTags[1].ti_Data &= ~(0xf << (group * 4));
    sdcSwitchTags[1].ti_Data |= value << (group * 4);

    return FNAME_SDCBUS(SendCmd)(sdcSwitchTags, sdcUnit->sdcu_Bus);
}


int FNAME_SDCBUS(SDUnitChangeFrequency)(struct sdcard_Unit *sdcUnit)
{
    unsigned int        timeout;
    struct TagItem sdcChFreqTags[] =
    {
        {SDCARD_TAG_CMD,        0},
        {SDCARD_TAG_ARG,        0},
        {SDCARD_TAG_RSPTYPE,    0},
        {SDCARD_TAG_RSP,        0},
        {TAG_DONE,              0}, /* SDCARD_TAG_DATA */
        {SDCARD_TAG_DATALEN,    0},
        {SDCARD_TAG_DATAFLAGS,  MMC_DATA_READ},
        {TAG_DONE,              0}
    };

    /* Read the SCR to find out if higher speeds are supported ..*/

    D(bug("[SDCard%02ld] %s: Attempt to send App Command ... \n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
    sdcChFreqTags[0].ti_Data = MMC_CMD_APP_CMD;
    sdcChFreqTags[1].ti_Data = sdcUnit->sdcu_CardRCA << 16;
    sdcChFreqTags[2].ti_Data = MMC_RSP_R1;
    if (FNAME_SDCBUS(SendCmd)(sdcChFreqTags, sdcUnit->sdcu_Bus) != -1)
    {
        ULONG   sdcardRespBuf[2] = {0, 0};

        sdcChFreqTags[0].ti_Data = SD_CMD_APP_SEND_SCR;
        sdcChFreqTags[1].ti_Data = 0;
        sdcChFreqTags[2].ti_Data = MMC_RSP_R1;

        timeout = 3;
        D(bug("[SDCard%02ld] %s: Attempt to Query SCR Register ... \n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
        do
        {
            sdcChFreqTags[4].ti_Tag = SDCARD_TAG_DATA;
            sdcChFreqTags[4].ti_Data = sdcardRespBuf;
            sdcChFreqTags[5].ti_Data = 8;

            if (FNAME_SDCBUS(SendCmd)(sdcChFreqTags, sdcUnit->sdcu_Bus) != -1)
                break;
        } while (timeout-- > 0);

        if (timeout > 0)
        {
            if (AROS_BE2LONG(sdcardRespBuf[0]) & SD_SCR_DATA4BIT)
                sdcUnit->sdcu_Flags |= AF_4bitData;

            /* v1.0 SDCards don't support switching */
            if (((AROS_BE2LONG(sdcardRespBuf[0]) >> 24) & 0xf) < 1)
            {
                D(bug("[SDCard%02ld] %s: Card doesnt support Switching\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
                return 0;
            }

            timeout = 4;
            while (timeout-- > 0) {
                if (FNAME_SDCBUS(SDUnitSwitch)(TRUE, 0, 1, sdcardRespBuf, sdcUnit) != -1)
                {
                    /* The high-speed function is busy.  Try again */
                    if (!(AROS_BE2LONG(sdcardRespBuf[7]) & SD_SCR_HIGHSPEED))
                        break;
                }
                else
                {
                    D(bug("[SDCard%02ld] %s: Switch failed\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
                    return -1;
                }
            }

            if (timeout > 0)
            {
                /* Is high-speed supported? */
                if (!(AROS_BE2LONG(sdcardRespBuf[3]) & SD_SCR_HIGHSPEED))
                {
                    D(bug("[SDCard%02ld] %s: Card doesnt support Highspeed mode\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
                    return 0;
                }

                if (FNAME_SDCBUS(SDUnitSwitch)(FALSE, 0, 1, sdcardRespBuf, sdcUnit) != -1)
                {
                    if ((AROS_BE2LONG(sdcardRespBuf[4]) & 0x0F000000) == 0x01000000)
                       sdcUnit->sdcu_Flags |= AF_HighSpeed;
                }
            }
        }
        else
        {
            D(bug("[SDCard%02ld] %s: Query Failed\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
        }
    }
    else
    {
        D(bug("[SDCard%02ld] %s: App Command Failed\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
    }
}

void FNAME_SDCBUS(BusTask)(struct sdcard_Bus *bus)
{
    struct SDCardBase *SDCardBase = bus->sdcb_DeviceBase;
    struct sdcard_Unit *unit;
    struct IORequest *msg;
    ULONG sdcReg;
    ULONG sig;

    D(bug("[SDCard**] Task started (bus: %u)\n", bus->sdcb_BusNum));

    bus->sdcb_Timer = sdcard_OpenTimer(SDCardBase);

    /* Get the signal used for sleeping */
    bus->sdcb_Task = FindTask(0);
    bus->sdcb_TaskSig = AllocSignal(-1);
    /* Failed to get it? Use SIGBREAKB_CTRL_E instead */
    if (bus->sdcb_TaskSig < 0)
        bus->sdcb_TaskSig = SIGBREAKB_CTRL_E;

    sig = 1L << bus->sdcb_MsgPort->mp_SigBit;

    sdcReg = FNAME_SDCBUS(MMIOReadLong)(SDHCI_PRESENT_STATE, bus);
    if (sdcReg & SDHCI_PS_CARD_PRESENT)
    {
        FNAME_SDC(RegisterVolume)(bus);
    }

    /* Wait forever and process messages */
    for (;;)
    {
        Wait(sig);

        /* Even if you get new signal, do not process it until Unit is not active */
        if (!(bus->sdcb_BusFlags & UNITF_ACTIVE))
        {
            bus->sdcb_BusFlags |= UNITF_ACTIVE;

            /* Empty the request queue */
            while ((msg = (struct IORequest *)GetMsg(bus->sdcb_MsgPort)))
            {
                /* And do IO's */
                if (FNAME_SDC(HandleIO)(msg))
                {
                    ReplyMsg((struct Message *)msg);
                }
            }

            bus->sdcb_BusFlags &= ~(UNITF_INTASK | UNITF_ACTIVE);
        }
    }
}
