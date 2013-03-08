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
#include <proto/vcmbox.h>

#include <asm/bcm2835.h>
#include <hardware/arasan.h>
#include <hardware/videocore.h>
#include <hardware/mmc.h>
#include <hardware/sdhc.h>

#include <string.h>

#include "sdcard_intern.h"
#include "timer.h"

#include LC_LIBDEFS_FILE

#define VCMB_PROPCHAN                     8
#define VCMBoxBase                      SDCardBase->sdcard_VCMBoxBase

unsigned int        VCMBoxMessage[8] __attribute__((used,aligned(16)));

BOOL FNAME_SDC(RegisterVolume)(struct sdcard_Bus *bus)
{
    struct SDCardBase     *SDCardBase = bus->sdcb_DeviceBase;
    struct sdcard_Unit    *sdcUnit = NULL;
    unsigned int        sdcCardPower, sdcReg, timeout = 1000000, timeout_udelay = 2000;
    struct DeviceNode   *devnode;
    BOOL sdcHighCap = FALSE;
    struct TagItem sdcRegTags[] =
    {
        {SDCARD_TAG_CMD,         0},
        {SDCARD_TAG_ARG,         0},
        {SDCARD_TAG_RSPTYPE,     0},
        {SDCARD_TAG_RSP,         0},
        {TAG_DONE,              0}
    };

    D(bug("[SDCard>>] %s()\n", __PRETTY_FUNCTION__));

    if ((ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 40L)) != NULL)
    {
        IPTR pp[24];
        ULONG response136[4] = {0, 0, 0, 0};

        D(bug("[SDCard>>] %s: Telling controller to idle...\n", __PRETTY_FUNCTION__));
        sdcRegTags[0].ti_Data = MMC_CMD_GO_IDLE_STATE;
        sdcRegTags[1].ti_Data = 0;
        sdcRegTags[2].ti_Data = MMC_RSP_NONE;
        if (FNAME_SDCBUS(SendCmd)(sdcRegTags, bus) == -1)
        {
            D(bug("[SDCard>>] %s: Failed to go idle?\n", __PRETTY_FUNCTION__));
            CloseLibrary((struct Library *)ExpansionBase);
            return FALSE;
        }

        for (; timeout_udelay > 0; timeout_udelay --) asm volatile("mov r0, r0\n");

        sdcRegTags[0].ti_Data = SD_CMD_SEND_IF_COND;
        sdcRegTags[1].ti_Data = ((bus->sdcb_Power & 0xFF8000) != 0) << 8 | 0xAA;
        sdcRegTags[2].ti_Data = MMC_RSP_R7;
        D(bug("[SDCard>>] %s: Querying Interface conditions [%08x] ... ", __PRETTY_FUNCTION__, sdcRegTags[1].ti_Data));

        if ((FNAME_SDCBUS(SendCmd)(sdcRegTags, bus)) != -1)
        {
            D(bug("response %08x\n", sdcRegTags[3].ti_Data));

            do {
                D(bug("[SDCard>>] %s: Attempt to send an App Command ... ", __PRETTY_FUNCTION__));
                sdcRegTags[0].ti_Data = MMC_CMD_APP_CMD;
                sdcRegTags[1].ti_Data = 0;
                sdcRegTags[2].ti_Data = MMC_RSP_R1;

                if (FNAME_SDCBUS(SendCmd)(sdcRegTags, bus) == -1)
                {
                    D(bug("error (-1)\n"));
                    CloseLibrary((struct Library *)ExpansionBase);
                    return FALSE;
                }
                D(bug("response %08x\n", sdcRegTags[3].ti_Data));

                sdcRegTags[0].ti_Data = SD_CMD_APP_SEND_OP_COND;
                sdcRegTags[1].ti_Data = (bus->sdcb_Power & 0xFF8000) | OCR_HCS;
                sdcRegTags[2].ti_Data = MMC_RSP_R3;

                D(bug("[SDCard>>] %s: Querying Operating conditions [%08x] ... ", __PRETTY_FUNCTION__, sdcRegTags[1].ti_Data));
                if (FNAME_SDCBUS(SendCmd)(sdcRegTags, bus) != -1)
                {
                    D(bug("response %08x\n", sdcRegTags[3].ti_Data));
                    timeout_udelay = timeout - 1000;
                    for (; timeout > timeout_udelay; timeout --) asm volatile("mov r0, r0\n");
                }
                else
                {
                    D(bug("error (-1)\n"));
                    CloseLibrary((struct Library *)ExpansionBase);
                    return FALSE;
                }
            } while ((!(sdcRegTags[3].ti_Data & OCR_BUSY)) && timeout--);

            if (timeout > 0)
            {
                D(bug("[SDCard>>] %s: Card OCR = %08x\n", __PRETTY_FUNCTION__, (sdcRegTags[3].ti_Data & 0xFFFF00)));

                sdcCardPower = LIBBASE->sdcard_Bus->sdcb_Power & (sdcRegTags[3].ti_Data & 0xFFFF00);

                if (sdcRegTags[3].ti_Data & OCR_HCS)
                    sdcHighCap = TRUE;

                FNAME_SDCBUS(SetPowerLevel)(sdcCardPower, bus);

                D(bug("[SDCard>>] %s: Card is now operating in Identification Mode\n", __PRETTY_FUNCTION__));

                /* Put the "card" into identify mode*/ 
                D(bug("[SDCard>>] %s: Querying Card Identification Data ...\n", __PRETTY_FUNCTION__));
                sdcRegTags[0].ti_Data = MMC_CMD_ALL_SEND_CID;
                sdcRegTags[1].ti_Data = 0;
                sdcRegTags[2].ti_Data = MMC_RSP_R2;
                sdcRegTags[3].ti_Data = response136;
                if (FNAME_SDCBUS(SendCmd)(sdcRegTags, bus) != -1)
                {
                    if (sdcRegTags[3].ti_Data)
                    {
                        D(bug("[SDCard>>] %s: %08x%08x%08x%08x\n", __PRETTY_FUNCTION__, response136[0], response136[1], response136[2], response136[3]));
                        D(bug("[SDCard>>] %s:   Card Identification Data (CID) Register\n", __PRETTY_FUNCTION__));
                        D(bug("[SDCard>>] %s:   ======================================\n", __PRETTY_FUNCTION__));
                        D(bug("[SDCard>>] %s:           Manuafacturer ID (MID) : %06x\n", __PRETTY_FUNCTION__, FNAME_SDCBUS(Rsp136Unpack)(response136, 120, 8)));
                        D(bug("[SDCard>>] %s:           Product Name (PNM) : %c%c%c%c%c\n", __PRETTY_FUNCTION__, FNAME_SDCBUS(Rsp136Unpack)(response136, 96, 8), FNAME_SDCBUS(Rsp136Unpack)(response136, 88, 8), FNAME_SDCBUS(Rsp136Unpack)(response136, 80, 8), FNAME_SDCBUS(Rsp136Unpack)(response136, 72, 8), FNAME_SDCBUS(Rsp136Unpack)(response136, 64, 8)));
                        D(bug("[SDCard>>] %s:           Product Revision (PRV) : %d.%d\n", __PRETTY_FUNCTION__, FNAME_SDCBUS(Rsp136Unpack)(response136, 60, 4), FNAME_SDCBUS(Rsp136Unpack)(response136, 56, 4)));
                        D(bug("[SDCard>>] %s:           Serial number (PSN) : %08x\n", __PRETTY_FUNCTION__,  FNAME_SDCBUS(Rsp136Unpack)(response136, 24, 32)));
                        D(bug("[SDCard>>] %s:           Manufacturing Date Code (MDT) : %d/%d\n", __PRETTY_FUNCTION__, FNAME_SDCBUS(Rsp136Unpack)(response136, 8, 4), FNAME_SDCBUS(Rsp136Unpack)(response136, 12, 8)));
                        D(bug("[SDCard>>] %s:           CRC7 checksum (CRC7) : %x\n", __PRETTY_FUNCTION__, FNAME_SDCBUS(Rsp136Unpack)(response136, 1, 7)));
                        D(bug("[SDCard>>] %s:           Reserved : %x\n", __PRETTY_FUNCTION__, FNAME_SDCBUS(Rsp136Unpack)(response136, 0, 1)));
                    }

                    D(bug("[SDCard>>] %s: Querying Card Relative Address... ", __PRETTY_FUNCTION__));
                    sdcRegTags[0].ti_Data = SD_CMD_SEND_RELATIVE_ADDR;
                    sdcRegTags[1].ti_Data = 0;
                    sdcRegTags[2].ti_Data = MMC_RSP_R6;
                    sdcRegTags[3].ti_Data = NULL;
                    if (FNAME_SDCBUS(SendCmd)(sdcRegTags, bus) != -1)
                    {
                        if ((sdcUnit = AllocVecPooled(SDCardBase->sdcard_MemPool, sizeof(struct sdcard_Unit))) != NULL)
                        {
                            sdcUnit->sdcu_Bus = bus;
                            sdcUnit->sdcu_UnitNum = bus->sdcb_UnitCnt++;
                            bus->sdcb_Units[sdcUnit->sdcu_UnitNum] = sdcUnit;
                            sdcUnit->sdcu_CardRCA = (sdcRegTags[3].ti_Data >> 16) & 0xFFFF;
                            sdcUnit->sdcu_CardPower = sdcCardPower;

                            D(bug("[RCA %d]\n", sdcUnit->sdcu_CardRCA));

                            sdcRegTags[0].ti_Data = MMC_CMD_SEND_CSD;
                            sdcRegTags[1].ti_Data = sdcUnit->sdcu_CardRCA << 16;
                            sdcRegTags[2].ti_Data = MMC_RSP_R2;
                            sdcRegTags[3].ti_Data = response136;
                            D(bug("[SDCard%02ld] %s: Querying Card Specific Data [%08x] ...\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, sdcRegTags[1].ti_Data));
                            if (FNAME_SDCBUS(SendCmd)(sdcRegTags, bus) != -1)
                            {
                                if (sdcRegTags[3].ti_Data)
                                {
                                    int __csdstruct = FNAME_SDCBUS(Rsp136Unpack)(response136, 126, 2);
                                    D(bug("[SDCard%02ld] %s: %08x%08x%08x%08x\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, response136[0], response136[1], response136[2], response136[3]));
                                    D(bug("[SDCard%02ld] %s:   Card Specific Data (CSD) Register\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
                                    D(bug("[SDCard%02ld] %s:   =================================\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
                                    D(bug("[SDCard%02ld] %s:           CSD_STRUCTURE : %x ", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, __csdstruct));

                                    sdcUnit->sdcu_Read32        = FNAME_SDCIO(ReadSector32);
                                    sdcUnit->sdcu_Write32       = FNAME_SDCIO(WriteSector32);
                                    sdcUnit->sdcu_Flags         = AF_MediaPresent;
                                    if (sdcHighCap)
                                        sdcUnit->sdcu_Flags     |= AF_HighCapacity;

                                    switch (__csdstruct)
                                    {
                                        case 0:
                                        {
                                            D(bug("[SDSC Card]\n"));
                                            pp[DE_SIZEBLOCK + 4] = 2 << (FNAME_SDCBUS(Rsp136Unpack)(response136, 80, 4) - 1);
                                            pp[DE_SECSPERBLOCK + 4] = pp[DE_SIZEBLOCK + 4] >> 9;
                                            pp[DE_HIGHCYL + 4] = ((1 + FNAME_SDCBUS(Rsp136Unpack)(response136, 62, 12)) << (FNAME_SDCBUS(Rsp136Unpack)(response136, 47, 3) + 2));
                                            break;
                                        }
                                        case 1:
                                        {
                                            D(bug("[SDHC/XC Card]\n"));
                                            
                                            pp[DE_SECSPERBLOCK + 4] = 2;
                                            pp[DE_SIZEBLOCK + 4] = 2 << (10 - 1);
                                            pp[DE_HIGHCYL + 4] = ((1 + FNAME_SDCBUS(Rsp136Unpack)(response136, 48, 22)) * (2 << (9 - 1)));

                                            sdcUnit->sdcu_Flags         |= AF_MMC;

                                            sdcUnit->sdcu_Read64        = FNAME_SDCIO(ReadSector64);
                                            sdcUnit->sdcu_Write64       = FNAME_SDCIO(WriteSector64);
                                            break;
                                        }
                                        default:
                                            D(bug("[SDCard%02ld] %s: Unsupported Card\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
                                            CloseLibrary((struct Library *)ExpansionBase);
                                            return FALSE;
                                    }

                                    sdcUnit->sdcu_Cylinders     = pp[DE_HIGHCYL + 4];
                                    sdcUnit->sdcu_Heads         = 1;
                                    sdcUnit->sdcu_Sectors       = pp[DE_SECSPERBLOCK + 4];
                                    sdcUnit->sdcu_Capacity      = sdcUnit->sdcu_Cylinders * sdcUnit->sdcu_Heads * sdcUnit->sdcu_Sectors;

                                    sdcUnit->sdcu_Eject         = FNAME_SDCIO(Eject);

                                    D(bug("[SDCard%02ld] %s:           READ_BL_LEN : %dbytes\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, pp[DE_SIZEBLOCK + 4] / sdcUnit->sdcu_Sectors));
                                    D(bug("[SDCard%02ld] %s:           C_SIZE : %d\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, sdcUnit->sdcu_Cylinders));

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
                                        D(bug("[SDCard%02ld] %s: %b: [%dMB Capacity]\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__, devnode->dn_Name, sdcUnit->sdcu_Capacity >> 11));
                                        D(bug("[SDCard%02ld] %s:        StartCyl:%d, EndCyl:%d ..\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__,
                                              pp[DE_LOWCYL + 4], pp[DE_HIGHCYL + 4]));
                                        D(bug("[SDCard%02ld] %s:        BlockSize:%d, SectorsPerBlock:%d ..\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__,
                                              pp[DE_SIZEBLOCK + 4], sdcUnit->sdcu_Sectors));

                                        FNAME_SDCBUS(SetClock)(25000000, sdcUnit->sdcu_Bus);

                                        AddBootNode(pp[DE_BOOTPRI + 4], ADNF_STARTPROC, devnode, NULL);
                                        D(bug("[SDCard%02ld] %s: done\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));

                                        return TRUE;
                                    }
                                }
                            }
                            else
                            {
                                D(bug("[SDCard%02ld] %s: returned error (-1)\n", sdcUnit->sdcu_UnitNum, __PRETTY_FUNCTION__));
                            }
                        }
                        else
                        {
                            D(bug("[SDCard>>] %s: Failed to allocate Unit #%d\n", __PRETTY_FUNCTION__, bus->sdcb_UnitCnt));
                        }
                    }
                    else
                    {
                        D(bug("error (-1)\n"));
                    }
                }
                else
                {
                    D(bug("[SDCard>>] %s: returned error (-1)\n", __PRETTY_FUNCTION__));
                }
            }
            else
            {
                D(bug("[SDCard>>] %s: Failed to set card operating conditions\n", __PRETTY_FUNCTION__));
            }
        }
        else
        {
            D(bug("[SDCard>>] %s: returned error (-1)\n", __PRETTY_FUNCTION__));
        }
        CloseLibrary((struct Library *)ExpansionBase);
    }
    else
    {
    }

    return FALSE;
}

#define VCPOWER_SDHCI		0
#define VCPOWER_STATE_ON	(1 << 0)
#define VCPOWER_STATE_WAIT	(1 << 1)
#define VCCLOCK_SDHCI           1

/*
 * This init routine has +127 priority, so it runs after all
 * bus scanners have done their job.
 * It initializes all discovered units.
 */
static int FNAME_SDC(Scan)(struct SDCardBase *SDCardBase)
{
    unsigned int        sdcReg;

    D(bug("[SDCard--] %s()\n", __PRETTY_FUNCTION__));

    *(volatile unsigned int *)GPSET0 = (1 << 16); // Turn Activity LED OFF

    SDCardBase->sdcard_Bus->sdcb_IntrMask = SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
	    SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_INDEX |
	    SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
	    SDHCI_INT_CARD_REMOVE | SDHCI_INT_CARD_INSERT |
	    SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL |
	    SDHCI_INT_DATA_END | SDHCI_INT_RESPONSE;

    FNAME_SDCBUS(SetClock)(HOSTCLOCK_MIN, SDCardBase->sdcard_Bus);

    FNAME_SDCBUS(SetPowerLevel)(SDCardBase->sdcard_Bus->sdcb_Power, SDCardBase->sdcard_Bus);

    D(bug("[SDCard--] %s: Setting Min Buswidth...\n", __PRETTY_FUNCTION__));
    sdcReg = FNAME_SDCBUS(MMIOReadByte)(SDHCI_HOST_CONTROL, SDCardBase->sdcard_Bus);
    sdcReg &= ~(SDHCI_HCTRL_4BITBUS|SDHCI_HCTRL_HISPD);
    FNAME_SDCBUS(MMIOWriteByte)(SDHCI_HOST_CONTROL, sdcReg, SDCardBase->sdcard_Bus);

    D(bug("[SDCard--] %s: Masking chipset Interrupts...\n", __PRETTY_FUNCTION__));

    FNAME_SDCBUS(MMIOWriteLong)(SDHCI_INT_ENABLE, SDCardBase->sdcard_Bus->sdcb_IntrMask, SDCardBase->sdcard_Bus);
    FNAME_SDCBUS(MMIOWriteLong)(SDHCI_SIGNAL_ENABLE, SDCardBase->sdcard_Bus->sdcb_IntrMask, SDCardBase->sdcard_Bus);

    D(bug("[SDCard--] %s: Launching Bus Task...\n", __PRETTY_FUNCTION__));

    return NewCreateTask(
        TASKTAG_PC         , FNAME_SDCBUS(BusTask),
        TASKTAG_NAME       , "SDCard Subsystem",
        TASKTAG_STACKSIZE  , STACK_SIZE,
        TASKTAG_PRI        , TASK_PRI,
        TASKTAG_TASKMSGPORT, &SDCardBase->sdcard_Bus->sdcb_MsgPort,
        TASKTAG_ARG1       , SDCardBase->sdcard_Bus,
        TAG_DONE) ? TRUE : FALSE;
}

static int FNAME_SDC(Init)(struct SDCardBase *SDCardBase)
{
    D(bug("[SDCard--] %s()\n", __PRETTY_FUNCTION__));

    if ((VCMBoxBase = OpenResource("vcmbox.resource")) == NULL)
    {
        D(bug("[SDCard--] %s: Failed to open vcmbox.resource\n", __PRETTY_FUNCTION__));
        return FALSE;
    }

    LIBBASE->sdcard_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED , 8192, 4096);
    if (LIBBASE->sdcard_MemPool == NULL)
    {
        D(bug("[SDCard--] %s: Failed to Allocate MemPool\n", __PRETTY_FUNCTION__));
        return FALSE;
    }

    D(bug("[SDCard--] %s: MemPool @ %p\n", __PRETTY_FUNCTION__, LIBBASE->sdcard_MemPool));

#if (0)
    VCMBoxMessage[0] = 8 * 4;
    VCMBoxMessage[1] = VCTAG_REQ;
    VCMBoxMessage[2] = VCTAG_GETPOWER;
    VCMBoxMessage[3] = 8;
    VCMBoxMessage[4] = 4;
    VCMBoxMessage[5] = VCPOWER_SDHCI;
    VCMBoxMessage[6] = 0;

    VCMBoxMessage[7] = 0; // terminate tag

    VCMBoxWrite(VCMB_BASE, VCMB_PROPCHAN, VCMBoxMessage);
    if (VCMBoxRead(VCMB_BASE, VCMB_PROPCHAN) != VCMBoxMessage)
    {
        D(bug("[SDCard--] %s: Failed to read controller's Power state\n", __PRETTY_FUNCTION__));
        return FALSE;
    }
    
    if (!(VCMBoxMessage[6] & VCPOWER_STATE_ON))
    {
        D(bug("[SDCard--] %s: Powering on Arasan SDHCI controller...\n", __PRETTY_FUNCTION__));

        VCMBoxMessage[0] = 8 * 4;
        VCMBoxMessage[1] = VCTAG_REQ;
        VCMBoxMessage[2] = VCTAG_SETPOWER;
        VCMBoxMessage[3] = 8;
        VCMBoxMessage[4] = 8;
        VCMBoxMessage[5] = VCPOWER_SDHCI;
        VCMBoxMessage[6] = VCPOWER_STATE_ON | VCPOWER_STATE_WAIT;

        VCMBoxMessage[7] = 0; // terminate tag

        VCMBoxWrite(VCMB_BASE, VCMB_PROPCHAN, VCMBoxMessage);
        if ((VCMBoxRead(VCMB_BASE, VCMB_PROPCHAN) != VCMBoxMessage) || (!(VCMBoxMessage[6] & VCPOWER_STATE_ON)))
        {
            D(bug("[SDCard--] %s: Failed to power on controller\n", __PRETTY_FUNCTION__));
            return FALSE;
        }
    }
#endif

    VCMBoxMessage[0] = 8 * 4;
    VCMBoxMessage[1] = VCTAG_REQ;
    VCMBoxMessage[2] = VCTAG_GETCLKRATE;
    VCMBoxMessage[3] = 8;
    VCMBoxMessage[4] = 4;
    VCMBoxMessage[5] = VCCLOCK_SDHCI;
    VCMBoxMessage[6] = 0;

    VCMBoxMessage[7] = 0; // terminate tag

    VCMBoxWrite(VCMB_BASE, VCMB_PROPCHAN, VCMBoxMessage);
    if (VCMBoxRead(VCMB_BASE, VCMB_PROPCHAN) != VCMBoxMessage)
    {
        D(bug("[SDCard--] %s: Failed to determine Max SDHC Clock\n", __PRETTY_FUNCTION__));
        return FALSE;
    }

    if ((LIBBASE->sdcard_Bus = AllocPooled(LIBBASE->sdcard_MemPool, sizeof(struct sdcard_Bus))) != NULL)
    {
        LIBBASE->sdcard_Bus->sdcb_DeviceBase = LIBBASE;
        LIBBASE->sdcard_Bus->sdcb_IOBase = ARASAN_BASE;
        LIBBASE->sdcard_Bus->sdcb_SectorShift = 9;

        LIBBASE->sdcard_Bus->sdcb_ClockMax = VCMBoxMessage[6];

        D(bug("[SDCard--] %s: Reseting SDHCI...\n", __PRETTY_FUNCTION__));

        FNAME_SDCBUS(SoftReset)(SDHCI_RESET_ALL, LIBBASE->sdcard_Bus);

        D(bug("[SDCard--] %s: SDHC Max Clock Rate : %dMHz\n", __PRETTY_FUNCTION__, LIBBASE->sdcard_Bus->sdcb_ClockMax / 1000000));
        D(bug("[SDCard--] %s: SDHC Min Clock Rate : %dHz (hardcoded)\n", __PRETTY_FUNCTION__, HOSTCLOCK_MIN));

        LIBBASE->sdcard_Bus->sdcb_Version = FNAME_SDCBUS(MMIOReadWord)(SDHCI_HOST_VERSION, LIBBASE->sdcard_Bus);
        LIBBASE->sdcard_Bus->sdcb_Capabilities = FNAME_SDCBUS(MMIOReadLong)(SDHCI_CAPABILITIES, LIBBASE->sdcard_Bus);
        LIBBASE->sdcard_Bus->sdcb_Power = MMC_VDD_165_195 | MMC_VDD_320_330 | MMC_VDD_330_340;

        D(bug("[SDCard--] %s: SDHCI Host Vers      : %d [SD Host Spec %d]\n", __PRETTY_FUNCTION__, ((LIBBASE->sdcard_Bus->sdcb_Version & 0xFF00) >> 8), (LIBBASE->sdcard_Bus->sdcb_Version & 0xFF) + 1));
        D(bug("[SDCard--] %s: SDHCI Capabilities   : 0x%08x\n", __PRETTY_FUNCTION__, LIBBASE->sdcard_Bus->sdcb_Capabilities));
        D(bug("[SDCard--] %s: SDHCI Voltages       : 0x%08x (hardcoded)\n", __PRETTY_FUNCTION__, LIBBASE->sdcard_Bus->sdcb_Power));

        LIBBASE->sdcard_Bus->sdcb_LastWrite = *((volatile unsigned int *)(SYSTIMER_CLO));
    }
    return TRUE;
}

static int FNAME_SDC(Open)
(
    LIBBASETYPEPTR LIBBASE,
    struct IORequest *iorq,
    ULONG unitnum,
    ULONG flags
)
{
    unsigned int        sdcReg = 0, sdcClkDiv, timeout = 10000, timeout_udelay;

    D(bug("[SDCard--] %s()\n", __PRETTY_FUNCTION__));

    /* Assume it failed */
    iorq->io_Error = IOERR_OPENFAIL;

    if ((unitnum < LIBBASE->sdcard_Bus->sdcb_UnitCnt) && (LIBBASE->sdcard_Bus->sdcb_Units[unitnum]))
    {
        struct TagItem sdcOpenTags[] =
        {
            {SDCARD_TAG_CMD,         MMC_CMD_SELECT_CARD},
            {SDCARD_TAG_ARG,         LIBBASE->sdcard_Bus->sdcb_Units[unitnum]->sdcu_CardRCA << 16},
            {SDCARD_TAG_RSPTYPE,     MMC_RSP_R1},
            {SDCARD_TAG_RSP,         0},
            {TAG_DONE,               0}
        };
        if (FNAME_SDCBUS(SendCmd)(sdcOpenTags, LIBBASE->sdcard_Bus) != -1)
        {
            iorq->io_Unit = &LIBBASE->sdcard_Bus->sdcb_Units[unitnum]->sdcu_Unit;
            ((struct sdcard_Unit *)iorq->io_Unit)->sdcu_Unit.unit_OpenCnt++;

            D(bug("[SDCard%02ld] %s: Selected card with RCA %d\n", unitnum, __PRETTY_FUNCTION__, LIBBASE->sdcard_Bus->sdcb_Units[unitnum]->sdcu_CardRCA));
            D(bug("[SDCard%02ld] %s: Card is now operating in Transfer Mode\n", unitnum, __PRETTY_FUNCTION__));

            if (!(((struct sdcard_Unit *)iorq->io_Unit)->sdcu_Flags & AF_HighCapacity))
            {
                sdcOpenTags[0].ti_Data = MMC_CMD_SET_BLOCKLEN;
                sdcOpenTags[1].ti_Data = 1 << ((struct sdcard_Unit *)iorq->io_Unit)->sdcu_Bus->sdcb_SectorShift;
                sdcOpenTags[2].ti_Data = MMC_RSP_R1;
                sdcOpenTags[3].ti_Data = 0;
                if (FNAME_SDCBUS(SendCmd)(sdcOpenTags, LIBBASE->sdcard_Bus) != -1)
                {
                    D(bug("[SDCard%02ld] %s: Blocklen set to %d\n", unitnum, __PRETTY_FUNCTION__, sdcOpenTags[1].ti_Data));
                }
                else
                {
                    D(bug("[SDCard%02ld] %s: Failed to change Blocklen\n", unitnum, __PRETTY_FUNCTION__));
                }
            }

            if (((struct sdcard_Unit *)iorq->io_Unit)->sdcu_Flags & AF_MMC)
            {
            }
            else
            {
                FNAME_SDCBUS(SDSCChangeFrequency)((struct sdcard_Unit *)iorq->io_Unit);
            }

            iorq->io_Error = 0;
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

    D(bug("[SDCard--] %s()\n", __PRETTY_FUNCTION__));

    /* First of all make the important fields of struct IORequest invalid! */
    iorq->io_Unit = (struct Unit *)~0;
    
    /* Decrease use counters of unit */
    unit->sdcu_Unit.unit_OpenCnt--;

    return TRUE;
}

ADD2INITLIB(FNAME_SDC(Init), 0)
ADD2INITLIB(FNAME_SDC(Scan), 127)
ADD2OPENDEV(FNAME_SDC(Open), 0)
ADD2CLOSEDEV(FNAME_SDC(Close), 0)
