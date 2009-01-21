/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: nvidia.hidd initialization
    Lang: English
*/

 /***************************************************************************\
|*                                                                           *|
|*       Copyright 2003 NVIDIA, Corporation.  All rights reserved.           *|
|*                                                                           *|
|*     NOTICE TO USER:   The source code  is copyrighted under  U.S. and     *|
|*     international laws.  Users and possessors of this source code are     *|
|*     hereby granted a nonexclusive,  royalty-free copyright license to     *|
|*     use this code in individual and commercial software.                  *|
|*                                                                           *|
|*     Any use of this source code must include,  in the user documenta-     *|
|*     tion and  internal comments to the code,  notices to the end user     *|
|*     as follows:                                                           *|
|*                                                                           *|
|*       Copyright 2003 NVIDIA, Corporation.  All rights reserved.           *|
|*                                                                           *|
|*     NVIDIA, CORPORATION MAKES NO REPRESENTATION ABOUT THE SUITABILITY     *|
|*     OF  THIS SOURCE  CODE  FOR ANY PURPOSE.  IT IS  PROVIDED  "AS IS"     *|
|*     WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  NVIDIA, CORPOR-     *|
|*     ATION DISCLAIMS ALL WARRANTIES  WITH REGARD  TO THIS SOURCE CODE,     *|
|*     INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGE-     *|
|*     MENT,  AND FITNESS  FOR A PARTICULAR PURPOSE.   IN NO EVENT SHALL     *|
|*     NVIDIA, CORPORATION  BE LIABLE FOR ANY SPECIAL,  INDIRECT,  INCI-     *|
|*     DENTAL, OR CONSEQUENTIAL DAMAGES,  OR ANY DAMAGES  WHATSOEVER RE-     *|
|*     SULTING FROM LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION     *|
|*     OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  ARISING OUT OF     *|
|*     OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOURCE CODE.     *|
|*                                                                           *|
|*     U.S. Government  End  Users.   This source code  is a "commercial     *|
|*     item,"  as that  term is  defined at  48 C.F.R. 2.101 (OCT 1995),     *|
|*     consisting  of "commercial  computer  software"  and  "commercial     *|
|*     computer  software  documentation,"  as such  terms  are  used in     *|
|*     48 C.F.R. 12.212 (SEPT 1995)  and is provided to the U.S. Govern-     *|
|*     ment only as  a commercial end item.   Consistent with  48 C.F.R.     *|
|*     12.212 and  48 C.F.R. 227.7202-1 through  227.7202-4 (JUNE 1995),     *|
|*     all U.S. Government End Users  acquire the source code  with only     *|
|*     those rights set forth herein.                                        *|
|*                                                                           *|
 \***************************************************************************/

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/semaphores.h>

#include <dos/bptr.h>

#include <libcore/compiler.h>

#include <utility/utility.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>

#include <oop/oop.h>

#include <hidd/pci.h>
#include <hidd/hidd.h>
#include <hidd/graphics.h>

#include <aros/symbolsets.h>

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE
#include "nv.h"

#undef HiddPCIDeviceAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#define HiddPCIDeviceAttrBase	(sd->pciAttrBase)
#define HiddBitMapAttrBase	(sd->bitMapAttrBase)
#define HiddNVidiaBitMapAttrBase (sd->nvBitMapAttrBase)
#define HiddPixFmtAttrBase	(sd->pixFmtAttrBase)
#define HiddGfxAttrBase		(sd->gfxAttrBase)
#define HiddSyncAttrBase	(sd->syncAttrBase)
#define __IHidd_PlanarBM	(sd->planarAttrBase)

static BOOL GenericInit(struct staticdata *sd)
{
    D(bug("[NVidia] Generic chip init\n"));
    IPTR regs = (IPTR)sd->Card.Registers;
    UWORD architecture = sd->Card.Architecture;
    UWORD implementation = sd->Card.Chipset;
    UWORD implementation_masked = implementation & 0x0FF0;
    /* turn off scaler by default */
    sd->Card.FpScale = TRUE;
    sd->Card.EnableIRQ	= 0;
    sd->Card.IO		= 0x3d0;
    /* original didn't have /4 in here */
    sd->Card.PRAMDAC0	= (ULONG*)(regs + (0x00680000));
    sd->Card.PFB	= (ULONG*)(regs + (0x00100000));
    sd->Card.PFIFO	= (ULONG*)(regs + (0x00002000));
    sd->Card.PGRAPH	= (ULONG*)(regs + (0x00400000));
    sd->Card.PEXTDEV	= (ULONG*)(regs + (0x00101000));
    sd->Card.PTIMER	= (ULONG*)(regs + (0x00009000));
    sd->Card.PMC	= (ULONG*)(regs + (0x00000000));
    sd->Card.FIFO	= (ULONG*)(regs + (0x00800000));
    
    /* 8 bit registers */
    sd->Card.PCIO0	= (UBYTE*)(regs + 0x00601000);
    sd->Card.PDIO0	= (UBYTE*)(regs + 0x00681000);
    sd->Card.PVIO	= (UBYTE*)(regs + 0x000C0000);

    sd->Card.FlatPanel	= 0;

    sd->Card.twoHeads =  (architecture >= NV_ARCH_10) &&
                     (implementation_masked != 0x0100) &&
                     (implementation_masked != 0x0150) &&
                     (implementation_masked != 0x01A0) &&
                     (implementation_masked != 0x0200);

    sd->Card.fpScaler = (sd->Card.FpScale && sd->Card.twoHeads && (implementation_masked != 0x0110));

    sd->Card.twoStagePLL = (implementation_masked == 0x0310) ||
                           (implementation_masked == 0x0340) ||
			   (architecture == NV_ARCH_40);
    
    sd->Card.alphaCursor = implementation_masked >= 0x0110;

    switch (implementation)
    {
	    case 0x0112:
	    case 0x0174:
	    case 0x0175:
	    case 0x0176:
	    case 0x0177:
	    case 0x0179:
	    case 0x017C:
	    case 0x017D:
	    case 0x0186:
	    case 0x0187:
	    case 0x018D:
	    case 0x0228:
	    case 0x0286:
	    case 0x028C:
	    case 0x0316:
	    case 0x0317:
	    case 0x031A:
	    case 0x031B:
	    case 0x031C:
	    case 0x031D:
	    case 0x031E:
	    case 0x031F:
	    case 0x0324:
	    case 0x0325:
	    case 0x0328:
	    case 0x0329:
	    case 0x032C:
	    case 0x032D:
	    case 0x0347:
	    case 0x0348:
	    case 0x0349:
	    case 0x034B:
	    case 0x034C:
	    case 0x0160:
	    case 0x0166:
	    case 0x0169:
	    case 0x016B:
	    case 0x016C:
	    case 0x016D:
	    case 0x00C8:
	    case 0x00CC:
	    case 0x0144:
	    case 0x0146:
	    case 0x0148:
	    case 0x0098:
	    case 0x0099:
	    D(bug("[NVidia] Assuming Digital FlatPanel\n"));
	    sd->Card.FlatPanel = 1;
	    break;
	default:
	    break;
    }

    if (architecture == NV_ARCH_04)
	nv4GetConfig(sd);
    else
	nv10GetConfig(sd);
    
    NVSelectHead(sd, 0);

    NVLockUnlock(sd, 0);

    if (!sd->Card.twoHeads)
    {
	VGA_WR08(sd->Card.PCIO, 0x3d4, 0x28);
	if (VGA_RD08(sd->Card.PCIO, 0x3d5) & 0x80) 
        {
	    VGA_WR08(sd->Card.PCIO, 0x3d4, 0x33);
             sd->Card.FlatPanel = 1;
            if (!(VGA_RD08(sd->Card.PCIO, 0x3d5) & 0x01)) 
                sd->Card.Television=1;
	} else {
	    sd->Card.FlatPanel = 0;
	}
    }
    else
    {
	UBYTE outputAfromCRTC, outputBfromCRTC;
	int CRTCnumber = -1;
	UBYTE slaved_on_A, slaved_on_B,tvA=0,tvB=0;
	BOOL analog_on_A, analog_on_B;
	ULONG oldhead;
	UBYTE cr44;
	
	if(implementation_masked != 0x0110) {
	    if(sd->Card.PRAMDAC0[0x0000052C/4] & 0x100)
		outputAfromCRTC = 1;
	    else
		outputAfromCRTC = 0;
	    if(sd->Card.PRAMDAC0[0x0000252C/4] & 0x100)
		outputBfromCRTC = 1;
	    else
		outputBfromCRTC = 0;
	    analog_on_A = NVIsConnected(sd, 0);
	    analog_on_B = NVIsConnected(sd, 1);
	} else {
	    outputAfromCRTC = 0;
	    outputBfromCRTC = 1;
	    analog_on_A = FALSE;
	    analog_on_B = FALSE;
	}

	VGA_WR08(sd->Card.PCIO, 0x03D4, 0x44);
	cr44 = VGA_RD08(sd->Card.PCIO, 0x03D5);

	VGA_WR08(sd->Card.PCIO, 0x03D5, 3);
	NVSelectHead(sd, 1);
	NVLockUnlock(sd, 0);

	VGA_WR08(sd->Card.PCIO, 0x03D4, 0x28);
	slaved_on_B = VGA_RD08(sd->Card.PCIO, 0x03D5) & 0x80;
	if(slaved_on_B) {
	    VGA_WR08(sd->Card.PCIO, 0x03D4, 0x33);
	    tvB = !(VGA_RD08(sd->Card.PCIO, 0x03D5) & 0x01);
	}

	VGA_WR08(sd->Card.PCIO, 0x03D4, 0x44);
	VGA_WR08(sd->Card.PCIO, 0x03D5, 0);
	NVSelectHead(sd, 0);
	NVLockUnlock(sd, 0);

	VGA_WR08(sd->Card.PCIO, 0x03D4, 0x28);
	slaved_on_A = VGA_RD08(sd->Card.PCIO, 0x03D5) & 0x80;
	if(slaved_on_A) {
	    VGA_WR08(sd->Card.PCIO, 0x03D4, 0x33);
	    tvA = !(VGA_RD08(sd->Card.PCIO, 0x03D5) & 0x01);
	}

	oldhead = sd->Card.PCRTC0[0x00000860/4];
	sd->Card.PCRTC0[0x00000860/4] = oldhead | 0x00000010;

	if(slaved_on_A && !tvA) {
	    CRTCnumber = 0;
	    sd->Card.FlatPanel = 1;
	} else
	if(slaved_on_B && !tvB) {
	    CRTCnumber = 1;
	    sd->Card.FlatPanel = 1;
	} else
	if(analog_on_A) {
	    CRTCnumber = outputAfromCRTC;
	    sd->Card.FlatPanel = 0;
	} else
	if(analog_on_B) {
	    CRTCnumber = outputBfromCRTC;
	    sd->Card.FlatPanel = 0;
	} else
	if(slaved_on_A) {
	    CRTCnumber = 0;
	    sd->Card.FlatPanel = 1;
	    sd->Card.Television = 1;
	} else
	if(slaved_on_B) {
	    CRTCnumber = 1;
	    sd->Card.FlatPanel = 1;
	    sd->Card.Television = 1;
	}
	
	if (CRTCnumber == -1)
	{
	    D(bug("[NVidia] Unable to detect CRTC number, using defaults\n"));

	    if (sd->Card.FlatPanel) sd->Card.CRTCnumber = 1;
	    else sd->Card.CRTCnumber = 0;
	}
	else sd->Card.CRTCnumber = CRTCnumber;

	if(implementation_masked == 0x0110)
	    cr44 = sd->Card.CRTCnumber * 0x3;

	sd->Card.PCRTC0[0x00000860/4] = oldhead;

	VGA_WR08(sd->Card.PCIO, 0x03D4, 0x44);
	VGA_WR08(sd->Card.PCIO, 0x03D5, cr44);
	NVSelectHead(sd, sd->Card.CRTCnumber);
    }

    if (sd->Card.FlatPanel && !sd->Card.Television)
    {
	sd->Card.fpWidth = sd->Card.PRAMDAC[0x0820/4] + 1;
	sd->Card.fpHeight= sd->Card.PRAMDAC[0x0800/4] + 1;
        sd->Card.fpVTotal = sd->Card.PRAMDAC[0x804/4] + 1;
        sd->Card.fpSyncs = sd->Card.PRAMDAC[0x0848/4] & 0x30000033;
    }

    sd->Card.LVDS = FALSE;

     if(sd->Card.FlatPanel && sd->Card.twoHeads) {
        sd->Card.PRAMDAC0[0x08B0/4] = 0x00010004;
        if(sd->Card.PRAMDAC0[0x08B4/4] & 1)
           sd->Card.LVDS = TRUE;
        }

    sd->Card.PRAMDAC[0x0300/4] = 0;

    D(bug("[NVidia] Configuration received.\n"));
    D(bug("[NVidia] Card has %dMB of video memory\n",
	sd->Card.RamAmountKBytes >> 10));

    sd->Card.FrameBufferSize = sd->Card.RamAmountKBytes * 1024;
    sd->Card.FbUsableSize = sd->Card.FrameBufferSize - 128 * 1024;

    D(bug("[NVidia] Max clock is %dMHz\n",
	sd->Card.MaxVClockFreqKHz / 1000));

kprintf("\n=== NVidia GenericInit FlatPanel = %d TwoHeads = %d CRTCnumber = %d twostagepll = %d\n\n",
    	sd->Card.FlatPanel,
	sd->Card.twoHeads,
	sd->Card.CRTCnumber,
	sd->Card.twoStagePLL);
	
    return TRUE;
}

static BOOL NV4Init(struct staticdata *sd)
{
    sd->Card.PRAMIN = sd->Card.Registers + 0x00710000;
    sd->Card.PCRTC0 = sd->Card.Registers + 0x00600000;
    return GenericInit(sd);
}

static BOOL NV10Init(struct staticdata *sd)
{
    return NV4Init(sd);
}

static BOOL NV20Init(struct staticdata *sd)
{
    return NV4Init(sd);
}

/* List of DeviceID's of supported nvidia cards */
static const struct NVDevice {
    UWORD VendorID, ProductID;
    CardType Type;
    UWORD Arch;
    BOOL (*Init)(struct staticdata*);
    BOOL masked_check;
    
} support[] = {

    /* Known: match ProductID exactly */
    { 0x10de, 0x0020, NV04,	NV_ARCH_04, NV4Init, FALSE }, /* RIVA TNT */

    { 0x10de, 0x0028, NV05,	NV_ARCH_04, NV4Init, FALSE }, /* RIVA TNT2 */
    { 0x10de, 0x002a, NV05,	NV_ARCH_04, NV4Init, FALSE }, /* Unknown TNT2 */
    { 0x10de, 0x002c, NV05,	NV_ARCH_04, NV4Init, FALSE }, /* Vanta */
    { 0x10de, 0x0029, NV05,	NV_ARCH_04, NV4Init, FALSE }, /* RIVA TNT2 Ultra -needs confirmation */
    { 0x10de, 0x002d, NV05M64,	NV_ARCH_04, NV4Init, FALSE }, /* RIVA TNT2 Model 64 */

    { 0x10de, 0x00a0, NV05,	NV_ARCH_04, NV4Init, FALSE }, /* Aladdin TNT2 */

    { 0x10de, 0x0100, NV10,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce 256 */
    { 0x10de, 0x0101, NV10,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce DDR */
    { 0x10de, 0x0103, NV10,	NV_ARCH_10, NV10Init, FALSE }, /* Quadro */

    { 0x10de, 0x0110, NV11,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce2 MX/MX 400 */
    { 0x10de, 0x0111, NV11,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce2 MX 100/200 */
    { 0x10de, 0x0112, NV11,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce2 Go */
    { 0x10de, 0x0113, NV11,	NV_ARCH_10, NV10Init, FALSE }, /* Quadro2 MXR/EX/Go */

    { 0x10de, 0x01A0, NV11,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce2 Integrated GPU - needs confirmation */

    { 0x10de, 0x0150, NV15,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce2 GTS */
    { 0x10de, 0x0151, NV15,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce2 Ti */
    { 0x10de, 0x0152, NV15,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce2 Ultra */
    { 0x10de, 0x0153, NV15,	NV_ARCH_10, NV10Init, FALSE }, /* Quadro2 Pro */

    { 0x10de, 0x0170, NV17,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 MX 460 */
    { 0x10de, 0x0171, NV17,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 MX 440 */
    { 0x10de, 0x0172, NV17,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 MX 420 */
    { 0x10de, 0x0173, NV17,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 MX 440-SE */
    { 0x10de, 0x0174, NV17,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 440 Go */
    { 0x10de, 0x0175, NV17,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 420 Go */
    { 0x10de, 0x0176, NV17,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 420 Go 32M */
    { 0x10de, 0x0177, NV17,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 460 Go */
    { 0x10de, 0x0178, NV17,	NV_ARCH_10, NV10Init, FALSE }, /* Quadro4 550 XGL */

    { 0x10de, 0x0179, NV17,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 440 Go 64M / GeForce4 Mx (Mac) */

    { 0x10de, 0x017a, NV17,	NV_ARCH_10, NV10Init, FALSE }, /* Quadro4 NVS */
    { 0x10de, 0x017c, NV17,	NV_ARCH_10, NV10Init, FALSE }, /* Quadro4 500 GoGL */
    { 0x10de, 0x017d, NV17,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 410 Go 16M */

    { 0x10de, 0x0181, NV18,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 MX 440 with AGP8x */
    { 0x10de, 0x0182, NV18,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 MX 440SE with AGP8x */
    { 0x10de, 0x0183, NV18,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 MX 420 with AGP8x */
    { 0x10de, 0x0185, NV18,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 MX 4000 */
    { 0x10de, 0x0186, NV18,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 448 Go */
    { 0x10de, 0x0187, NV18,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 488 Go */
    { 0x10de, 0x0188, NV18,	NV_ARCH_10, NV10Init, FALSE }, /* Quadro4 580 XGL */

    { 0x10de, 0x0189, NV18,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 MX with AGP8X (Mac) */

    { 0x10de, 0x018a, NV18,	NV_ARCH_10, NV10Init, FALSE }, /* Quadro4 280 NVS */
    { 0x10de, 0x018b, NV18,	NV_ARCH_10, NV10Init, FALSE }, /* Quadro4 380 XGL */
    { 0x10de, 0x018c, NV18,	NV_ARCH_10, NV10Init, FALSE }, /* Quadro NVS 50 PCI */
    { 0x10de, 0x018d, NV18,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 448 Go */

    { 0x10de, 0x01f0, NV17,	NV_ARCH_10, NV10Init, FALSE }, /* GeForce4 MX Integerated GPU */
    
    { 0x10de, 0x0200, NV20,	NV_ARCH_20, NV20Init, FALSE }, /* GeForce3 */
    { 0x10de, 0x0201, NV20,	NV_ARCH_20, NV20Init, FALSE }, /* GeForce3 Ti 200 */
    { 0x10de, 0x0202, NV20,	NV_ARCH_20, NV20Init, FALSE }, /* GeForce3 Ti 500 */
    { 0x10de, 0x0203, NV20,	NV_ARCH_20, NV20Init, FALSE }, /* Quadro DCC */

    { 0x10de, 0x0250, NV25,	NV_ARCH_20, NV20Init, FALSE }, /* GeForce4 Ti 4600 */
    { 0x10de, 0x0251, NV25,	NV_ARCH_20, NV20Init, FALSE }, /* GeForce4 Ti 4400 */
    { 0x10de, 0x0253, NV25,	NV_ARCH_20, NV20Init, FALSE }, /* GeForce4 Ti 4200 */
    { 0x10de, 0x0258, NV25,	NV_ARCH_20, NV20Init, FALSE }, /* Quadro4 900 XGL */
    { 0x10de, 0x0259, NV25,	NV_ARCH_20, NV20Init, FALSE }, /* Quadro4 750 XGL */
    { 0x10de, 0x025b, NV25,	NV_ARCH_20, NV20Init, FALSE }, /* Quadro4 700 XGL */

    { 0x10de, 0x0280, NV28,	NV_ARCH_20, NV20Init, FALSE }, /* GeForce4 Ti 4800 */
    { 0x10de, 0x0281, NV28,	NV_ARCH_20, NV20Init, FALSE }, /* GeForce4 Ti 4200 with AGP8X */
    { 0x10de, 0x0282, NV28,	NV_ARCH_20, NV20Init, FALSE }, /* GeForce4 Ti 4800 SE */
    { 0x10de, 0x0286, NV28,	NV_ARCH_20, NV20Init, FALSE }, /* GeForce4 4200 Go */
    { 0x10de, 0x028c, NV28,	NV_ARCH_20, NV20Init, FALSE }, /* Quadro4 700 GoGL */
    { 0x10de, 0x0288, NV28,	NV_ARCH_20, NV20Init, FALSE }, /* Quadro4 980 XGL */
    { 0x10de, 0x0289, NV28,	NV_ARCH_20, NV20Init, FALSE }, /* Quadro4 780 XGL */

    { 0x10de, 0x0301, NV30,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5800 Ultra */
    { 0x10de, 0x0302, NV30,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5800 */
    { 0x10de, 0x0308, NV30,	NV_ARCH_30, NV20Init, FALSE }, /* Quadro FX 2000 */
    { 0x10de, 0x0309, NV30,	NV_ARCH_30, NV20Init, FALSE }, /* Quadro FX 1000 */

    { 0x10de, 0x0311, NV31,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5600 Ultra */
    { 0x10de, 0x0312, NV31,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5600 */
    { 0x10de, 0x0314, NV31,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5600 SE */
    { 0x10de, 0x031a, NV31,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX Go5600 */
    { 0x10de, 0x031b, NV31,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX Go5650 */
    { 0x10de, 0x031c, NV31,	NV_ARCH_30, NV20Init, FALSE }, /* Quadro FX Go700 */
 
    { 0x10de, 0x0320, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5200 */
    { 0x10de, 0x0321, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5200 Ultra */
    { 0x10de, 0x0322, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5200 */
    { 0x10de, 0x0323, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5200SE */
    { 0x10de, 0x0324, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX Go5200 */
    { 0x10de, 0x0325, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX Go5250 */
    { 0x10de, 0x0326, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5500 */
    { 0x10de, 0x0327, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5100 */
    { 0x10de, 0x0328, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX Go5200 32M/64M */

    { 0x10de, 0x0329, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* 0x329 / GeForce FX 5200 (Mac) */

    { 0x10de, 0x032a, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* Quadro NVS 280 PCI */
    { 0x10de, 0x032b, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* Quadro FX 500/600 PCI */
    { 0x10de, 0x032c, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX Go53xx Series */
    { 0x10de, 0x032d, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX Go5100 */

    { 0x10de, 0x0330, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce Fx 5900 Ultra */
    { 0x10de, 0x0331, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5900 */
    { 0x10de, 0x0332, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5900XT */
    { 0x10de, 0x0333, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5950 Ultra */
    { 0x10de, 0x0334, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5900ZT */
    { 0x10de, 0x0338, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* Quadro FX 3000 */
    { 0x10de, 0x033F, NV34,	NV_ARCH_30, NV20Init, FALSE }, /* Quadro FX 700 */

    { 0x10de, 0x0341, NV36,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5700 Ultra */
    { 0x10de, 0x0342, NV36,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5700 */
    { 0x10de, 0x0343, NV36,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5700LE */
    { 0x10de, 0x0344, NV36,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX 5700VE */
    { 0x10de, 0x0347, NV36,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX Go5700 */
    { 0x10de, 0x0348, NV36,	NV_ARCH_30, NV20Init, FALSE }, /* GeForce FX Go5700 */
    { 0x10de, 0x034C, NV36,	NV_ARCH_30, NV20Init, FALSE }, /* Quadro FX Go1000 */
    { 0x10de, 0x034E, NV36,	NV_ARCH_30, NV20Init, FALSE }, /* Quadro FX 1100 */

    { 0x10de, 0x0040, NV36,	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 Ultra */
    { 0x10de, 0x0041, NV36,	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 */
    { 0x10de, 0x0042, NV36,	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 LE */
    { 0x10de, 0x0043, NV36,	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 XE */
    { 0x10de, 0x0044, NV36,     NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 XT */
    { 0x10de, 0x0045, NV36,     NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 GT */
    { 0x10de, 0x0046, NV36,     NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 GT */
    { 0x10de, 0x0047, NV36,     NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 GS */
    { 0x10de, 0x0048, NV36,     NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 XT */
    { 0x10de, 0x004E, NV36,	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 4000 */

    { 0x10de, 0x00C0, NV36,	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 GS */
    { 0x10de, 0x00C1, NV36,	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 */
    { 0x10de, 0x00C2, NV36,	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 LE */
    { 0x10de, 0x00C3, NV36,	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 XT */
    { 0x10de, 0x00C8, NV36,	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 6800 */
    { 0x10de, 0x00C9, NV36,	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 6800 Ultr */
    { 0x10de, 0x00CC, NV36,	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX Go1400 */
    { 0x10de, 0x00CD, NV36,	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 3450/4000 SDI */
    { 0x10de, 0x00CE, NV36,	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 1400 */

    { 0x10de, 0x00F1, NV36,     NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6600 / GeForce 6600 GT (Verified: LeadTek GeForce 6600 GT) */
    { 0x10de, 0x00F2, NV43,     NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6600 / GeForce 6600 GT (Verified: Club3D GeForce 6600) */

    { 0x10de, 0x0140, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6600 GT */
    { 0x10de, 0x0141, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6600 */
    { 0x10de, 0x0142, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6600 LE */
    { 0x10de, 0x0143, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6600 VE */
    { 0x10de, 0x0144, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 6600 */
    { 0x10de, 0x0145, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6610 XL */
    { 0x10de, 0x0146, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 6600 TE/6200 TE */
    { 0x10de, 0x0147, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6700 XL */
    { 0x10de, 0x0148, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 6600 */
    { 0x10de, 0x0149, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 6600 GT */
    { 0x10de, 0x014C, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 550 */
    { 0x10de, 0x014D, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 550 */
    { 0x10de, 0x014E, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 540 */
    { 0x10de, 0x014F, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6200 */

    { 0x10de, 0x0160, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6500 */
    { 0x10de, 0x0161, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6200 TurboCache */
    { 0x10de, 0x0162, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6200SE TurboCache */
    { 0x10de, 0x0163, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6200 LE */
    { 0x10de, 0x0164, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 6200 */
    { 0x10de, 0x0165, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro NVS 285 */
    { 0x10de, 0x0166, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 6400 */
    { 0x10de, 0x0167, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 6200 */
    { 0x10de, 0x0168, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 6400 */
    { 0x10de, 0x0169, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6250 */
    
    { 0x10de, 0x0211, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 */
    { 0x10de, 0x0212, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 LE */
    { 0x10de, 0x0215, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 GT */
    { 0x10de, 0x0218, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6800 XT */

    { 0x10de, 0x0221, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6200 */
    { 0x10de, 0x0222, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6200 A-LE */

    { 0x10de, 0x0090, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7800 GTX */
    { 0x10de, 0x0091, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7800 GTX */
    { 0x10de, 0x0092, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7800 GT */
    { 0x10de, 0x0093, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7800 GS */
    { 0x10de, 0x0095, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7800 SLI */
    { 0x10de, 0x0098, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 7800 */
    { 0x10de, 0x0099, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 7800 GTX */
    { 0x10de, 0x009D, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 4500 */

    { 0x10de, 0x01D1, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7300 LE */
    { 0x10de, 0x01D3, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7300 SE */
    { 0x10de, 0x01D6, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 7200 */
    { 0x10de, 0x01D7, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 7300 */
    { 0x10de, 0x01D8, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 7400 */
    { 0x10de, 0x01D9, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 7400 GS */
    { 0x10de, 0x01DA, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro NVS 110M */
    { 0x10de, 0x01DB, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro NVS 120M */
    { 0x10de, 0x01DC, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 350M */
    { 0x10de, 0x01DD, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7500 LE */
    { 0x10de, 0x01DE, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 350 */
    { 0x10de, 0x01DF, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7300 GS */

    { 0x10de, 0x0391, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7600 GT */
    { 0x10de, 0x0392, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7600 GS */
    { 0x10de, 0x0393, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7600 GT */
    { 0x10de, 0x0394, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7600 LE */
    { 0x10de, 0x0395, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7600 GT */
    { 0x10de, 0x0397, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 7700 */
    { 0x10de, 0x0398, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 7600 */
    { 0x10de, 0x0399, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 7600 GT */
    { 0x10de, 0x039A, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce NVS 300M */
    { 0x10de, 0x039B, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 7900 SE */
    { 0x10de, 0x039C, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 550M */
    { 0x10de, 0x039E, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 560 */
    
    { 0x10de, 0x0290, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7900 GTX */
    { 0x10de, 0x0291, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7900 GT */
    { 0x10de, 0x0292, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 7900 GS */
    { 0x10de, 0x0298, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 7900 GS */
    { 0x10de, 0x0299, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 7900 GTX */
    { 0x10de, 0x029A, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 2500M */
    { 0x10de, 0x029B, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 1500M */
    { 0x10de, 0x029C, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 5500 */
    { 0x10de, 0x029D, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 3500 */
    { 0x10de, 0x029E, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 1500 */
    { 0x10de, 0x029F, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* Quadro FX 4500 X2 */

    { 0x10de, 0x0240, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6150 */
    { 0x10de, 0x0241, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6150 LE */
    { 0x10de, 0x0242, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce 6100 */
    { 0x10de, 0x0244, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 6150 */
    { 0x10de, 0x0247, NV36, 	NV_ARCH_40, NV20Init, FALSE }, /* GeForce Go 6100 */
    
    { 0x0000, 0x0000, }
};

#define sd			((struct staticdata*)hook->h_Data)
AROS_UFH3(void, Enumerator,
    AROS_UFHA(struct Hook *,	hook,	    A0),
    AROS_UFHA(OOP_Object *,	pciDevice,  A2),
    AROS_UFHA(APTR,		message,    A1))
{
    AROS_USERFUNC_INIT
    
    struct NVDevice *sup = (struct NVDevice *)support;
    IPTR ProductID;
    IPTR VendorID;

    /* Get the Device's ProductID */
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &ProductID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &VendorID);

    D(bug("[NVidia] Enumerator: checking productid %d vendorid %d  sd->Deivce %x\n",
    	  ProductID, VendorID, sd->Device));

    /* And try to match it with supported cards */
    while (sup->VendorID)
    {
    	BOOL found = FALSE;
	
	if (sup->VendorID == VendorID)
	{
	    if (!sup->masked_check && (sup->ProductID == ProductID))
	    {
	    	found = TRUE;
	    }
	    else if (sup->masked_check && (sup->ProductID == (ProductID & 0xFFF0)))
	    {
	    	found = TRUE;
	    }
	}

	if (found)
	{
	    /* Matching card found */
	    APTR buf;
	    IPTR size;
	    struct pHidd_PCIDriver_MapPCI mappci,*msg = &mappci;
	    OOP_Object *driver;
	    struct MemChunk *mc;
	
	    struct TagItem attrs[] = {
		{ aHidd_PCIDevice_isIO,	    FALSE },	/* Don't listen IO transactions */
		{ aHidd_PCIDevice_isMEM,    TRUE },	/* Listen to MEM transactions */
		{ aHidd_PCIDevice_isMaster, TRUE },	/* Can work in BusMaster */
		{ TAG_DONE, 0UL },
	    };

    	    D(bug("[NVidia] Enumerator: found productid %d vendorid %d masked_check %d\n",
	    	  sup->ProductID, sup->VendorID, sup->masked_check));
	

	    sd->Card.ProductID = ProductID;
	    sd->Card.VendorID = VendorID;
	    sd->Card.Type = sup->Type;
	    sd->Card.Architecture = sup->Arch;
	    sd->Card.Chipset = ProductID;

	    D(bug("[NVidia] Chip architecture = %d\n", sd->Card.Architecture));

	    /*-------- DO NOT CHANGE/REMOVE -------------*/
	    bug("\003\n"); /* Tell vga text mode debug output to die */
	    /*-------- DO NOT CHANGE/REMOVE -------------*/
			    
	    /*
		Fix PCI device attributes (perhaps already set, but if the 
		NVidia would be the second card in the system, it may stay
		uninitialized.
	    */
	    OOP_SetAttrs(pciDevice, (struct TagItem*)&attrs);
	    
	    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&driver);
	    sd->pcidriver = driver;

	    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base1, (APTR)&buf);
	    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size1, (APTR)&size);

	    sd->mid_ReadLong = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigLong);
	    
	    mappci.mID = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_MapPCI);
	    mappci.PCIAddress = buf;
	    mappci.Length = size;
	    
	    sd->Card.FbAddress = buf;
	    sd->Card.FrameBuffer = (APTR)OOP_DoMethod(driver, (OOP_Msg)msg);
	    mc = (struct MemChunk *)sd->Card.FrameBuffer;

	    sd->CardMem->mh_Node.ln_Type = NT_MEMORY;
	    sd->CardMem->mh_Node.ln_Name = "nVidia Framebuffer";
	    sd->CardMem->mh_First = mc;
	    sd->CardMem->mh_Lower = (APTR)mc;

	    D(bug("[NVidia] Got framebuffer @ %x (size=%x)\n", sd->Card.FrameBuffer, size));

	    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, (APTR)&buf);
	    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0, (APTR)&size);

	    mappci.PCIAddress = buf;
	    mappci.Length = size;
	    
	    sd->Card.Registers = (APTR)OOP_DoMethod(driver, (OOP_Msg)msg);
	    D(bug("[NVidia] Got registers @ %x (size=%x)\n", sd->Card.Registers, size));
	    
	    if (sup->Init(sd))
	    {
		struct CardState *state = (struct CardState *)AllocPooled(sd->memPool, 
					sizeof(struct CardState));

		sd->CardMem->mh_Free = sd->Card.FbUsableSize;
		sd->CardMem->mh_Upper = (APTR)(sd->CardMem->mh_Free + (IPTR)mc);

		mc->mc_Next = NULL;
		mc->mc_Bytes = sd->CardMem->mh_Free;

		D(bug("[NVidia] Usable size: %dKB\n", sd->CardMem->mh_Free >> 10));

		sd->Device = pciDevice;

		InitMode(sd, state, 640, 480, 16, 25200, 0, 
		    640, 480,
		    656, 752, 800,
		    490, 492, 525);
/*
		InitMode(sd, state, 800, 600, 24, 36000, 0,
		    800, 600,
		    824, 896, 1024,
		    601, 603, 625);
*/
		LoadState(sd, state);
		DPMS(sd,vHidd_Gfx_DPMSLevel_On);

		sd->scratch_buffer = AllocBitmapArea(sd, 65536, 1, 4, TRUE);
//		acc_test(sd);
	    }
	    else
	    {
		struct pHidd_PCIDriver_UnmapPCI unmappci,*msg=&unmappci;
	
		OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size1, (APTR)&size);

		unmappci.mID = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_UnmapPCI);
		unmappci.CPUAddress = sd->Card.FrameBuffer;
		unmappci.Length = size;

		OOP_DoMethod(driver, (OOP_Msg)msg);
	
		OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0, (APTR)&size);
		unmappci.CPUAddress = sd->Card.Registers;
		unmappci.Length = size;

		OOP_DoMethod(driver, (OOP_Msg)msg);
	
		sd->Device = NULL;
	    }

	    break;
	}
	sup++;
    }

    D(bug("[NVidia] Enumerator found a card (ProductID=%04x)\n", ProductID));
    D(bug("[NVidia] The card is %ssupported\n",
	sd->Device ? "":"un"));

    AROS_USERFUNC_EXIT
}

#undef sd
static void Find_NV_Card(struct staticdata *sd)
{
    D(bug("[NVidia] Find_NV_Card\n"));

    if (HiddPCIDeviceAttrBase)
    {
	sd->pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
	
	D(bug("[NVidia] Creating PCI object\n"));

	if (sd->pci)
	{
	    struct Hook FindHook = {
		h_Entry:    (IPTR (*)())Enumerator,
		h_Data:	    sd,
	    };

	    struct TagItem Requirements[] = {
		{ tHidd_PCI_Interface,	0x00 },
		{ tHidd_PCI_Class,	0x03 },
		{ tHidd_PCI_SubClass,	0x00 },
		{ tHidd_PCI_VendorID,	0x10de }, // NVidia VendorID. May require more of them
		{ TAG_DONE, 0UL }
	    };
	
	    struct pHidd_PCI_EnumDevices enummsg = {
		mID:		OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
		callback:	&FindHook,
		requirements:	(struct TagItem*)&Requirements,
	    }, *msg = &enummsg;
	    D(bug("[NVidia] Calling search Hook\n"));
	    OOP_DoMethod(sd->pci, (OOP_Msg)msg);
	}
    }
}

static int NV_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[NVidia] Initialization\n"));

    /* Global memory pool and static data creation */
    LIBBASE->memPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED, 8192, 4096);
    if (LIBBASE->memPool)
    {
	struct staticdata *sd = &LIBBASE->sd;
	
	sd->memPool = LIBBASE->memPool;
	sd->CardMem = &LIBBASE->mh;
	    
	InitSemaphore(&sd->HWLock);
	InitSemaphore(&sd->MultiBMLock);
	    
	sd->dpms = vHidd_Gfx_DPMSLevel_On;

	{
	    struct OOP_ABDescr attrbases[] = 
	    {
		{ IID_Hidd_PCIDevice,   &HiddPCIDeviceAttrBase },
		{ IID_Hidd_BitMap,	    &HiddBitMapAttrBase },
		{ IID_Hidd_PixFmt,	    &HiddPixFmtAttrBase },
		{ IID_Hidd_Sync,	    &HiddSyncAttrBase },
		{ IID_Hidd_Gfx,	    &HiddGfxAttrBase },
		{ IID_Hidd_nvBitMap,    &HiddNVidiaBitMapAttrBase },
		{ IID_Hidd_PlanarBM,    &__IHidd_PlanarBM },
		{ NULL, NULL }
	    };
		
	    if (OOP_ObtainAttrBases(attrbases))
	    {
		Find_NV_Card(sd);
		
		if (sd->Device != NULL)
		    return TRUE;
	    }

	    OOP_ReleaseAttrBases(attrbases);

	    OOP_DisposeObject(sd->pci);
	}
    }

    /*
	Something failed. Ehm, that shouldn't happen at all. Now we have to
	clean everything up
    */
    DeletePool(LIBBASE->memPool);
    
    D(bug("[NVidia] nvBase=%x. FAILURE! Now freezing so you can check debug output!\n", LIBBASE));
    
    Disable(); for(;;) ;
    
    return FALSE;
}

ADD2INITLIB(NV_Init, 0)
