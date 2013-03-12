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
#include <proto/kernel.h>

#include <asm/bcm2835.h>
#include <hardware/mmc.h>
#include <hardware/sdhc.h>

#include <string.h>

#include "sdcard_intern.h"
#include "timer.h"

#include LC_LIBDEFS_FILE

UBYTE FNAME_SDCBUS(MMIOReadByte)(ULONG reg, struct sdcard_Bus *bus)
{
    ULONG val = *(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3);

    return (val >> ((reg & 3) << 3)) & 0xFF;
}

UWORD FNAME_SDCBUS(MMIOReadWord)(ULONG reg, struct sdcard_Bus *bus)
{
    ULONG val = *(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3);

    return (val >> (((reg >> 1) & 1) << 4)) & 0xFFFF;
}

ULONG FNAME_SDCBUS(MMIOReadLong)(ULONG reg, struct sdcard_Bus *bus)
{
    return *(volatile ULONG *)(bus->sdcb_IOBase + reg);
}

void FNAME_SDCBUS(ArasanWriteLong)(ULONG reg, ULONG val, struct sdcard_Bus *bus)
{
    /* Bug: two SDC clock cycle delay required between successive chipset writes */
    while (sdcard_CurrentTime() < (bus->sdcb_LastWrite + 6))
        sdcard_Udelay(1);

    *(volatile ULONG *)(bus->sdcb_IOBase + reg) = val;
    bus->sdcb_LastWrite = sdcard_CurrentTime();
}

void FNAME_SDCBUS(MMIOWriteByte)(ULONG reg, UBYTE val, struct sdcard_Bus *bus)
{
    ULONG currval = *(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3);
    ULONG shift = (reg & 3) << 3;
    ULONG mask = 0xFF << shift;
    ULONG newval = (currval & ~mask) | (val << shift);

    FNAME_SDCBUS(ArasanWriteLong)(reg & ~3, newval, bus);
}

void FNAME_SDCBUS(MMIOWriteWord)(ULONG reg, UWORD val, struct sdcard_Bus *bus)
{
    ULONG currval = *(volatile ULONG *)(((ULONG)bus->sdcb_IOBase + reg) & ~3);
    ULONG shift = ((reg >> 1) & 1) << 4;
    ULONG mask = 0xFFFF << shift;
    ULONG newval = (currval & ~mask) | (val << shift);

    FNAME_SDCBUS(ArasanWriteLong)(reg & ~3, newval, bus);
}

void FNAME_SDCBUS(MMIOWriteLong)(ULONG reg, ULONG val, struct sdcard_Bus *bus)
{
    FNAME_SDCBUS(ArasanWriteLong)(reg, val, bus);
}

void FNAME_SDCBUS(SoftReset)(UBYTE mask, struct sdcard_Bus *bus)
{
    ULONG timeout = 100;

    FNAME_SDCBUS(MMIOWriteByte)(SDHCI_RESET, mask, bus);
    while (FNAME_SDCBUS(MMIOReadByte)(SDHCI_RESET, bus) & mask) {
        if (timeout == 0) {
            bug("[SDCard--] %s: Timeout\n", __PRETTY_FUNCTION__);
            break;
        }
        sdcard_Udelay(1000);
        timeout--;
    }
}

void FNAME_SDCBUS(SetClock)(ULONG speed, struct sdcard_Bus *bus)
{
    ULONG       sdcClkDiv, timeout;
    UWORD       sdcClkCtrlCur, sdcClkCtrl;

    sdcClkCtrlCur = FNAME_SDCBUS(MMIOReadWord)(SDHCI_CLOCK_CONTROL, bus);

    for (sdcClkDiv = 0; sdcClkDiv < V300_MAXCLKDIV; sdcClkDiv++) {
        if ((bus->sdcb_ClockMax / (sdcClkDiv + 1)) <= speed)
                break;
    }

    sdcClkCtrl = (sdcClkDiv & SDHCI_DIV_MASK) << SDHCI_DIVIDER_SHIFT;
    sdcClkCtrl |= ((sdcClkDiv & SDHCI_DIV_HI_MASK) >> SDHCI_DIV_MASK_LEN) << SDHCI_DIVIDER_HI_SHIFT;

    if (sdcClkCtrl != (sdcClkCtrlCur & ~(SDHCI_CLOCK_INT_EN|SDHCI_CLOCK_INT_STABLE|SDHCI_CLOCK_CARD_EN)))
    {
        FNAME_SDCBUS(MMIOWriteWord)(SDHCI_CLOCK_CONTROL, 0, bus);

        D(bug("[SDCard--] %s: Changing CLOCK_CONTROL [0x%04x -> 0x%04x] (div %d)\n", __PRETTY_FUNCTION__, sdcClkCtrlCur & ~(SDHCI_CLOCK_INT_EN|SDHCI_CLOCK_INT_STABLE|SDHCI_CLOCK_CARD_EN), sdcClkCtrl, sdcClkDiv));

        FNAME_SDCBUS(MMIOWriteWord)(SDHCI_CLOCK_CONTROL, (sdcClkCtrl | SDHCI_CLOCK_INT_EN), bus);

        timeout = 20;
        while (!((sdcClkCtrl = FNAME_SDCBUS(MMIOReadWord)(SDHCI_CLOCK_CONTROL, bus)) & SDHCI_CLOCK_INT_STABLE)) {
            if (timeout == 0) {
                D(bug("[SDCard--] %s: SDHCI Clock failed to stabilise\n", __PRETTY_FUNCTION__));
                break;
            }
            sdcard_Udelay(1000);
            timeout --;
        }
    }
    else
        if (sdcClkCtrlCur & SDHCI_CLOCK_CARD_EN)
            return;

    D(bug("[SDCard--] %s: Enabling clock...\n", __PRETTY_FUNCTION__));
    sdcClkCtrl |= SDHCI_CLOCK_CARD_EN;
    FNAME_SDCBUS(MMIOWriteWord)(SDHCI_CLOCK_CONTROL, sdcClkCtrl, bus);
}

void FNAME_SDCBUS(SetPowerLevel)(ULONG supportedlvls, BOOL lowest, struct sdcard_Bus *bus)
{
    UBYTE sdcReg = 0, lvlCur;

    if (lowest)
    {
        if (supportedlvls & MMC_VDD_165_195)
            sdcReg = SDHCI_POWER_180;
        else if (supportedlvls & (MMC_VDD_290_300|MMC_VDD_300_310))
            sdcReg = SDHCI_POWER_300;
        else if (supportedlvls & (MMC_VDD_320_330|MMC_VDD_330_340))
            sdcReg = SDHCI_POWER_330;
    }
    else
    {
        if (supportedlvls & (MMC_VDD_320_330|MMC_VDD_330_340))
            sdcReg = SDHCI_POWER_330;
        else if (supportedlvls & (MMC_VDD_290_300|MMC_VDD_300_310))
            sdcReg = SDHCI_POWER_300;
        else if (supportedlvls & MMC_VDD_165_195)
            sdcReg = SDHCI_POWER_180;
    }

    lvlCur = FNAME_SDCBUS(MMIOReadByte)(SDHCI_POWER_CONTROL, bus);
    if ((lvlCur & ~SDHCI_POWER_ON) != sdcReg)
    {
        D(bug("[SDCard--] %s: Changing Power Lvl [0x%x -> 0x%x]\n", __PRETTY_FUNCTION__, lvlCur & ~SDHCI_POWER_ON, sdcReg));
        FNAME_SDCBUS(MMIOWriteByte)(SDHCI_POWER_CONTROL, sdcReg, bus);
        sdcReg |= SDHCI_POWER_ON;
        FNAME_SDCBUS(MMIOWriteByte)(SDHCI_POWER_CONTROL, sdcReg, bus);
    }
    else
    {
        if (!(lvlCur & SDHCI_POWER_ON))
        {
            D(bug("[SDCard--] %s: Enabling Power Lvl [0x%x]\n", __PRETTY_FUNCTION__, lvlCur));
            lvlCur |= SDHCI_POWER_ON;
            FNAME_SDCBUS(MMIOWriteByte)(SDHCI_POWER_CONTROL, lvlCur, bus);
        }
    }
}

ULONG FNAME_SDCBUS(SendCmd)(struct TagItem *CmdTags, struct sdcard_Bus *bus)
{
    UWORD sdCommand      = (UWORD)GetTagData(SDCARD_TAG_CMD, 0, CmdTags);
    ULONG sdArg          = GetTagData(SDCARD_TAG_ARG, 0, CmdTags);
    ULONG sdResponseType = GetTagData(SDCARD_TAG_RSPTYPE, MMC_RSP_NONE, CmdTags);
    ULONG sdDataLen = 0, sdDataFlags;

    UWORD sdcTransMode = 0, sdCommandFlags;
    ULONG sdcInhibitMask = SDHCI_PS_CMD_INHIBIT;
    ULONG timeout = 10;
    ULONG ret = 0;

    if ((sdDataLen = GetTagData(SDCARD_TAG_DATALEN, 0, CmdTags)) > 0)
    {
        sdDataFlags = GetTagData(SDCARD_TAG_DATAFLAGS, 0, CmdTags);
    };

    /* Dont wait for DATA inihibit for stop commands */
    if (sdCommand != MMC_CMD_STOP_TRANSMISSION)
        sdcInhibitMask |= SDHCI_PS_DATA_INHIBIT;

    while (FNAME_SDCBUS(MMIOReadLong)(SDHCI_PRESENT_STATE, bus) & sdcInhibitMask) {
        if (timeout == 0) {
            D(bug("[SDCard--] %s: Controller failed to release inhibited bit(s).\n", __PRETTY_FUNCTION__));
            return -1;
        }
        sdcard_Udelay(1000);
        timeout--;
    }

    if (!(sdResponseType & MMC_RSP_PRESENT))
        sdCommandFlags = SDHCI_CMD_RESP_NONE;
    else if (sdResponseType & MMC_RSP_136)
        sdCommandFlags = SDHCI_CMD_RESP_LONG;
    else if (sdResponseType & MMC_RSP_BUSY)
        sdCommandFlags = SDHCI_CMD_RESP_SHORT_BUSY;
    else
        sdCommandFlags = SDHCI_CMD_RESP_SHORT;

    if (sdResponseType & MMC_RSP_CRC)
        sdCommandFlags |= SDHCI_CMD_CRC;
    if (sdResponseType & MMC_RSP_OPCODE)
        sdCommandFlags |= SDHCI_CMD_INDEX;
    if (sdDataLen > 0)
        sdCommandFlags |= SDHCI_CMD_DATA;

    if (sdDataLen > 0) {
        sdcTransMode = SDHCI_TRANSMOD_BLK_CNT_EN;
        D(bug("[SDCard--] %s: Configuring Data Transfer\n", __PRETTY_FUNCTION__));

        sdcard_SetLED(LED_ON);

        FNAME_SDCBUS(MMIOWriteByte)(SDHCI_TIMEOUT_CONTROL, SDHCI_TIMEOUT_MAX, bus);

        FNAME_SDCBUS(MMIOWriteWord)(SDHCI_BLOCK_SIZE, ((1 << 16) | ((sdDataLen > (1 << bus->sdcb_SectorShift)) ? (1 << bus->sdcb_SectorShift) : sdDataLen)), bus);
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

#if (1)
        FNAME_SDCBUS(MMIOWriteWord)(SDHCI_TRANSFER_MODE, sdcTransMode, bus);
#endif

        D(bug("[SDCard--] %s: Mode %08x [%d x %dBytes]\n", __PRETTY_FUNCTION__, sdcTransMode, (((sdDataLen >> bus->sdcb_SectorShift) > 0) ? (sdDataLen >> bus->sdcb_SectorShift) : 1), ((sdDataLen > (1 << bus->sdcb_SectorShift)) ? (1 << bus->sdcb_SectorShift) : sdDataLen)));
    }

#ifdef KernelBase
#undef KernelBase
#define KernelBase bus->sdcb_DeviceBase->sdcard_KernelBase
#endif
    KrnModifyIRQHandler(bus->sdcb_IRQHandle, bus, CmdTags);

    FNAME_SDCBUS(MMIOWriteLong)(SDHCI_ARGUMENT, sdArg, bus);
#if (0)
    if (sdcTransMode)
        FNAME_SDCBUS(MMIOWriteLong)(SDHCI_TRANSFER_MODE, (sdcTransMode << 16) | SDHCI_MAKE_CMD(sdCommand, sdCommandFlags), bus);
    else
#endif
        FNAME_SDCBUS(MMIOWriteWord)(SDHCI_COMMAND, SDHCI_MAKE_CMD(sdCommand, sdCommandFlags), bus);

    D(bug("[SDCard--] %s: CMD Sent\n", __PRETTY_FUNCTION__));

    return 0;
}

ULONG FNAME_SDCBUS(FinishCmd)(struct TagItem *CmdTags, struct sdcard_Bus *bus)
{
    struct TagItem *Response = NULL;

    D(UWORD sdCommand      = (UWORD)GetTagData(SDCARD_TAG_CMD, 0, CmdTags));
    ULONG sdResponseType = GetTagData(SDCARD_TAG_RSPTYPE, MMC_RSP_NONE, CmdTags);
    ULONG ret = 0;

    if (sdResponseType != MMC_RSP_NONE)
    {
        if ((Response = FindTagItem(SDCARD_TAG_RSP, CmdTags)) != NULL)
        {
            D(bug("[SDCard--] %s: Reading CMD %02d Response ", __PRETTY_FUNCTION__, sdCommand));
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
                    D(bug("[SDCard--] %s:   %08x%08x%08x%08x\n", __PRETTY_FUNCTION__, ((ULONG *)Response->ti_Data)[0], ((ULONG *)Response->ti_Data)[1], ((ULONG *)Response->ti_Data)[2], ((ULONG *)Response->ti_Data)[3]));
                }
            }
            else
            {
                Response->ti_Data = FNAME_SDCBUS(MMIOReadLong)(SDHCI_RESPONSE, bus);
                D(bug("[= %08x]\n", Response->ti_Data));
            }
        }
    }
    else
    {
        D(bug("[SDCard--] %s: CMD %02d Response received when none expected!", __PRETTY_FUNCTION__, sdCommand));
    }
    return ret;
}

ULONG FNAME_SDCBUS(FinishData)(struct TagItem *CmdTags, struct sdcard_Bus *bus)
{
    D(UWORD     sdCommand = (UWORD)GetTagData(SDCARD_TAG_CMD, 0, CmdTags));
    ULONG       sdcStateMask = SDHCI_PS_DATA_AVAILABLE | SDHCI_PS_SPACE_AVAILABLE,
                sdCommandMask = SDHCI_INT_SPACE_AVAIL | SDHCI_INT_DATA_AVAIL,
                sdData, sdDataLen = 0, sdDataFlags, sdStatus, sdcReg;
    ULONG       timeout = 1000;
    ULONG ret = 0;

    if ((sdData = GetTagData(SDCARD_TAG_DATA, 0, CmdTags)) != 0)
    {
        sdDataLen = GetTagData(SDCARD_TAG_DATALEN, 0, CmdTags);
        sdDataFlags = GetTagData(SDCARD_TAG_DATAFLAGS, 0, CmdTags);
        if (!(sdDataLen))
            sdData = 0;
    };

    if (sdData)
    {
        D(bug("[SDCard--] %s: Transfering CMD %02d Data..\n", __PRETTY_FUNCTION__, sdCommand));
        do {
            sdStatus = FNAME_SDCBUS(MMIOReadLong)(SDHCI_INT_STATUS, bus);
            if (sdStatus & SDHCI_INT_ERROR) {
                D(bug("[SDCard--] %s:    Error [status 0x%X]!\n", __PRETTY_FUNCTION__, sdStatus));
                ret = -1;
                break;
            }
            if ((sdStatus & sdCommandMask) && (FNAME_SDCBUS(MMIOReadLong)(SDHCI_PRESENT_STATE, bus) & sdcStateMask)) {
                ULONG currbyte, tranlen = (sdDataLen > (1 << bus->sdcb_SectorShift)) ? (1 << bus->sdcb_SectorShift) : sdDataLen;

                do
                {
                    D(bug("[SDCard--] %s: Attempting to read %dbytes\n", __PRETTY_FUNCTION__, tranlen));
                    for (currbyte = 0; currbyte < tranlen; currbyte++)
                    {
                        DUMP(
                            if ((currbyte % 16) == 0)
                            {
                                bug("[SDCard--] %s:    ", __PRETTY_FUNCTION__);
                            }
                        )
                        if ((currbyte % 4) == 0)
                        {
                            sdcReg = FNAME_SDCBUS(MMIOReadLong)(SDHCI_BUFFER, bus);
                        }
                        *(UBYTE *)sdData = sdcReg & 0xFF;
                        sdData++;
                        sdDataLen--;
                        sdcReg >>= 8;
                        DUMP(
                            if ((currbyte % 4) == 3)
                            {
                                bug(" %08x", *(ULONG *)(sdData - 4));
                            }
                            if ((currbyte % 16) == 15)
                            {
                                bug("\n");
                            }
                        )
                    }
                    DUMP(
                        if ((currbyte % 16) != 15)
                        {
                            bug("\n");
                        }
                    )
                } while (sdDataLen > 0);

                break;
            }
            else if (!(sdStatus & SDHCI_INT_DATA_END))
            {
                sdcard_Udelay(1000);

                if (timeout-- <= 0)
                {
                    D(bug("[SDCard--] %s:    Timeout!\n", __PRETTY_FUNCTION__));
                    ret = -1;
                    break;
                }
            }
        } while (!(sdStatus & SDHCI_INT_DATA_END));

        sdcard_SetLED(LED_OFF);
    }

    return ret;
}

ULONG FNAME_SDCBUS(WaitCmd)(ULONG mask, ULONG timeout, struct sdcard_Bus *bus)
{
    do {
        if (bus->sdcb_BusStatus & SDHCI_INT_ERROR)
            break;
        sdcard_Udelay(1000);
    } while (((bus->sdcb_BusStatus & mask) == mask) && (--timeout > 0));

    if ((timeout <= 0) || (bus->sdcb_BusStatus & SDHCI_INT_ERROR))
    {
        return -1;
    }

    return 0;
}

ULONG FNAME_SDCBUS(WaitUnitStatus)(ULONG timeout, struct sdcard_Unit *sdcUnit)
{
    struct TagItem sdcStatusTags[] =
    {
        {SDCARD_TAG_CMD,         MMC_CMD_SEND_STATUS},
        {SDCARD_TAG_ARG,         sdcUnit->sdcu_CardRCA << 16},
        {SDCARD_TAG_RSPTYPE,     MMC_RSP_R1},
        {SDCARD_TAG_RSP,         0},
        {TAG_DONE,               0}
    };

    UBYTE retryreq = 5;

    do {
        if ((FNAME_SDCBUS(SendCmd)(sdcStatusTags, sdcUnit->sdcu_Bus) != -1) && (FNAME_SDCBUS(WaitCmd)(SDHCI_INT_RESPONSE, 10, sdcUnit->sdcu_Bus) != -1))
        {
            if ((sdcStatusTags[3].ti_Data & MMC_STATUS_RDY_FOR_DATA) &&
                (sdcStatusTags[3].ti_Data & MMC_STATUS_STATE_MASK) != MMC_STATUS_STATE_PRG)
                break;
            else if (sdcStatusTags[3].ti_Data & MMC_STATUS_MASK) {
                D(bug("[SDCard%02ld] %s: Status Error = %08x\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, sdcStatusTags[3].ti_Data));
                return -1;
            }
        } else if (--retryreq < 0)
                return -1;

        sdcard_Udelay(1000);
    } while (--timeout > 0);

    if (timeout <= 0) {
        D(bug("[SDCard%02ld] %s: Timeout\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
        return -1;
    }

    bug("[SDCard%02ld] %s: State = %08x\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, sdcStatusTags[3].ti_Data & MMC_STATUS_STATE_MASK);

    return 0;
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


ULONG FNAME_SDCBUS(SDSCSwitch)(BOOL test, int group, UBYTE value, APTR buf, struct sdcard_Unit *sdcUnit)
{
    struct TagItem sdcSwitchTags[] =
    {
        {SDCARD_TAG_CMD,        SD_CMD_SWITCH_FUNC},
        {SDCARD_TAG_ARG,        0},
        {SDCARD_TAG_RSPTYPE,    MMC_RSP_R1},
        {SDCARD_TAG_RSP,        0},
        {SDCARD_TAG_DATA,       (IPTR)buf},
        {SDCARD_TAG_DATALEN,    64},
        {SDCARD_TAG_DATAFLAGS,  MMC_DATA_READ},
        {TAG_DONE,              0}
    };
    ULONG retVal;
    
    D(bug("[SDCard%02ld] %s()\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    sdcSwitchTags[1].ti_Data = ((test) ? 0 : (1 << 31)) | 0xFFFFFF;
    sdcSwitchTags[1].ti_Data &= ~(0xF << (group * 4));
    sdcSwitchTags[1].ti_Data |= value << (group * 4);

    if ((retVal =  FNAME_SDCBUS(SendCmd)(sdcSwitchTags, sdcUnit->sdcu_Bus)) != -1)
    {
        retVal =  FNAME_SDCBUS(WaitCmd)(SDHCI_INT_RESPONSE, 10, sdcUnit->sdcu_Bus);
    }
    return retVal;
}

ULONG FNAME_SDCBUS(SDSCChangeFrequency)(struct sdcard_Unit *sdcUnit)
{
    unsigned int        timeout;
    ULONG               sdcRespBuf[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct TagItem      sdcChFreqTags[] =
    {
        {SDCARD_TAG_CMD,        0},
        {SDCARD_TAG_ARG,        0},
        {SDCARD_TAG_RSPTYPE,    0},
        {SDCARD_TAG_RSP,        0},
        {0,                     (IPTR)sdcRespBuf}, /* SDCARD_TAG_DATA */
        {SDCARD_TAG_DATALEN,    8},
        {SDCARD_TAG_DATAFLAGS,  MMC_DATA_READ},
        {TAG_DONE,              0}
    };

    /* Read the SCR to find out if higher speeds are supported ..*/
    timeout = 3;
    do {
        D(bug("[SDCard%02ld] %s: Preparing for Card App Command ... \n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
        sdcChFreqTags[0].ti_Data = MMC_CMD_APP_CMD;
        sdcChFreqTags[1].ti_Data = sdcUnit->sdcu_CardRCA << 16;
        sdcChFreqTags[2].ti_Data = MMC_RSP_R1;
        sdcChFreqTags[4].ti_Tag = TAG_DONE;

        if ((FNAME_SDCBUS(SendCmd)(sdcChFreqTags, sdcUnit->sdcu_Bus) == -1) || (FNAME_SDCBUS(WaitCmd)(SDHCI_INT_RESPONSE, 10, sdcUnit->sdcu_Bus) == -1))
        {
            D(bug("[SDCard%02ld] %s: App Command Failed\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
            return FALSE;
        }
        D(bug("[SDCard%02ld] %s: App Command Response = %08x\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, sdcChFreqTags[3].ti_Data));

        sdcChFreqTags[0].ti_Data = SD_CMD_APP_SEND_SCR;
        sdcChFreqTags[1].ti_Data = 0;
        sdcChFreqTags[2].ti_Data = MMC_RSP_R1;
        sdcChFreqTags[4].ti_Tag = SDCARD_TAG_DATA;

        D(bug("[SDCard%02ld] %s: Querying SCR Register ... \n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
        if ((FNAME_SDCBUS(SendCmd)(sdcChFreqTags, sdcUnit->sdcu_Bus) != -1) && (FNAME_SDCBUS(WaitCmd)(SDHCI_INT_RESPONSE, 10, sdcUnit->sdcu_Bus) != -1))
        {
            D(bug("[SDCard%02ld] %s: Query Response = %08x\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, sdcChFreqTags[3].ti_Data));
            break;
        }
    } while (--timeout > 0);

    if (timeout > 0)
    {
        if (AROS_BE2LONG(sdcRespBuf[0]) & SD_SCR_DATA4BIT)
            sdcUnit->sdcu_Flags |= AF_Card_4bitData;

        /* v1.0 SDCards don't support switching */
        if (((AROS_BE2LONG(sdcRespBuf[0]) >> 24) & 0xf) < 1)
        {
            D(bug("[SDCard%02ld] %s: Card doesnt support Switching\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
            return 0;
        }

        timeout = 4;
        while (--timeout > 0) {
            if (FNAME_SDCBUS(SDSCSwitch)(TRUE, 0, 1, sdcRespBuf, sdcUnit) != -1)
            {
                /* The high-speed function is busy.  Try again */
                if (!(AROS_BE2LONG(sdcRespBuf[7]) & SD_SCR_HIGHSPEED))
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
            if (!(AROS_BE2LONG(sdcRespBuf[3]) & SD_SCR_HIGHSPEED))
            {
                D(bug("[SDCard%02ld] %s: Card doesnt support Highspeed mode\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
                return 0;
            }

            if (FNAME_SDCBUS(SDSCSwitch)(FALSE, 0, 1, sdcRespBuf, sdcUnit) != -1)
            {
                if ((AROS_BE2LONG(sdcRespBuf[4]) & 0x0F000000) == 0x01000000)
                   sdcUnit->sdcu_Flags |= AF_Card_HighSpeed;
            }
        }
    }
    else
    {
        D(bug("[SDCard%02ld] %s: Timeout Querying SCR\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
        return -1;
    }

    return 0;
}

ULONG FNAME_SDCBUS(MMCChangeFrequency)(struct sdcard_Unit *sdcUnit)
{
    return 0;
}

/********** BUS IRQ HANDLER **************/

void FNAME_SDCBUS(BusIRQ)(struct sdcard_Bus *bus, struct TagItem *IRQCommandTags)
{
    BOOL error = FALSE;

    D(bug("[SDCard**] %s(bus: %u @ 0x%p)\n", __PRETTY_FUNCTION__, bus->sdcb_BusNum, bus));

    if (!(bus))
    {
        D(bug("[SDCard**] %s: Bad Params!\n", __PRETTY_FUNCTION__));
        return;
    }

    bus->sdcb_BusStatus = FNAME_SDCBUS(MMIOReadLong)(SDHCI_INT_STATUS, bus);

    D(bug("[SDCard**] %s: Status = %08x\n", __PRETTY_FUNCTION__, bus->sdcb_BusStatus));

    if (!(bus->sdcb_BusStatus & SDHCI_INT_ERROR))
    {
        if (bus->sdcb_BusStatus & (SDHCI_INT_CARD_INSERT|SDHCI_INT_CARD_REMOVE))
        {
            bus->sdcb_BusFlags &= ~AF_Bus_MediaPresent;
            bus->sdcb_BusFlags |= AF_Bus_MediaChanged;

            if (bus->sdcb_BusStatus & SDHCI_INT_CARD_INSERT)
                bus->sdcb_BusFlags |= AF_Bus_MediaPresent;

            FNAME_SDCBUS(MMIOWriteLong)(SDHCI_INT_STATUS, bus->sdcb_BusStatus & (SDHCI_INT_CARD_INSERT|SDHCI_INT_CARD_REMOVE), bus);
            bus->sdcb_BusStatus &= ~(SDHCI_INT_CARD_INSERT|SDHCI_INT_CARD_REMOVE);
        }
        if (bus->sdcb_BusStatus & SDHCI_INT_CMD_MASK)
        {
            if (IRQCommandTags)
            {
                if (bus->sdcb_BusStatus & SDHCI_INT_RESPONSE)
                {
                    FNAME_SDCBUS(FinishCmd)(IRQCommandTags, bus);
                }
            }
            else
            {
                D(bug("[SDCard**] %s: Response IRQ received - but no command in progress!\n", __PRETTY_FUNCTION__));
            }

            FNAME_SDCBUS(MMIOWriteLong)(SDHCI_INT_STATUS, bus->sdcb_BusStatus & SDHCI_INT_CMD_MASK, bus);
            bus->sdcb_BusStatus &= ~SDHCI_INT_CMD_MASK; 
        }

        if (bus->sdcb_BusStatus & SDHCI_INT_DATA_MASK)
        {
            if (IRQCommandTags)
            {
                if (bus->sdcb_BusStatus & SDHCI_INT_DATA_END|SDHCI_INT_DMA_END|SDHCI_INT_DATA_AVAIL|SDHCI_INT_DATA_END_BIT)
                {
                    FNAME_SDCBUS(FinishData)(IRQCommandTags, bus);
                }
            }
            else
            {
                D(bug("[SDCard**] %s: Data IRQ received - but no command in progress!\n", __PRETTY_FUNCTION__));
            }

            FNAME_SDCBUS(MMIOWriteLong)(SDHCI_INT_STATUS, bus->sdcb_BusStatus & SDHCI_INT_DATA_MASK, bus);
            bus->sdcb_BusStatus &= ~SDHCI_INT_DATA_MASK; 
        }
    }
    else
    {
        D(bug("[SDCard**] %s: ERROR\n", __PRETTY_FUNCTION__));
        if (bus->sdcb_BusStatus & SDHCI_INT_ACMD12ERR)
        {
            D(bug("[SDCard**] %s:       [acmd12err = %04x    ]\n", __PRETTY_FUNCTION__, FNAME_SDCBUS(MMIOReadWord)(SDHCI_ACMD12_ERR, bus)));
        }
        error = TRUE;
    }

    if (error)
    {
        if (bus->sdcb_BusStatus & bus->sdcb_IntrMask)
        {
            D(bug("[SDCard**] %s: Clearing Unhandled Interrupts [%08x]\n", __PRETTY_FUNCTION__, bus->sdcb_BusStatus & bus->sdcb_IntrMask));
            FNAME_SDCBUS(MMIOWriteLong)(SDHCI_INT_STATUS, bus->sdcb_BusStatus & bus->sdcb_IntrMask, bus);
            bus->sdcb_BusStatus &= ~bus->sdcb_IntrMask;
        }
        D(bug("[SDCard**] %s: Reseting SDHCI CMD/DATA\n", __PRETTY_FUNCTION__));

        FNAME_SDCBUS(SoftReset)(SDHCI_RESET_CMD, bus);
        FNAME_SDCBUS(SoftReset)(SDHCI_RESET_DATA, bus);
    }
    D(bug("[SDCard**] %s: Done.\n", __PRETTY_FUNCTION__));
}

/********** BUS MANAGEMENT TASK **************/

void FNAME_SDCBUS(BusTask)(struct sdcard_Bus *bus)
{
    struct SDCardBase *SDCardBase = bus->sdcb_DeviceBase;
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
