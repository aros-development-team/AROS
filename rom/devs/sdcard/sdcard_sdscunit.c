/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <hardware/mmc.h>
#include <hardware/sdhc.h>

#include "sdcard_base.h"
#include "sdcard_bus.h"
#include "sdcard_unit.h"

ULONG FNAME_SDCUNIT(SDSCSwitch)(BOOL test, int group, UBYTE value, APTR buf, struct sdcard_Unit *sdcUnit)
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

ULONG FNAME_SDCUNIT(SDSCChangeFrequency)(struct sdcard_Unit *sdcUnit)
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
            if (FNAME_SDCUNIT(SDSCSwitch)(TRUE, 0, 1, sdcRespBuf, sdcUnit) != -1)
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

            if (FNAME_SDCUNIT(SDSCSwitch)(FALSE, 0, 1, sdcRespBuf, sdcUnit) != -1)
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
