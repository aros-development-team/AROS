/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
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

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE
#include "nv.h"

static __attribute__((unused)) LONG __no_exec()
{
    return -1;
}

static const char nv_VersionID[] = VERSION_STRING;
static const char nv_Name[] = NAME_STRING;

static const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];

const struct Resident nv_Resident TEXT_SECTION = {
    RTC_MATCHWORD,
    (struct Resident*)&nv_Resident,
    &LIBEND,
    RTF_AUTOINIT,
    VERSION_NUMBER,
    NT_HIDD,
    0,
    (STRPTR)nv_Name,
    (STRPTR)&nv_VersionID[6],
    (ULONG*)inittabl
};

static const APTR inittabl[4] = {
    (APTR)sizeof(LIBBASETYPE),
    (APTR)LIBFUNCTABLE,
    NULL,
    nv_init
};

#undef HiddPCIDeviceAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#define SysBase			(sd->sysbase)
#define OOPBase			(sd->oopbase)
#define UtilityBase		(sd->utilitybase)
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
    ULONG implementation = sd->Card.Chipset;
    
    sd->Card.EnableIRQ	= 0;
    sd->Card.IO		= 0x3d0;
    sd->Card.PRAMDAC0	= (ULONG*)(regs + 0x00680000);
    sd->Card.PFB	= (ULONG*)(regs + 0x00100000);
    sd->Card.PFIFO	= (ULONG*)(regs + 0x00002000);
    sd->Card.PGRAPH	= (ULONG*)(regs + 0x00400000);
    sd->Card.PEXTDEV	= (ULONG*)(regs + 0x00101000);
    sd->Card.PTIMER	= (ULONG*)(regs + 0x00009000);
    sd->Card.PMC	= (ULONG*)(regs + 0x00000000);
    sd->Card.FIFO	= (ULONG*)(regs + 0x00800000);
    sd->Card.PCIO0	= (UBYTE*)(regs + 0x00601000);
    sd->Card.PDIO0	= (UBYTE*)(regs + 0x00681000);
    sd->Card.PVIO	= (UBYTE*)(regs + 0x000C0000);

    sd->Card.FlatPanel	= 0;

    sd->Card.twoHeads =  (implementation >= 0x0110) &&
                     (implementation != 0x0150) &&
                     (implementation != 0x01A0) &&
                     (implementation != 0x0200);

    sd->Card.fpScaler = (sd->Card.twoHeads && (implementation != 0x0110));

    sd->Card.twoStagePLL = (implementation == 0x0310) ||
                       (implementation == 0x0340);
    
    sd->Card.alphaCursor = (implementation & 0x0ff0) >= 0x0110;

    switch (implementation)
    {
	case 0x0112:
	case 0x0174:
	case 0x0175:
	case 0x0176:
	case 0x0177:
	case 0x0179:
	case 0x017c:
	case 0x017d:
	case 0x0186:
	case 0x0187:
	case 0x0286:
	case 0x028c:
	case 0x0316:
	case 0x0317:
	case 0x031a:
	case 0x031b:
	case 0x031c:
	case 0x031d:
	case 0x031e:
	case 0x031f:
	case 0x0326:
	case 0x032e:
	    D(bug("[NVidia] Assuming Digital FlatPanel\n"));
	    sd->Card.FlatPanel = 1;
	    break;
	default:
	    break;
    }

    if (sd->Card.Architecture == NV_ARCH_04)
	nv4GetConfig(sd);
    else
	nv10GetConfig(sd);
    
    NVSelectHead(sd, 0);

    NVLockUnlock(sd, 0);

    if (!sd->Card.twoHeads)
    {
	VGA_WR08(sd->Card.PCIO, 0x3d4, 0x28);
	if (VGA_RD08(sd->Card.PCIO, 0x3d5) & 0x80)
	    sd->Card.FlatPanel = 1;
	else
	    sd->Card.FlatPanel = 0;
    }
    else
    {
	UBYTE outputAfromCRTC, outputBfromCRTC;
	int CRTCnumber = -1;
	UBYTE slaved_on_A, slaved_on_B,tvA=0,tvB=0;
	BOOL analog_on_A, analog_on_B;
	ULONG oldhead;
	UBYTE cr44;
	
	if(implementation != 0x0110) {
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

	if(implementation == 0x0110)
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
    }

    sd->Card.PRAMDAC[0x0300/4] = 0;

    D(bug("[NVidia] Configuration received.\n"));
    D(bug("[NVidia] Card has %dMB of video memory\n",
	sd->Card.RamAmountKBytes >> 10));

    sd->Card.FrameBufferSize = sd->Card.RamAmountKBytes * 1024;
    sd->Card.FbUsableSize = sd->Card.FrameBufferSize - 128 * 1024;

    D(bug("[NVidia] Max clock is %dMHz\n",
	sd->Card.MaxVClockFreqKHz / 1000));

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
} support[] = {
    { 0x10de, 0x0020, NV04,	NV_ARCH_04, NV4Init },
    { 0x10de, 0x0028, NV05,	NV_ARCH_04, NV4Init },
    { 0x10de, 0x0029, NV05,	NV_ARCH_04, NV4Init },
    { 0x10de, 0x002a, NV05,	NV_ARCH_04, NV4Init },
    { 0x10de, 0x002b, NV05,	NV_ARCH_04, NV4Init },
    { 0x10de, 0x002c, NV05,	NV_ARCH_04, NV4Init },
    { 0x10de, 0x002d, NV05M64,	NV_ARCH_04, NV4Init },
    { 0x10de, 0x002e, NV06,	NV_ARCH_04, NV4Init },
    { 0x10de, 0x002f, NV06,	NV_ARCH_04, NV4Init },
    { 0x10de, 0x00a0, NV05,	NV_ARCH_04, NV4Init },
    { 0x10de, 0x0100, NV10,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0101, NV10,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0102, NV10,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0103, NV10,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0110, NV11,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0111, NV11,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0112, NV11,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0113, NV11,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0150, NV15,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0151, NV15,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0152, NV15,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0153, NV15,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0170, NV17,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0171, NV17,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0172, NV17,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0173, NV17,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0174, NV17,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0175, NV17,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0176, NV17,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0177, NV17,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0179, NV17,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0178, NV17,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x017a, NV17,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x017c, NV17,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x017d, NV17,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0180, NV18,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0181, NV18,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0182, NV18,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0186, NV18,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0187, NV18,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0188, NV18,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x018a, NV18,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x018b, NV18,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x01a0, NV11,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x01f0, NV17,	NV_ARCH_10, NV10Init },
    { 0x10de, 0x0200, NV20,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0201, NV20,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0202, NV20,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0203, NV20,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0250, NV25,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0251, NV25,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0252, NV25,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0253, NV25,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0258, NV25,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0259, NV25,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x025b, NV25,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0280, NV28,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0281, NV28,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0282, NV28,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0286, NV28,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0288, NV28,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0289, NV28,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x028c, NV28,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x02a0, NV20,	NV_ARCH_20, NV20Init },
    { 0x10de, 0x0301, NV30,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0302, NV30,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0308, NV30,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0309, NV30,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0311, NV31,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0312, NV31,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0314, NV31,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0316, NV31,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0317, NV31,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x031a, NV31,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x031b, NV31,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x031c, NV31,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x031d, NV31,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x031e, NV31,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x031f, NV31,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0321, NV34,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0322, NV34,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0324, NV34,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0326, NV34,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x032b, NV34,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x032e, NV34,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0330, NV34,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0331, NV34,	NV_ARCH_30, NV20Init },
    { 0x10de, 0x0338, NV34,	NV_ARCH_30, NV20Init },

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

    /* And try to match it with supported cards */
    while (sup->VendorID)
    {
	if (sup->VendorID == VendorID && sup->ProductID == ProductID)
	{
	    /* Matching card found */
	    APTR buf;
	    ULONG size;
	    struct pHidd_PCIDriver_MapPCI mappci,*msg = &mappci;
	    OOP_Object *driver;
	    struct MemChunk *mc;
	
	    struct TagItem attrs[] = {
		{ aHidd_PCIDevice_isIO,	    FALSE },	/* Don't listen IO transactions */
		{ aHidd_PCIDevice_isMEM,    TRUE },	/* Listen to MEM transactions */
		{ aHidd_PCIDevice_isMaster, TRUE },	/* Can work in BusMaster */
		{ TAG_DONE, 0UL },
	    };

	    sd->Card.ProductID = ProductID;
	    sd->Card.VendorID = VendorID;
	    sd->Card.Type = sup->Type;
	    sd->Card.Architecture = sup->Arch;
	    sd->Card.Chipset = ProductID;

	    D(bug("[NVidia] Chip architecture = %d\n", sd->Card.Architecture));

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

	    sd->mid_ReadLong = OOP_GetMethodID(CLID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigLong);
	    
	    mappci.mID = OOP_GetMethodID(CLID_Hidd_PCIDriver, moHidd_PCIDriver_MapPCI);
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

		unmappci.mID = OOP_GetMethodID(CLID_Hidd_PCIDriver, moHidd_PCIDriver_UnmapPCI);
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
		mID:		OOP_GetMethodID(CLID_Hidd_PCI, moHidd_PCI_EnumDevices),
		callback:	&FindHook,
		requirements:	(struct TagItem*)&Requirements,
	    }, *msg = &enummsg;
	    D(bug("[NVidia] Calling search Hook\n"));
	    OOP_DoMethod(sd->pci, (OOP_Msg)msg);
	}
    }
}

#undef SysBase
AROS_UFH3(LIBBASETYPEPTR, nv_init,
    AROS_UFHA(LIBBASETYPEPTR,	LIBBASE,D0),
    AROS_UFHA(BPTR,		slist,	A0),
    AROS_UFHA(struct ExecBase*,	SysBase,A6))
{
    AROS_USERFUNC_INIT

    D(bug("[NVidia] Initialization\n"));

    /* Pre-initialization */
    LIBBASE->segList = slist;
    LIBBASE->sysbase = SysBase;

    /* Make library look properly */
    LIBBASE->LibNode.lib_Node.ln_Pri	= nv_Resident.rt_Pri;
    LIBBASE->LibNode.lib_Node.ln_Name	= nv_Resident.rt_Name;
    LIBBASE->LibNode.lib_Node.ln_Type	= nv_Resident.rt_Type;
    
    LIBBASE->LibNode.lib_Flags	    = LIBF_SUMUSED | LIBF_CHANGED;
    LIBBASE->LibNode.lib_Version    = VERSION_NUMBER;
    LIBBASE->LibNode.lib_Revision   = REVISION_NUMBER;
    LIBBASE->LibNode.lib_IdString   = (STRPTR)&nv_VersionID[6];

    /* Global memory pool and static data creation */
    LIBBASE->memPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED, 8192, 4096);
    if (LIBBASE->memPool)
    {
	struct staticdata *sd;
	sd = LIBBASE->sd = AllocPooled(LIBBASE->memPool, sizeof(struct staticdata));
	if (LIBBASE->sd)
	{
	    sd->memPool = LIBBASE->memPool;
	    sd->sysbase = SysBase;
	    sd->CardMem = &LIBBASE->mh;
	    
	    InitSemaphore(&sd->HWLock);

	    sd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
	    sd->dpms = vHidd_Gfx_DPMSLevel_On;

	    if (sd->oopbase)
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
		    sd->utilitybase = OpenLibrary(UTILITYNAME, 0);

		    if (sd->utilitybase)
		    {
			Find_NV_Card(sd);
		    
			if (sd->Device)
			{
			    if (init_nvclass(sd))
			    {
				if (init_onbitmapclass(sd))
				{
				    if (init_offbitmapclass(sd))
				    {
					if (init_nvplanarbmclass(sd))
					{
					    D(bug("[NVidia] nvBase=%08x\n", LIBBASE));
					    return LIBBASE;
					}
				    }
				}
			    }
			}
	    	    }

		    OOP_ReleaseAttrBases(attrbases);

		    OOP_DisposeObject(sd->pci);
	
		    CloseLibrary(sd->utilitybase);
		}
		CloseLibrary(sd->oopbase);
	    }
	    
	}
    }

    /*
	Something failed. Ehm, that shouldn't happen at all. Now we have to
	clean everything up
    */
    DeletePool(LIBBASE->memPool);
    
    UBYTE *negptr = (UBYTE*)LIBBASE;
    ULONG negsize, possize, totalsize;

    negsize = LIBBASE->LibNode.lib_NegSize;
    possize = LIBBASE->LibNode.lib_PosSize;
    totalsize = negsize + possize;
    negptr -= negsize;

    FreeMem(negptr, totalsize);
    LIBBASE = NULL;

    D(bug("[NVidia] nvBase=%x\n", LIBBASE));
    return LIBBASE;

    AROS_USERFUNC_EXIT
}

#undef SysBase
#undef OOPBase
#undef UtilityBase
#define SysBase	    (LIBBASE->sysbase)
#define SD(cl)	    (LIBBASE->sd)
#define OOPBase	    (SD(x)->oopbase)
#define UtilityBase (SD(x)->utilitybase)

AROS_LH1(LIBBASETYPEPTR, open,
    AROS_LHA(ULONG, version, D0),
    LIBBASETYPEPTR, LIBBASE, 1, nvidia)
{
    AROS_LIBFUNC_INIT

    LIBBASE->LibNode.lib_OpenCnt++;
    return LIBBASE;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(APTR, close,
    LIBBASETYPEPTR, LIBBASE, 2, nvidia)
{
    AROS_LIBFUNC_INIT

    LIBBASE->LibNode.lib_OpenCnt--;
    return NULL;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BPTR, expunge,
    AROS_LHA(LIBBASETYPEPTR, LIBBASE, D0),
    struct ExecBase *, sysBase, 3, nvidia)
{
    AROS_LIBFUNC_INIT

    LIBBASE->LibNode.lib_Flags |= LIBF_DELEXP;
    return NULL;

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(LONG, null,
    LIBBASETYPEPTR, LIBBASE, 4, nvidia)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}

