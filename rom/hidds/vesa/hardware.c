/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: vesa "hardware" functions
    Lang: English
*/

#define DEBUG 0

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/macros.h>
#include <aros/bootloader.h>
#include <asm/io.h>
#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>
#include <oop/oop.h>
#include <hidd/pci.h>

#include "bitmap.h"
#include "vesagfxclass.h"
#include "hardware.h"

#include <string.h>

static void Find_PCI_Card(struct HWData *sd);

BOOL initVesaGfxHW(struct HWData *data)
{
    struct BootLoaderBase *BootLoaderBase;
    struct VesaInfo *vi;

    if ((BootLoaderBase = OpenResource("bootloader.resource")))
    {
	D(bug("[Vesa] Init: Bootloader.resource opened\n"));

	if ((vi = (struct VesaInfo *)GetBootInfo(BL_Video)))
	{
	    D(bug("[Vesa] Init: Got Vesa structure from resource\n"));
	    if (vi->ModeNumber == 3)
	    {
		D(bug("[Vesa] Init: Textmode was specified. Aborting\n"));
		return FALSE;
	    }

	    data->width = vi->XSize; data->height = vi->YSize;
	    data->bitsperpixel = data->depth = vi->BitsPerPixel;
	    data->bytesperline = vi->BytesPerLine;
	    data->redmask = vi->Masks[VI_Red];
	    data->greenmask = vi->Masks[VI_Green];
	    data->bluemask = vi->Masks[VI_Blue];
	    data->redshift = vi->Shifts[VI_Red];
	    data->greenshift = vi->Shifts[VI_Green];
	    data->blueshift = vi->Shifts[VI_Blue];
	    data->framebuffer = vi->FrameBuffer;
	    data->palettewidth = vi->PaletteWidth;

	    if (!data->framebuffer)
		Find_PCI_Card(data);
	    if (!data->framebuffer) {
		D(bug("[Vesa] HwInit: Framebuffer not found\n"));
		return FALSE;
	    }
	    
	    if (data->depth > 24)
	    {
	    	data->bytesperpixel = 4;
	    }
	    else if (data->depth > 16)
	    {
	    	data->bytesperpixel = 3;
	    }
	    else if (data->depth > 8)
	    {
	    	data->bytesperpixel = 2;
	    }
	    else
	    {
	    	data->bytesperpixel = 1;
	    }
	    D(bug("[Vesa] HwInit: Clearing %d kB of framebuffer at 0x%08x"
		" size %d kB\n", data->height * data->bytesperline >> 10,
		data->framebuffer, vi->FrameBufferSize));
            ClearBuffer(data);
	    D(bug("[Vesa] HwInit: Linear framebuffer at 0x%08x\n",data->framebuffer));
	    D(bug("[Vesa] HwInit: Screenmode %dx%dx%d\n",data->width,data->height,data->depth));
	    D(bug("[Vesa] HwInit: Masks R %08x<<%2d G %08x<<%2d B %08x<<%2d\n",
			data->redmask, data->redshift,
			data->greenmask, data->greenshift,
			data->bluemask, data->blueshift));
	    D(bug("[Vesa] HwInit: PaletteWidth %d\n", data->palettewidth));
	    D(bug("[vesa] HwInit: BytesPerPixel %d\n", data->bytesperpixel));
	    return TRUE;
	}
	
    }

    bug("[Vesa] HwInit: No Vesa information from the bootloader. Failing\n");
    return FALSE;
}

void vesaDoRefreshArea(struct HWData *hwdata, struct BitmapData *data,
		       LONG x1, LONG y1, LONG x2, LONG y2)
{
    UBYTE *src, *dst;
    ULONG srcmod, dstmod;
    LONG y, w, h, sx, sy;

    x1 += data->xoffset;
    y1 += data->yoffset;
    x2 += data->xoffset;
    y2 += data->yoffset;

    /* Clip the rectangle against physical display borders */
    if ((x1 >= data->disp_width) || (x2 < 1) ||
        (y1 >= data->disp_height) || (y2 < 1))
	return;
    if (x1 < 0)
	x1 = 0;
    if (y1 < 0)
	y1 = 0;
    if (x2 > data->disp_width)
	x2 = data->disp_width;
    if (y2 > data->disp_height)
	y2 = data->disp_height;

    /* Calculate width and height */
    w = x2 - x1;
    h = y2 - y1;

    /* Jump back to bitmap coordinates (adjusted) */
    sx = x1 - data->xoffset;
    sy = y1 - data->yoffset;

    w *= data->bytesperpix;

    srcmod = data->bytesperline;
    dstmod = hwdata->bytesperline;

    src = data->VideoData + sy * data->bytesperline + sx * data->bytesperpix;
    dst = hwdata->framebuffer + y1 * hwdata->bytesperline + x1 * hwdata->bytesperpixel;

    /*
     * Disable screen debug output if not done yet.
     * TODO: develop some mechanism to tell kernel that we actually
     * didn't change the mode, so upon warm reboot it can reuse
     * the framebuffer for debug output.
     */
    if (!hwdata->owned)
    {
    	RawPutChar(0x03);
    	hwdata->owned = TRUE;
    }

    /*
    ** common sense assumption: memcpy can't possibly be faster than CopyMem[Quick]
    */
    if ((srcmod != dstmod) || (srcmod != w))
    {
	for(y = 0; y < h; y++)
	{
	    CopyMem(src, dst, w);
	    src += srcmod;
	    dst += dstmod;
	}
    }
    else
    {
	/* this is a plain total fast rulez copy */
	CopyMem(src, dst, w*h);
    }
}

AROS_UFH3(void, Enumerator,
    AROS_UFHA(struct Hook *,	hook,	    A0),
    AROS_UFHA(OOP_Object *,	pciDevice,  A2),
    AROS_UFHA(APTR,		message,    A1))
{
    AROS_USERFUNC_INIT

    APTR buf;
    IPTR size;
    IPTR Vendor;
    OOP_Object *driver;
    struct pHidd_PCIDriver_MapPCI mappci,*msg = &mappci;
    struct HWData *sd = hook->h_Data;

    D(bug("[VESA] Enumerator: Found device\n"));

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&driver);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &Vendor);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, (APTR)&buf);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0, &size);

    /* BIOS of S3 video cards may forget to set up linear framebuffer start address.
       Here we do this manually.
       This thing was looked up in x.org S3 driver source code. Applicable to all S3 cards. */
    if (Vendor == PCI_VENDOR_S3) {
		outb(0x59, vgaIOBase + 4);
		outb((IPTR)buf >> 24, vgaIOBase + 5);  
		outb(0x5A, vgaIOBase + 4);
		outb((IPTR)buf >> 16, vgaIOBase + 5);
    }

    mappci.mID = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_MapPCI);
    mappci.PCIAddress = buf;
    mappci.Length = size;
    sd->framebuffer = (APTR)OOP_DoMethod(driver, (OOP_Msg)msg);

    D(bug("[VESA] Got framebuffer @ %x (size=%x)\n", sd->framebuffer, size));

    AROS_USERFUNC_EXIT
}

static void Find_PCI_Card(struct HWData *sd)
{
    OOP_Object *pci;

    D(bug("[VESA] Find_PCI_Card\n"));

    sd->pciDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    if (sd->pciDeviceAttrBase)
    {
	pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
	
	D(bug("[VESA] Creating PCI object\n"));

	if (pci)
	{
	    struct Hook FindHook = {
		h_Entry:    (IPTR (*)())Enumerator,
		h_Data:	    sd,
	    };

	    struct TagItem Requirements[] = {
		{ tHidd_PCI_Interface,	0x00 },
		{ tHidd_PCI_Class,	0x03 },
		{ tHidd_PCI_SubClass,	0x00 },
		{ TAG_DONE, 0UL }
	    };
	
	    struct pHidd_PCI_EnumDevices enummsg = {
		mID:		OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
		callback:	&FindHook,
		requirements:	(struct TagItem*)&Requirements,
	    }, *msg = &enummsg;
	    D(bug("[VESA] Calling search Hook\n"));
	    OOP_DoMethod(pci, (OOP_Msg)msg);
	    OOP_DisposeObject(pci);
	}

	OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    }
}

/*
** DACLoad --
**      load a palette
*/
void DACLoad(struct VesaGfx_staticdata *xsd, UBYTE *DAC,
	     unsigned char first, int num)
{
    int i, n;

    n = first * 3;
    
    ObtainSemaphore(&xsd->HW_acc);
    outb(first, 0x3C8);
    for (i=0; i<num*3; i++)
    {
	outb(DAC[n++], 0x3C9);
    }
    ReleaseSemaphore(&xsd->HW_acc);
}

/*
** ClearBuffer --
**      clear the screen buffer
*/
void ClearBuffer(struct HWData *data)
{
    IPTR *p, *limit;

    if (!data->owned)
    {
    	RawPutChar(0x03);
    	data->owned = TRUE;
    }

    p = (IPTR *)data->framebuffer;
    limit = (IPTR *)((IPTR)p + data->height * data->bytesperline);
    while (p < limit)
        *p++ = 0;
}
