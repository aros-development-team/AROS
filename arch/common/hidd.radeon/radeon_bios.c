/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ATI radeon driver. BIOS part.
    Lang: English
*/

/*
 * This file is a derived version of the XFree86 ATI driver (radeon_bios.c file)
 */

#include <exec/types.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/graphics.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

//#include <asm/io.h>

#include "ati.h"
#include "radeon.h"
#include "radeon_reg.h"
#include "radeon_macros.h"
#include "radeon_bios.h"

#define DEBUG 1 
#include <aros/debug.h>

#undef HiddPCIDeviceAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#define SysBase         (sd->sysbase)
#define HiddPCIDeviceAttrBase   (sd->pciAttrBase)
#define HiddATIBitMapAttrBase   (sd->atiBitMapAttrBase)
#define HiddBitMapAttrBase  (sd->bitMapAttrBase)
#define HiddPixFmtAttrBase  (sd->pixFmtAttrBase)
#define HiddGfxAttrBase     (sd->gfxAttrBase)
#define HiddSyncAttrBase    (sd->syncAttrBase)

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b) ((a)>(b)?(b):(a))
#endif


/* Read the Video BIOS block and the FP registers (if applicable). */
BOOL RADEONGetBIOSInfo(struct ati_staticdata *sd)
{
    int tmp;

    if (sd->Card.VBIOS[0] != 0x55 || sd->Card.VBIOS[1] != 0xaa) {
        D(bug("[ATI] Video BIOS not detected in PCI space!\n"));
        D(bug("[ATI] Attempting to read Video BIOS from legacy ISA space!\n"));
        sd->Card.VBIOS = AllocPooled(sd->memPool, 65536);
        CopyMemQuick((APTR)0x000c0000, sd->Card.VBIOS, 65536);
    }

    if (sd->Card.VBIOS[0] != 0x55 || sd->Card.VBIOS[1] != 0xaa) {
        D(bug("[ATI] Unrecognized BIOS signature, BIOS data will not be used\n"));
        if (sd->Card.VBIOS != sd->Card.vbios_org) 
            FreePooled(sd->memPool, sd->Card.VBIOS, 65536);
        sd->Card.VBIOS = NULL;
        return FALSE;
    }

    if (sd->Card.VBIOS) sd->Card.ROMHeaderStart = RADEON_BIOS16(0x48);

    if(!sd->Card.ROMHeaderStart) {
        D(bug("[ATI] Invalid ROM pointer, BIOS data will not be used\n"));
        if (sd->Card.VBIOS != sd->Card.vbios_org) 
            FreePooled(sd->memPool, sd->Card.VBIOS, 65536);
        sd->Card.VBIOS = NULL;
        return FALSE;
    }

    tmp = sd->Card.ROMHeaderStart + 4;
    if ((RADEON_BIOS8(tmp)   == 'A' &&
         RADEON_BIOS8(tmp+1) == 'T' &&
         RADEON_BIOS8(tmp+2) == 'O' &&
         RADEON_BIOS8(tmp+3) == 'M') ||
        (RADEON_BIOS8(tmp)   == 'M' &&
         RADEON_BIOS8(tmp+1) == 'O' &&
         RADEON_BIOS8(tmp+2) == 'T' &&
         RADEON_BIOS8(tmp+3) == 'A'))
        sd->Card.IsAtomBios = TRUE;
    else
        sd->Card.IsAtomBios = FALSE;

    if (sd->Card.IsAtomBios)
        sd->Card.MasterDataStart = RADEON_BIOS16 (sd->Card.ROMHeaderStart + 32);

    D(bug("[ATI] %s BIOS detected\n", sd->Card.IsAtomBios ? "ATOM":"Legacy"));

    return TRUE;
}

BOOL RADEONGetConnectorInfoFromBIOS(struct ati_staticdata *sd)
{
    int i = 0, j, tmp, tmp0=0, tmp1=0;

    if(!sd->Card.VBIOS) return FALSE;

    if (sd->Card.IsAtomBios) {
        if((tmp = RADEON_BIOS16 (sd->Card.MasterDataStart + 22))) {
            int crtc = 0, id[2];
            tmp1 = RADEON_BIOS16 (tmp + 4);
            for (i=0; i<8; i++) {
                if(tmp1 & (1<<i)) {
                    UWORD portinfo = RADEON_BIOS16(tmp+6+i*2);
                    if (crtc < 2) {
                        if ((i==2) || (i==6)) continue; /* ignore TV here */

                        if (crtc == 1) {
                            /* sharing same port with id[0] */
                            if (((portinfo>>8) & 0xf) == id[0]) {
                                if (i == 3)
                                    sd->Card.PortInfo[0].TMDSType = TMDS_INT;
                                else if (i == 7)
                                    sd->Card.PortInfo[0].TMDSType = TMDS_EXT;

                                if (sd->Card.PortInfo[0].DACType == DAC_UNKNOWN)
                                    sd->Card.PortInfo[0].DACType = (portinfo & 0xf) - 1;
                                continue;
                            }
                        }
                        id[crtc] = (portinfo>>8) & 0xf;
                        sd->Card.PortInfo[crtc].DACType = (portinfo & 0xf) - 1;
                        sd->Card.PortInfo[crtc].ConnectorType = (portinfo>>4) & 0xf;
                        if (i == 3)
                            sd->Card.PortInfo[crtc].TMDSType = TMDS_INT;
                        else if (i == 7)
                            sd->Card.PortInfo[crtc].TMDSType = TMDS_EXT;

                        if((tmp0 = RADEON_BIOS16 (sd->Card.MasterDataStart + 24)) && id[crtc]) {
                            switch (RADEON_BIOS16 (tmp0 + 4 + 27 * id[crtc]) * 4)
                            {
                            case RADEON_GPIO_MONID:
                                sd->Card.PortInfo[crtc].DDCType = DDC_MONID;
                                break;
                            case RADEON_GPIO_DVI_DDC:
                                sd->Card.PortInfo[crtc].DDCType = DDC_DVI;
                                break;
                            case RADEON_GPIO_VGA_DDC:
                                sd->Card.PortInfo[crtc].DDCType = DDC_VGA;
                                break;
                            case RADEON_GPIO_CRT2_DDC:
                                sd->Card.PortInfo[crtc].DDCType = DDC_CRT2;
                                break;
                            default:
                                sd->Card.PortInfo[crtc].DDCType = DDC_NONE;
                                break;
                            }

                        } else {
                            sd->Card.PortInfo[crtc].DDCType = DDC_NONE;
                        }
                        crtc++;
                    } else {
                        /* we have already had two CRTCs assigned. the rest may share the same
                         * port with the existing connector, fill in them accordingly.
                         */
                        for (j=0; j<2; j++) {
                            if (((portinfo>>8) & 0xf) == id[j]) {
                                if (i == 3)
                                    sd->Card.PortInfo[j].TMDSType = TMDS_INT;
                                else if (i == 7)
                                    sd->Card.PortInfo[j].TMDSType = TMDS_EXT;

                                if (sd->Card.PortInfo[j].DACType == DAC_UNKNOWN)
                                    sd->Card.PortInfo[j].DACType = (portinfo & 0xf) - 1;
                            }
                        }
                    }
                }
            }

            for (i=0; i<2; i++) {
                D(bug("[ATI] Port%d: DDCType-%d, DACType-%d, TMDSType-%d, ConnectorType-%d\n",
                           i, sd->Card.PortInfo[i].DDCType, sd->Card.PortInfo[i].DACType,
                           sd->Card.PortInfo[i].TMDSType, sd->Card.PortInfo[i].ConnectorType));
            }
        } else {
            D(bug("[ATI] No Device Info Table found!\n"));
            return FALSE;
        }
    } else {
        /* Some laptops only have one connector (VGA) listed in the connector table,
         * we need to add LVDS in as a non-DDC display.
         * Note, we can't assume the listed VGA will be filled in PortInfo[0],
         * when walking through connector table. connector_found has following meaning:
         * 0 -- nothing found,
         * 1 -- only PortInfo[0] filled,
         * 2 -- only PortInfo[1] filled,
         * 3 -- both are filled.
         */
        int connector_found = 0;

        if ((tmp = RADEON_BIOS16(sd->Card.ROMHeaderStart + 0x50))) {
            for (i = 1; i < 4; i++) {

                if (!RADEON_BIOS8(tmp + i*2) && i > 1) break; /* end of table */

                tmp0 = RADEON_BIOS16(tmp + i*2);
                if (((tmp0 >> 12) & 0x0f) == 0) continue;     /* no connector */
                if (connector_found > 0) {
                    if (sd->Card.PortInfo[tmp1].DDCType == ((tmp0 >> 8) & 0x0f))
                        continue;                             /* same connector */
                }

                /* internal DDC_DVI port will get assigned to PortInfo[0], or if there is no DDC_DVI (like in some IGPs). */
                tmp1 = ((((tmp0 >> 8) & 0xf) == DDC_DVI) || (tmp1 == 1)) ? 0 : 1; /* determine port info index */

                sd->Card.PortInfo[tmp1].DDCType        = (tmp0 >> 8) & 0x0f;
                if (sd->Card.PortInfo[tmp1].DDCType > DDC_CRT2) sd->Card.PortInfo[tmp1].DDCType = DDC_NONE_DETECTED;
                sd->Card.PortInfo[tmp1].DACType        = (tmp0 & 0x01) ? DAC_TVDAC : DAC_PRIMARY;
                sd->Card.PortInfo[tmp1].ConnectorType  = (tmp0 >> 12) & 0x0f;
                if (sd->Card.PortInfo[tmp1].ConnectorType > CONNECTOR_UNSUPPORTED) sd->Card.PortInfo[tmp1].ConnectorType = CONNECTOR_UNSUPPORTED;
                sd->Card.PortInfo[tmp1].TMDSType       = ((tmp0 >> 4) & 0x01) ? TMDS_EXT : TMDS_INT;

                /* some sanity checks */
                if (((sd->Card.PortInfo[tmp1].ConnectorType != CONNECTOR_DVI_D) &&
                     (sd->Card.PortInfo[tmp1].ConnectorType != CONNECTOR_DVI_I)) &&
                    sd->Card.PortInfo[tmp1].TMDSType == TMDS_INT)
                    sd->Card.PortInfo[tmp1].TMDSType = TMDS_UNKNOWN;

                connector_found += (tmp1 + 1);
            }
        } else {
            D(bug("[ATI] No Connector Info Table found!\n"));
            return FALSE;
        }
        if (sd->Card.IsMobility) {
            /* For the cases where only one VGA connector is found,
               we assume LVDS is not listed in the connector table,
               add it in here as the first port.
            */
            if ((connector_found < 3) && (sd->Card.PortInfo[tmp1].ConnectorType == CONNECTOR_CRT)) {
                if (connector_found == 1) {
                    memcpy (&sd->Card.PortInfo[1], &sd->Card.PortInfo[0],
                            sizeof (sd->Card.PortInfo[0]));
                }
                sd->Card.PortInfo[0].DACType = DAC_TVDAC;
                sd->Card.PortInfo[0].TMDSType = TMDS_UNKNOWN;
                sd->Card.PortInfo[0].DDCType = DDC_NONE_DETECTED;
                sd->Card.PortInfo[0].ConnectorType = CONNECTOR_PROPRIETARY;

                D(bug("[ATI] LVDS port is not in connector table, added in.\n"));
                if (connector_found == 0) connector_found = 1;
                else connector_found = 3;
            }

            if ((tmp = RADEON_BIOS16(sd->Card.ROMHeaderStart + 0x42))) {
                if ((tmp0 = RADEON_BIOS16(tmp + 0x15))) {
                    if ((tmp1 = RADEON_BIOS8(tmp0+2) & 0x07)) {
                        sd->Card.PortInfo[0].DDCType = tmp1;
                        D(bug("[ATI] LCD DDC Info Table found!\n"));
                    }
                }
            }
        } else if (connector_found == 2) {
            memcpy (&sd->Card.PortInfo[0], &sd->Card.PortInfo[1],
                    sizeof (sd->Card.PortInfo[0]));
            sd->Card.PortInfo[1].DACType = DAC_UNKNOWN;
            sd->Card.PortInfo[1].TMDSType = TMDS_UNKNOWN;
            sd->Card.PortInfo[1].DDCType = DDC_NONE_DETECTED;
            sd->Card.PortInfo[1].ConnectorType = CONNECTOR_NONE;
            connector_found = 1;
        }

        if (connector_found == 0) {
            D(bug("[ATI] No connector found in Connector Info Table.\n"));
        } else {
            D(bug("[ATI] Connector0: DDCType-%d, DACType-%d, TMDSType-%d, ConnectorType-%d\n",
                       sd->Card.PortInfo[0].DDCType, sd->Card.PortInfo[0].DACType,
                       sd->Card.PortInfo[0].TMDSType, sd->Card.PortInfo[0].ConnectorType));
        }
        if (connector_found == 3) {
            D(bug("[ATI] Connector1: DDCType-%d, DACType-%d, TMDSType-%d, ConnectorType-%d\n",
                       sd->Card.PortInfo[1].DDCType, sd->Card.PortInfo[1].DACType,
                       sd->Card.PortInfo[1].TMDSType, sd->Card.PortInfo[1].ConnectorType));
        }
    }
    return TRUE;
}

/* Read PLL parameters from BIOS block.  Default to typical values if there
   is no BIOS. */
BOOL RADEONGetClockInfoFromBIOS(struct ati_staticdata *sd)
{
    RADEONPLLRec *pll = &sd->Card.pll;
    UWORD pll_info_block;

    if (!sd->Card.VBIOS) {
        return FALSE;
    } else {
        if (sd->Card.IsAtomBios) {
            pll_info_block = RADEON_BIOS16 (sd->Card.MasterDataStart + 12);

            pll->reference_freq = RADEON_BIOS16 (pll_info_block + 82);
            pll->reference_div = 0; /* Need to derive from existing setting
                                        or use a new algorithm to calculate
                                        from min_input and max_input
                                     */
            pll->min_pll_freq = RADEON_BIOS16 (pll_info_block + 78);
            pll->max_pll_freq = RADEON_BIOS32 (pll_info_block + 32);
            pll->xclk = RADEON_BIOS16 (pll_info_block + 72);

            sd->Card.sclk = RADEON_BIOS32(pll_info_block + 8) / 100.0;
            sd->Card.mclk = RADEON_BIOS32(pll_info_block + 12) / 100.0;
            if (sd->Card.sclk == 0) sd->Card.sclk = 200;
            if (sd->Card.mclk == 0) sd->Card.mclk = 200;

            D(bug("[ATI] ref_freq: %d, min_pll: %ld, max_pll: %ld, xclk: %d, sclk: %f, mclk: %f\n",
                       pll->reference_freq, pll->min_pll_freq, pll->max_pll_freq, pll->xclk, sd->Card.sclk, sd->Card.mclk));

        } else {
            pll_info_block = RADEON_BIOS16 (sd->Card.ROMHeaderStart + 0x30);

            pll->reference_freq = RADEON_BIOS16 (pll_info_block + 0x0e);
            pll->reference_div = RADEON_BIOS16 (pll_info_block + 0x10);
            pll->min_pll_freq = RADEON_BIOS32 (pll_info_block + 0x12);
            pll->max_pll_freq = RADEON_BIOS32 (pll_info_block + 0x16);
            pll->xclk = RADEON_BIOS16 (pll_info_block + 0x08);

            sd->Card.sclk = RADEON_BIOS16(pll_info_block + 8) / 100.0;
            sd->Card.mclk = RADEON_BIOS16(pll_info_block + 10) / 100.0;
        }
    }    
    
    return TRUE;
}
