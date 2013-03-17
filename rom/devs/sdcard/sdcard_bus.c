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
#include <proto/utility.h>
#include <proto/kernel.h>

#include <asm/bcm2835.h>
#include <hardware/mmc.h>
#include <hardware/sdhc.h>

#include <string.h>

#include "sdcard_base.h"
#include "sdcard_bus.h"
#include "sdcard_unit.h"
#include "timer.h"

#include LC_LIBDEFS_FILE

BOOL FNAME_SDCBUS(StartUnit)(struct sdcard_Unit *sdcUnit)
{
    struct TagItem sdcStartTags[] =
    {
        {SDCARD_TAG_CMD,         MMC_CMD_SELECT_CARD},
        {SDCARD_TAG_ARG,         sdcUnit->sdcu_CardRCA << 16},
        {SDCARD_TAG_RSPTYPE,     MMC_RSP_R1},
        {SDCARD_TAG_RSP,         0},
        {TAG_DONE,               0}
    };

    if (sdcUnit->sdcu_Flags & AF_Card_MMC)
    {
        FNAME_SDCUNIT(MMCChangeFrequency)(sdcUnit);
    }
    else
    {
        FNAME_SDCUNIT(SDSCChangeFrequency)(sdcUnit);
    }

    if ((FNAME_SDCBUS(SendCmd)(sdcStartTags, sdcUnit->sdcu_Bus) != -1) && (FNAME_SDCBUS(WaitCmd)(SDHCI_INT_RESPONSE, 10, sdcUnit->sdcu_Bus) != -1))
    {
        if (FNAME_SDCUNIT(WaitStatus)(1000, sdcUnit) == -1)
        {
            D(bug("[SDCard%02ld] %s: Failed to Wait for Cards status\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
        }

        D(bug("[SDCard%02ld] %s: Selected card with RCA %d [select response %08x]\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, sdcUnit->sdcu_CardRCA, sdcStartTags[3].ti_Data));
        D(bug("[SDCard%02ld] %s: Card is now operating in Transfer Mode\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));

        if (!(sdcUnit->sdcu_Flags & AF_Card_HighCapacity))
        {
            sdcStartTags[0].ti_Data = MMC_CMD_SET_BLOCKLEN;
            sdcStartTags[1].ti_Data = 1 << sdcUnit->sdcu_Bus->sdcb_SectorShift;
            sdcStartTags[2].ti_Data = MMC_RSP_R1;
            sdcStartTags[3].ti_Data = 0;
            if ((FNAME_SDCBUS(SendCmd)(sdcStartTags, sdcUnit->sdcu_Bus) != -1) && (FNAME_SDCBUS(WaitCmd)(SDHCI_INT_RESPONSE, 10, sdcUnit->sdcu_Bus) != -1))
            {
                D(bug("[SDCard%02ld] %s: Blocklen set to %d\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, sdcStartTags[1].ti_Data));
            }
            else
            {
                D(bug("[SDCard%02ld] %s: Failed to change Blocklen\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
            }
        }

        if (sdcUnit->sdcu_Flags & AF_Card_MMC)
        {
            if (sdcUnit->sdcu_Flags & AF_Card_HighSpeed)
            {
                if (sdcUnit->sdcu_Flags & AB_Card_HighSpeed52)
                    FNAME_SDCBUS(SetClock)(52000000, sdcUnit->sdcu_Bus);
                else
                    FNAME_SDCBUS(SetClock)(26000000, sdcUnit->sdcu_Bus);
            }
            else
                FNAME_SDCBUS(SetClock)(20000000, sdcUnit->sdcu_Bus);
        }
        else
        {
            if (sdcUnit->sdcu_Flags & AF_Card_HighSpeed)
                FNAME_SDCBUS(SetClock)(50000000, sdcUnit->sdcu_Bus);
            else
                FNAME_SDCBUS(SetClock)(25000000, sdcUnit->sdcu_Bus);
        }
    }
    return TRUE;
}

BOOL FNAME_SDCBUS(RegisterUnit)(struct sdcard_Bus *bus)
{
    unsigned int        sdcCardPower, timeout = 1000;
    LIBBASETYPEPTR      LIBBASE = bus->sdcb_DeviceBase;
    ULONG               sdcRsp136[4] = {0, 0, 0, 0};
    struct sdcard_Unit  *sdcUnit = NULL;
    struct DeviceNode   *devnode;
    struct TagItem sdcRegTags[] =
    {
        {SDCARD_TAG_CMD,         0},
        {SDCARD_TAG_ARG,         0},
        {SDCARD_TAG_RSPTYPE,     0},
        {SDCARD_TAG_RSP,         0},
        {TAG_DONE,               0}
    };
    BOOL                sdcHighCap = FALSE;
    IPTR                pp[24];

    D(bug("[SDCard>>] %s()\n", __PRETTY_FUNCTION__));

    D(bug("[SDCard>>] %s: Putting card into Idle state ...\n", __PRETTY_FUNCTION__));
    sdcRegTags[0].ti_Data = MMC_CMD_GO_IDLE_STATE;
    sdcRegTags[1].ti_Data = 0;
    sdcRegTags[2].ti_Data = MMC_RSP_NONE;
    if (FNAME_SDCBUS(SendCmd)(sdcRegTags, bus) == -1)
    {
        D(bug("[SDCard>>] %s: Error: Card failed to go idle!\n", __PRETTY_FUNCTION__));
        return FALSE;
    }

    sdcard_Udelay(2000);

    sdcRegTags[0].ti_Data = SD_CMD_SEND_IF_COND;
    sdcRegTags[1].ti_Data = ((bus->sdcb_Power & 0xFF8000) != 0) << 8 | 0xAA;
    sdcRegTags[2].ti_Data = MMC_RSP_R7;
    D(bug("[SDCard>>] %s: Querying Interface conditions [%08x] ...\n", __PRETTY_FUNCTION__, sdcRegTags[1].ti_Data));

    if ((FNAME_SDCBUS(SendCmd)(sdcRegTags, bus) != -1)  && (FNAME_SDCBUS(WaitCmd)(SDHCI_INT_RESPONSE, 10, bus) != -1))
    {
        D(bug("[SDCard>>] %s: Query Response = %08x\n", __PRETTY_FUNCTION__, sdcRegTags[3].ti_Data));
        D(bug("[SDCard>>] %s: SD2.0 Compliant Card .. publishing high-capacity support\n", __PRETTY_FUNCTION__));
        sdcHighCap = TRUE;
    }
    else
    {
        D(bug("[SDCard>>] %s: SD1.0 Compliant Card\n", __PRETTY_FUNCTION__));
    }

    do {
        D(bug("[SDCard>>] %s: Preparing for Card App Command ...\n", __PRETTY_FUNCTION__));
        sdcRegTags[0].ti_Data = MMC_CMD_APP_CMD;
        sdcRegTags[1].ti_Data = 0;
        sdcRegTags[2].ti_Data = MMC_RSP_R1;

        if ((FNAME_SDCBUS(SendCmd)(sdcRegTags, bus) == -1) || (FNAME_SDCBUS(WaitCmd)(SDHCI_INT_RESPONSE, 10, bus) == -1))
        {
            D(bug("[SDCard>>] %s: App Command Failed\n", __PRETTY_FUNCTION__));
            return FALSE;
        }
        D(bug("[SDCard>>] %s: App Command Response = %08x\n", __PRETTY_FUNCTION__, sdcRegTags[3].ti_Data));

        sdcRegTags[0].ti_Data = SD_CMD_APP_SEND_OP_COND;
        sdcRegTags[1].ti_Data = (bus->sdcb_Power & 0xFF8000) | (sdcHighCap ? OCR_HCS : 0);
#warning "TODO: Publish support for OCR_S18R/OCR_XPC on capable Hosts"
        sdcRegTags[2].ti_Data = MMC_RSP_R3;

        D(bug("[SDCard>>] %s: Querying Operating conditions [%08x] ...\n", __PRETTY_FUNCTION__, sdcRegTags[1].ti_Data));
        if ((FNAME_SDCBUS(SendCmd)(sdcRegTags, bus) != -1) && (FNAME_SDCBUS(WaitCmd)(SDHCI_INT_RESPONSE, 10, bus) != -1))
        {
            D(bug("[SDCard>>] %s: Query Response = %08x\n", __PRETTY_FUNCTION__, sdcRegTags[3].ti_Data));
            sdcard_Udelay(1000);
        }
        else
        {
            D(bug("error (-1)\n"));
            return FALSE;
        }
    } while ((!(sdcRegTags[3].ti_Data & OCR_BUSY)) && (--timeout > 0));

    sdcHighCap = FALSE;

    if (timeout > 0)
    {
        D(bug("[SDCard>>] %s: Card OCR = %08x\n", __PRETTY_FUNCTION__, (sdcRegTags[3].ti_Data & 0xFFFF00)));

        sdcCardPower = bus->sdcb_Power & (sdcRegTags[3].ti_Data & 0xFFFF00);

        D(bug("[SDCard>>] %s: Card is now operating in Identification Mode\n", __PRETTY_FUNCTION__));
        
        if (sdcRegTags[3].ti_Data & OCR_HCS)
        {
            D(bug("[SDCard>>] %s: High Capacity Card detected\n", __PRETTY_FUNCTION__));
            sdcHighCap = TRUE;
        }

        if (sdcRegTags[3].ti_Data & OCR_S18R)
        {
            D(bug("[SDCard>>] %s: Begin Voltage Switching..\n", __PRETTY_FUNCTION__));
        }

        FNAME_SDCBUS(SetPowerLevel)(sdcCardPower, TRUE, bus);

        /* Put the "card" into identify mode*/ 
        D(bug("[SDCard>>] %s: Querying Card Identification Data ...\n", __PRETTY_FUNCTION__));
        sdcRegTags[0].ti_Data = MMC_CMD_ALL_SEND_CID;
        sdcRegTags[1].ti_Data = 0;
        sdcRegTags[2].ti_Data = MMC_RSP_R2;
        sdcRegTags[3].ti_Data = (IPTR)sdcRsp136;
        if ((FNAME_SDCBUS(SendCmd)(sdcRegTags, bus) != -1) && (FNAME_SDCBUS(WaitCmd)(SDHCI_INT_RESPONSE, 10, bus) != -1))
        {
            if (sdcRegTags[3].ti_Data)
            {
                D(bug("[SDCard>>] %s: # Card Identification Data (CID) Register\n", __PRETTY_FUNCTION__));
                D(bug("[SDCard>>] %s: # ======================================\n", __PRETTY_FUNCTION__));
                D(bug("[SDCard>>] %s: #         Manuafacturer ID (MID) : %06x\n", __PRETTY_FUNCTION__, FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 120, 8)));
                D(bug("[SDCard>>] %s: #         Product Name (PNM) : %c%c%c%c%c\n", __PRETTY_FUNCTION__, FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 96, 8), FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 88, 8), FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 80, 8), FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 72, 8), FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 64, 8)));
                D(bug("[SDCard>>] %s: #         Product Revision (PRV) : %d.%d\n", __PRETTY_FUNCTION__, FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 60, 4), FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 56, 4)));
                D(bug("[SDCard>>] %s: #         Serial number (PSN) : %08x\n", __PRETTY_FUNCTION__,  FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 24, 32)));
                D(bug("[SDCard>>] %s: #         Manufacturing Date Code (MDT) : %d/%d\n", __PRETTY_FUNCTION__, FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 8, 4), FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 12, 8)));
                D(bug("[SDCard>>] %s: #         CRC7 checksum (CRC7) : %x\n", __PRETTY_FUNCTION__, FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 1, 7)));
                D(bug("[SDCard>>] %s: #         Reserved : %x\n", __PRETTY_FUNCTION__, FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 0, 1)));
            }

            D(bug("[SDCard>>] %s: Querying Card Relative Address... \n", __PRETTY_FUNCTION__));
            sdcRegTags[0].ti_Data = SD_CMD_SEND_RELATIVE_ADDR;
            sdcRegTags[1].ti_Data = 0;
            sdcRegTags[2].ti_Data = MMC_RSP_R6;
            sdcRegTags[3].ti_Data = 0;
            if ((FNAME_SDCBUS(SendCmd)(sdcRegTags, bus) != -1) && (FNAME_SDCBUS(WaitCmd)(SDHCI_INT_RESPONSE, 10, bus) != -1))
            {
                if ((sdcUnit = AllocVecPooled(LIBBASE->sdcard_MemPool, sizeof(struct sdcard_Unit))) != NULL)
                {
                    sdcUnit->sdcu_Bus = bus;
                    if ((sdcUnit->sdcu_UnitNum = bus->sdcb_BusUnits->sdcbu_UnitCnt++) > bus->sdcb_BusUnits->sdcbu_UnitMax)
                    {
                        return FALSE;
                    }
                    (&bus->sdcb_BusUnits->sdcbu_Units)[sdcUnit->sdcu_UnitNum] = sdcUnit;
                    sdcUnit->sdcu_UnitNum += bus->sdcb_BusUnits->sdcbu_UnitBase;
                    sdcUnit->sdcu_CardRCA = (sdcRegTags[3].ti_Data >> 16) & 0xFFFF;
                    sdcUnit->sdcu_CardPower = sdcCardPower;

                    D(bug("[SDCard>>] %s: Card RCA = %d\n", __PRETTY_FUNCTION__, sdcUnit->sdcu_CardRCA));

                    sdcRegTags[0].ti_Data = MMC_CMD_SEND_CSD;
                    sdcRegTags[1].ti_Data = sdcUnit->sdcu_CardRCA << 16;
                    sdcRegTags[2].ti_Data = MMC_RSP_R2;
                    sdcRegTags[3].ti_Data = (IPTR)sdcRsp136;
                    D(bug("[SDCard%02ld] %s: Querying Card Specific Data [%08x] ...\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, sdcRegTags[1].ti_Data));
                    if ((FNAME_SDCBUS(SendCmd)(sdcRegTags, bus) != -1) && (FNAME_SDCBUS(WaitCmd)(SDHCI_INT_RESPONSE, 10, bus) != -1))
                    {
                        if (FNAME_SDCUNIT(WaitStatus)(1000, sdcUnit) == -1)
                        {
                            D(bug("[SDCard%02ld] %s: Failed to Wait for Cards status\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
                        }

                        if (sdcRegTags[3].ti_Data)
                        {
                            int __csdstruct = FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 126, 2);
                            D(bug("[SDCard%02ld] %s: # Card Specific Data (CSD) Register\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
                            D(bug("[SDCard%02ld] %s: # =================================\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
                            D(bug("[SDCard%02ld] %s: #         CSD_STRUCTURE : %x ", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, __csdstruct));

                            sdcUnit->sdcu_Read32                = FNAME_SDCIO(ReadSector32);
                            sdcUnit->sdcu_Write32               = FNAME_SDCIO(WriteSector32);
                            sdcUnit->sdcu_Bus->sdcb_BusFlags    = AF_Bus_MediaPresent;
#if defined(SDHCI_READONLY)
                            sdcUnit->sdcu_Flags                 = AF_Card_WriteProtect;
#endif
                            if (sdcHighCap)
                            {
                                sdcUnit->sdcu_Flags             |= AF_Card_HighCapacity;
                                sdcUnit->sdcu_Read64            = FNAME_SDCIO(ReadSector64);
                                sdcUnit->sdcu_Write64           = FNAME_SDCIO(WriteSector64);
                            }

                            switch (__csdstruct)
                            {
                                case 0:
                                {
                                    D(bug("[SDSC Card]\n"));
                                    pp[DE_SIZEBLOCK + 4] = 2 << (FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 80, 4) - 1);
                                    pp[DE_SECSPERBLOCK + 4] = pp[DE_SIZEBLOCK + 4] >> 9;
                                    pp[DE_HIGHCYL + 4] = ((1 + FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 62, 12)) << (FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 47, 3) + 2));
                                    break;
                                }
                                case 1:
                                {
                                    D(bug("[SDHC/XC Card]\n"));
                                    
                                    pp[DE_SECSPERBLOCK + 4] = 2;
                                    pp[DE_SIZEBLOCK + 4] = 2 << (10 - 1);
                                    pp[DE_HIGHCYL + 4] = ((1 + FNAME_SDCBUS(Rsp136Unpack)(sdcRsp136, 48, 22)) * (2 << (9 - 1)));

                                    sdcUnit->sdcu_Flags         |= AF_Card_MMC;

                                    break;
                                }
                                default:
                                    D(bug("[SDCard%02ld] %s: Unsupported Card\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
                                    return FALSE;
                            }

                            sdcUnit->sdcu_Cylinders     = pp[DE_HIGHCYL + 4];
                            sdcUnit->sdcu_Heads         = 1;
                            sdcUnit->sdcu_Sectors       = pp[DE_SECSPERBLOCK + 4];
                            sdcUnit->sdcu_Capacity      = sdcUnit->sdcu_Cylinders * sdcUnit->sdcu_Heads * sdcUnit->sdcu_Sectors;

                            sdcUnit->sdcu_Eject         = FNAME_SDCIO(Eject);

                            D(bug("[SDCard%02ld] %s: #         READ_BL_LEN : %dbytes\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, pp[DE_SIZEBLOCK + 4] / sdcUnit->sdcu_Sectors));
                            D(bug("[SDCard%02ld] %s: #         C_SIZE : %d\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, sdcUnit->sdcu_Cylinders));

                            pp[0] 		        = (IPTR)"MMC0";
                            pp[1]		        = (IPTR)MOD_NAME_STRING;
                            pp[2]		        = 0;
                            pp[DE_TABLESIZE    + 4]     = DE_BOOTBLOCKS;
                            pp[DE_NUMHEADS     + 4]     = sdcUnit->sdcu_Heads;
                            pp[DE_BLKSPERTRACK + 4]     = 1;
                            pp[DE_RESERVEDBLKS + 4]     = 2;
                            pp[DE_LOWCYL       + 4]     = 0;
                            pp[DE_NUMBUFFERS   + 4]     = 10;
                            pp[DE_BUFMEMTYPE   + 4]     = MEMF_PUBLIC | MEMF_31BIT;
                            pp[DE_MAXTRANSFER  + 4]     = 0x00200000;
                            pp[DE_MASK         + 4]     = 0x7FFFFFFE;
                            pp[DE_BOOTPRI      + 4]     = 0;
                            pp[DE_DOSTYPE      + 4]     = AROS_MAKE_ID('D','O','S','\001');
                            pp[DE_CONTROL      + 4]     = 0;
                            pp[DE_BOOTBLOCKS   + 4]     = 2;

                            devnode = MakeDosNode(pp);

                            if (devnode)
                            {
                                bug("[SDCard%02ld] %b: [%ldMB Capacity]\n", sdcUnit->sdcu_UnitNum, devnode->dn_Name, sdcUnit->sdcu_Capacity >> 11);
                                D(bug("[SDCard%02ld] %s:        StartCyl:%d, EndCyl:%d ..\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__,
                                      pp[DE_LOWCYL + 4], pp[DE_HIGHCYL + 4]));
                                D(bug("[SDCard%02ld] %s:        BlockSize:%d, SectorsPerBlock:%d ..\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__,
                                      pp[DE_SIZEBLOCK + 4], sdcUnit->sdcu_Sectors));

                                AddBootNode(pp[DE_BOOTPRI + 4], ADNF_STARTPROC, devnode, NULL);
                                D(bug("[SDCard%02ld] %s: Unit detection complete\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));

                                return TRUE;
                            }
                        }
                    }
                    else
                    {
                        D(bug("[SDCard%02ld] %s: Card failed to send CSD\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
                    }
                }
                else
                {
                    D(bug("[SDCard%02ld] %s: Failed to allocate Unit\n", bus->sdcb_BusUnits->sdcbu_UnitCnt, __PRETTY_FUNCTION__));
                }
            }
            else
            {
                D(bug("[SDCard>>] %s: Card failed to send RCA\n", __PRETTY_FUNCTION__));
            }
        }
        else
        {
            D(bug("[SDCard>>] %s: Card failed to send CID\n", __PRETTY_FUNCTION__));
        }
    }
    else
    {
        D(bug("[SDCard>>] %s: Card failed to initialise\n", __PRETTY_FUNCTION__));
    }

    return FALSE;
}

void FNAME_SDCBUS(SoftReset)(UBYTE mask, struct sdcard_Bus *bus)
{
    ULONG timeout = 100;

    bus->sdcb_IOWriteByte(SDHCI_RESET, mask, bus);
    while (bus->sdcb_IOReadByte(SDHCI_RESET, bus) & mask) {
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

    sdcClkCtrlCur = bus->sdcb_IOReadWord(SDHCI_CLOCK_CONTROL, bus);

    sdcClkDiv = FNAME_SDCBUS(GetClockDiv)(speed, bus);

    sdcClkCtrl = (sdcClkDiv & SDHCI_DIV_MASK) << SDHCI_DIVIDER_SHIFT;
    sdcClkCtrl |= ((sdcClkDiv & SDHCI_DIV_HI_MASK) >> SDHCI_DIV_MASK_LEN) << SDHCI_DIVIDER_HI_SHIFT;

    if (sdcClkCtrl != (sdcClkCtrlCur & ~(SDHCI_CLOCK_INT_EN|SDHCI_CLOCK_INT_STABLE|SDHCI_CLOCK_CARD_EN)))
    {
        bus->sdcb_IOWriteWord(SDHCI_CLOCK_CONTROL, 0, bus);

        D(bug("[SDCard--] %s: Changing CLOCK_CONTROL [0x%04x -> 0x%04x] (div %d)\n", __PRETTY_FUNCTION__, sdcClkCtrlCur & ~(SDHCI_CLOCK_INT_EN|SDHCI_CLOCK_INT_STABLE|SDHCI_CLOCK_CARD_EN), sdcClkCtrl, sdcClkDiv));

        bus->sdcb_IOWriteWord(SDHCI_CLOCK_CONTROL, (sdcClkCtrl | SDHCI_CLOCK_INT_EN), bus);

        timeout = 20;
        while (!((sdcClkCtrl = bus->sdcb_IOReadWord(SDHCI_CLOCK_CONTROL, bus)) & SDHCI_CLOCK_INT_STABLE)) {
            if (timeout == 0) {
                bug("[SDCard--] %s: SDHCI Clock failed to stabilise\n", __PRETTY_FUNCTION__);
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
    bus->sdcb_IOWriteWord(SDHCI_CLOCK_CONTROL, sdcClkCtrl, bus);
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

    lvlCur = bus->sdcb_IOReadByte(SDHCI_POWER_CONTROL, bus);
    if ((lvlCur & ~SDHCI_POWER_ON) != sdcReg)
    {
        D(bug("[SDCard--] %s: Changing Power Lvl [0x%x -> 0x%x]\n", __PRETTY_FUNCTION__, lvlCur & ~SDHCI_POWER_ON, sdcReg));
        bus->sdcb_IOWriteByte(SDHCI_POWER_CONTROL, sdcReg, bus);
        sdcReg |= SDHCI_POWER_ON;
        bus->sdcb_IOWriteByte(SDHCI_POWER_CONTROL, sdcReg, bus);
    }
    else
    {
        if (!(lvlCur & SDHCI_POWER_ON))
        {
            D(bug("[SDCard--] %s: Enabling Power Lvl [0x%x]\n", __PRETTY_FUNCTION__, lvlCur));
            lvlCur |= SDHCI_POWER_ON;
            bus->sdcb_IOWriteByte(SDHCI_POWER_CONTROL, lvlCur, bus);
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
    ULONG retVal = 0;

    if ((sdDataLen = GetTagData(SDCARD_TAG_DATALEN, 0, CmdTags)) > 0)
    {
        sdDataFlags = GetTagData(SDCARD_TAG_DATAFLAGS, 0, CmdTags);
    };

    /* Dont wait for DATA inihibit for stop commands */
    if (sdCommand != MMC_CMD_STOP_TRANSMISSION)
        sdcInhibitMask |= SDHCI_PS_DATA_INHIBIT;

    while (bus->sdcb_IOReadLong(SDHCI_PRESENT_STATE, bus) & sdcInhibitMask) {
        if (timeout == 0) {
            bug("[SDCard--] %s: Controller failed to release inhibited bit(s).\n", __PRETTY_FUNCTION__);
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
        DTRANS(bug("[SDCard--] %s: Configuring Data Transfer\n", __PRETTY_FUNCTION__));

        if (bus->sdcb_LEDCtrl)
            bus->sdcb_LEDCtrl(LED_ON);

        bus->sdcb_IOWriteByte(SDHCI_TIMEOUT_CONTROL, SDHCI_TIMEOUT_MAX, bus);

        bus->sdcb_IOWriteWord(SDHCI_BLOCK_SIZE, ((1 << 16) | ((sdDataLen > (1 << bus->sdcb_SectorShift)) ? (1 << bus->sdcb_SectorShift) : sdDataLen)), bus);
        if ((sdDataLen >> bus->sdcb_SectorShift) > 1)
        {
            sdcTransMode |= SDHCI_TRANSMOD_MULTI;
            bus->sdcb_IOWriteWord(SDHCI_BLOCK_COUNT, sdDataLen >> bus->sdcb_SectorShift, bus);
        }
        else
        {
            bus->sdcb_IOWriteWord(SDHCI_BLOCK_COUNT, 1, bus);
        }

        if (sdDataFlags == MMC_DATA_READ)
            sdcTransMode |= SDHCI_TRANSMOD_READ;

        if (!(bus->sdcb_Quirks & AF_Quirk_AtomicTMAndCMD))
        {
            bus->sdcb_IOWriteWord(SDHCI_TRANSFER_MODE, sdcTransMode, bus);
        }

        DTRANS(bug("[SDCard--] %s: Mode %08x [%d x %dBytes]\n", __PRETTY_FUNCTION__, sdcTransMode, (((sdDataLen >> bus->sdcb_SectorShift) > 0) ? (sdDataLen >> bus->sdcb_SectorShift) : 1), ((sdDataLen > (1 << bus->sdcb_SectorShift)) ? (1 << bus->sdcb_SectorShift) : sdDataLen)));
    }

#ifdef KernelBase
#undef KernelBase
#define KernelBase bus->sdcb_DeviceBase->sdcard_KernelBase
#endif
    KrnModifyIRQHandler(bus->sdcb_IRQHandle, bus, CmdTags);

    bus->sdcb_IOWriteLong(SDHCI_ARGUMENT, sdArg, bus);
    if ((bus->sdcb_Quirks & AF_Quirk_AtomicTMAndCMD) && (sdcTransMode))
    {
        bus->sdcb_IOWriteLong(SDHCI_TRANSFER_MODE, (sdcTransMode << 16) | SDHCI_MAKE_CMD(sdCommand, sdCommandFlags), bus);
    }
    else
    {
        bus->sdcb_IOWriteWord(SDHCI_COMMAND, SDHCI_MAKE_CMD(sdCommand, sdCommandFlags), bus);
    }

    D(bug("[SDCard--] %s: CMD %02d Sent\n", __PRETTY_FUNCTION__, sdCommand));

    return retVal;
}

ULONG FNAME_SDCBUS(FinishCmd)(struct TagItem *CmdTags, struct sdcard_Bus *bus)
{
    struct TagItem *Response = NULL;

    D(UWORD sdCommand      = (UWORD)GetTagData(SDCARD_TAG_CMD, 0, CmdTags));
    ULONG sdResponseType = GetTagData(SDCARD_TAG_RSPTYPE, MMC_RSP_NONE, CmdTags);
    ULONG retVal = 0;

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
                        ((ULONG *)Response->ti_Data)[i] = bus->sdcb_IOReadLong(SDHCI_RESPONSE + (3 - i) * 4, bus) << 8;
                        if (i != 3)
                            ((ULONG *)Response->ti_Data)[i] |= bus->sdcb_IOReadByte(SDHCI_RESPONSE + (3 - i) * 4 - 1, bus);
                    }
                    D(bug("[SDCard--] %s:   %08x%08x%08x%08x\n", __PRETTY_FUNCTION__, ((ULONG *)Response->ti_Data)[0], ((ULONG *)Response->ti_Data)[1], ((ULONG *)Response->ti_Data)[2], ((ULONG *)Response->ti_Data)[3]));
                }
            }
            else
            {
                Response->ti_Data = bus->sdcb_IOReadLong(SDHCI_RESPONSE, bus);
                D(bug("[= %08x]\n", Response->ti_Data));
            }
        }
    }

    return retVal;
}

ULONG FNAME_SDCBUS(FinishData)(struct TagItem *CmdTags, struct sdcard_Bus *bus)
{
    DTRANS(UWORD     sdCommand = (UWORD)GetTagData(SDCARD_TAG_CMD, 0, CmdTags));
    ULONG       sdcStateMask = SDHCI_PS_DATA_AVAILABLE | SDHCI_PS_SPACE_AVAILABLE,
                sdCommandMask = SDHCI_INT_SPACE_AVAIL | SDHCI_INT_DATA_AVAIL,
                sdData, sdDataLen = 0, sdStatus, sdcReg = 0;
    ULONG       timeout = 1000;
    ULONG       retVal = 0;

    if ((sdData = GetTagData(SDCARD_TAG_DATA, 0, CmdTags)) != 0)
    {
        sdDataLen = GetTagData(SDCARD_TAG_DATALEN, 0, CmdTags);
        if (!(sdDataLen))
            sdData = 0;
    };

    if (sdData)
    {
        DTRANS(bug("[SDCard--] %s: Transfering CMD %02d Data..\n", __PRETTY_FUNCTION__, sdCommand));
        do {
            sdStatus = bus->sdcb_IOReadLong(SDHCI_INT_STATUS, bus);
            if (sdStatus & SDHCI_INT_ERROR) {
                bug("[SDCard--] %s:    Error [status 0x%X]!\n", __PRETTY_FUNCTION__, sdStatus);
                retVal = -1;
                break;
            }
            if ((sdStatus & sdCommandMask) && (bus->sdcb_IOReadLong(SDHCI_PRESENT_STATE, bus) & sdcStateMask)) {
                ULONG currbyte, tranlen = (sdDataLen > (1 << bus->sdcb_SectorShift)) ? (1 << bus->sdcb_SectorShift) : sdDataLen;

                do
                {
                    DTRANS(bug("[SDCard--] %s: Attempting to read %dbytes\n", __PRETTY_FUNCTION__, tranlen));
                    for (currbyte = 0; currbyte < tranlen; currbyte++)
                    {
                        DTRANS(DUMP(
                            if ((currbyte % 16) == 0)
                            {
                                bug("[SDCard--] %s:    ", __PRETTY_FUNCTION__);
                            }
                        ))
                        if ((currbyte % 4) == 0)
                        {
                            sdcReg = bus->sdcb_IOReadLong(SDHCI_BUFFER, bus);
                        }
                        *(UBYTE *)sdData = sdcReg & 0xFF;
                        sdData++;
                        sdDataLen--;
                        sdcReg >>= 8;
                        DTRANS(DUMP(
                            if ((currbyte % 4) == 3)
                            {
                                bug(" %08x", *(ULONG *)(sdData - 4));
                            }
                            if ((currbyte % 16) == 15)
                            {
                                bug("\n");
                            }
                        ))
                    }
                    DTRANS(DUMP(
                        if ((currbyte % 16) != 15)
                        {
                            bug("\n");
                        }
                    ))
                } while (sdDataLen > 0);

                break;
            }
            else if (!(sdStatus & SDHCI_INT_DATA_END))
            {
                sdcard_Udelay(1000);

                if (timeout-- <= 0)
                {
                    bug("[SDCard--] %s:    Timeout!\n", __PRETTY_FUNCTION__);
                    retVal = -1;
                    break;
                }
            }
        } while (!(sdStatus & SDHCI_INT_DATA_END));

        if (bus->sdcb_LEDCtrl)
            bus->sdcb_LEDCtrl(LED_OFF);
    }

    return retVal;
}

ULONG FNAME_SDCBUS(WaitCmd)(ULONG mask, ULONG timeout, struct sdcard_Bus *bus)
{
    do {
        sdcard_Udelay(1000);
        if (bus->sdcb_BusStatus & SDHCI_INT_ERROR)
            break;
    } while (((bus->sdcb_BusStatus & mask) == mask) && (--timeout > 0));

    if ((timeout <= 0) || (bus->sdcb_BusStatus & SDHCI_INT_ERROR))
    {
        return -1;
    }

    return 0;
}

ULONG FNAME_SDCUNIT(WaitStatus)(ULONG timeout, struct sdcard_Unit *sdcUnit)
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
                bug("[SDCard%02ld] %s: Status [Error = %08x]\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, sdcStatusTags[3].ti_Data);
                return -1;
            }
        } else if (--retryreq < 0)
                return -1;

        sdcard_Udelay(1000);
    } while (--timeout > 0);

    if (timeout <= 0) {
        bug("[SDCard%02ld] %s: Timeout\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__);
        return -1;
    }

    D(bug("[SDCard%02ld] %s: State = %08x\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, sdcStatusTags[3].ti_Data & MMC_STATUS_STATE_MASK));

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

/********** BUS IRQ HANDLER **************/

void FNAME_SDCBUS(BusIRQ)(struct sdcard_Bus *bus, struct TagItem *IRQCommandTags)
{
    ULONG       sdcBusAckMask = 0;
    BOOL        error = FALSE;

    DIRQ(bug("[SDCard**] %s(bus: %u @ 0x%p)\n", __PRETTY_FUNCTION__, bus->sdcb_BusNum, bus));

    if (!(bus))
    {
        DIRQ(bug("[SDCard**] %s: Bad Params!\n", __PRETTY_FUNCTION__));
        return;
    }

    bus->sdcb_BusStatus = bus->sdcb_IOReadLong(SDHCI_INT_STATUS, bus);

    DIRQ(bug("[SDCard**] %s: Status = %08x\n", __PRETTY_FUNCTION__, bus->sdcb_BusStatus));

    if (!(bus->sdcb_BusStatus & SDHCI_INT_ERROR))
    {
        if (bus->sdcb_BusStatus & (SDHCI_INT_CARD_INSERT|SDHCI_INT_CARD_REMOVE))
        {
            bus->sdcb_BusFlags &= ~AF_Bus_MediaPresent;
            bus->sdcb_BusFlags |= AF_Bus_MediaChanged;

            if (bus->sdcb_BusStatus & SDHCI_INT_CARD_INSERT)
                bus->sdcb_BusFlags |= AF_Bus_MediaPresent;

            sdcBusAckMask |= (bus->sdcb_BusStatus & (SDHCI_INT_CARD_INSERT|SDHCI_INT_CARD_REMOVE));
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

            sdcBusAckMask |= (bus->sdcb_BusStatus & SDHCI_INT_CMD_MASK);
            bus->sdcb_BusStatus &= ~SDHCI_INT_CMD_MASK; 
        }
        if (bus->sdcb_BusStatus & SDHCI_INT_DATA_MASK)
        {
            if (IRQCommandTags)
            {
                if (bus->sdcb_BusStatus & (SDHCI_INT_DATA_END|SDHCI_INT_DMA_END|SDHCI_INT_DATA_AVAIL|SDHCI_INT_DATA_END_BIT))
                {
                    FNAME_SDCBUS(FinishData)(IRQCommandTags, bus);
                }
            }
            sdcBusAckMask |= (bus->sdcb_BusStatus & SDHCI_INT_DATA_MASK);
            bus->sdcb_BusStatus &= ~SDHCI_INT_DATA_MASK;
        }
        if (sdcBusAckMask)
        {
            bus->sdcb_IOWriteLong(SDHCI_INT_STATUS, sdcBusAckMask, bus);
        }
    }
    else
    {
        bug("[SDCard**] %s: ERROR\n", __PRETTY_FUNCTION__);
        if (bus->sdcb_BusStatus & SDHCI_INT_ACMD12ERR)
        {
            bug("[SDCard**] %s:       [acmd12err = %04x    ]\n", __PRETTY_FUNCTION__, bus->sdcb_IOReadWord(SDHCI_ACMD12_ERR, bus));
        }
        error = TRUE;
    }

    if (error)
    {
        if (bus->sdcb_BusStatus & bus->sdcb_IntrMask)
        {
            bug("[SDCard**] %s: Clearing Unhandled Interrupts [%08x]\n", __PRETTY_FUNCTION__, bus->sdcb_BusStatus & bus->sdcb_IntrMask);
            bus->sdcb_IOWriteLong(SDHCI_INT_STATUS, bus->sdcb_BusStatus & bus->sdcb_IntrMask, bus);
            bus->sdcb_BusStatus &= ~bus->sdcb_IntrMask;
        }
        bug("[SDCard**] %s: Reseting SDHCI CMD/DATA\n", __PRETTY_FUNCTION__);

        FNAME_SDCBUS(SoftReset)(SDHCI_RESET_CMD, bus);
        FNAME_SDCBUS(SoftReset)(SDHCI_RESET_DATA, bus);
    }
    DIRQ(bug("[SDCard**] %s: Done.\n", __PRETTY_FUNCTION__));
}

/********** BUS MANAGEMENT TASK **************/

void FNAME_SDCBUS(BusTask)(struct sdcard_Bus *bus)
{
    LIBBASETYPEPTR LIBBASE = bus->sdcb_DeviceBase;
    struct IORequest *msg;
    ULONG sdcReg;
    ULONG sig;

    D(bug("[SDCard**] Task started (bus: %u)\n", bus->sdcb_BusNum));

    bus->sdcb_Timer = sdcard_OpenTimer(LIBBASE);

    /* Get the signal used for sleeping */
    bus->sdcb_Task = FindTask(0);
    bus->sdcb_TaskSig = AllocSignal(-1);
    /* Failed to get it? Use SIGBREAKB_CTRL_E instead */
    if (bus->sdcb_TaskSig < 0)
        bus->sdcb_TaskSig = SIGBREAKB_CTRL_E;

    sig = 1L << bus->sdcb_MsgPort->mp_SigBit;

    sdcReg = bus->sdcb_IOReadLong(SDHCI_PRESENT_STATE, bus);
    if (sdcReg & SDHCI_PS_CARD_PRESENT)
    {
        FNAME_SDCBUS(RegisterUnit)(bus);
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
