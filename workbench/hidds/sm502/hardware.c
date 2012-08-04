/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: sm502 "hardware" functions
    Lang: English
*/

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
#include "sm502gfxclass.h"
#include "hardware.h"

#include <string.h>

/* SM502 registers */
#define SM502_PDC       0x080000        /* Panel Display Control */
#define SM502_PCK       0x080008        /* Panel Color Key       */
#define SM502_PFBO      0x080010        /* Panel FB Offset       */
#define SM502_PFBW      0x080014        /* Panel FB Width        */
#define SM502_PFBH      0x080018        /* Panel FB Height       */
#define SM502_PPR       0x080400        /* Panel Palette RAM     */

static BOOL Find_PCI_Card(struct SM502_HWData *hw);

BOOL initSM502GfxHW(struct SM502_HWData *hw)
{
    ULONG mode;

    /* Find the first device */
    if (!Find_PCI_Card(hw)) {
        D(bug("[SM502] No card found\n"));
        return FALSE;
    }

    hw->disp_width = (smread(hw, SM502_PFBO) >> 16) & 0x3fff;
    hw->disp_height = (smread(hw, SM502_PFBH) >> 16) & 0x0fff; 
    hw->xoffset = smread(hw, SM502_PFBW) & 0xfff;
    hw->yoffset = smread(hw, SM502_PFBH) & 0xfff;
    hw->bytesperline = smread(hw, SM502_PFBO) & 0x3fff;
    mode = smread(hw, SM502_PDC);

    /* Convert display width from bytes to pixels */
    if ((mode & 3) == 1) {
        hw->disp_width /= 2;
    }
    if ((mode & 3) == 2) {
        hw->disp_width /= 4;
    }

    /* Switch from 32 bit mode */
    if ((mode & 3) != 2) {
        int i;

        mode = (mode & ~3) | 2;
        smwrite(hw, SM502_PDC, mode);
        smwrite(hw, SM502_PCK, 0);
        hw->bytesperline = ((hw->disp_width * 4) + 127) & ~127;
        smwrite(hw, SM502_PFBO, (hw->bytesperline << 16) | (hw->bytesperline));
        for (i = 0; i < 256; i++)
            smwrite(hw, SM502_PPR + (i << 2), 0x00000000);
    }


    /* Setup the current video parameters */
    hw->bitsperpixel = 32;
    hw->bytesperpixel = 4;
    hw->redmask = 0xff << 8;
    hw->redshift = 16;
    hw->greenmask = 0xff << 16;
    hw->greenshift = 8;
    hw->bluemask = 0xff << 24;
    hw->blueshift = 0;
    hw->depth = 24;

    hw->width   = hw->disp_width - hw->xoffset;
    hw->height  = hw->disp_height - hw->yoffset;
    hw->palettewidth = (hw->depth == 8) ? 256 : 0;

    D(bug("[SM502] HwInit: Clearing %d kB of framebuffer at 0x%08x",
        hw->height * hw->bytesperline >> 10,
        hw->framebuffer));
    ClearBuffer(hw);
    D(bug("[SM502] HwInit: Linear framebuffer at 0x%08x\n",hw->framebuffer));
    D(bug("[SM502] HwInit: Screenmode %dx%dx%d\n",hw->width,hw->height,hw->depth));
    D(bug("[SM502] HwInit: Masks R %08x<<%2d G %08x<<%2d B %08x<<%2d\n",
                hw->redmask, hw->redshift,
                hw->greenmask, hw->greenshift,
                hw->bluemask, hw->blueshift));
    D(bug("[SM502] HwInit: PaletteWidth %d\n", hw->palettewidth));
    D(bug("[sm502] HwInit: BytesPerPixel %d\n", hw->bytesperpixel));
    return TRUE;
}

void sm502DoRefreshArea(struct SM502_HWData *hwdata, struct BitmapData *data,
                       LONG x1, LONG y1, LONG x2, LONG y2)
{
    UBYTE *src, *dst;
    ULONG srcmod, dstmod;
    LONG  y, w, h;
    LONG sx, sy;

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
    /* Jump back to bitmap coordinatess (adjusted) */
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
    AROS_UFHA(struct Hook *,    hook,       A0),
    AROS_UFHA(OOP_Object *,     pciDevice,  A2),
    AROS_UFHA(APTR,             message,    A1))
{
    AROS_USERFUNC_INIT

    APTR buf;
    IPTR size;
    IPTR Vendor;
    OOP_Object *driver;
    struct pHidd_PCIDriver_MapPCI mappci,*msg = &mappci;
    struct SM502_HWData *sd = hook->h_Data;

    D(bug("[SM502] Enumerator: Found device\n"));

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&driver);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &Vendor);

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, (APTR)&buf);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0, &size);

    mappci.mID = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_MapPCI);
    mappci.PCIAddress = buf;
    mappci.Length = size;
    sd->framebuffer = (APTR)OOP_DoMethod(driver, (OOP_Msg)msg);

    D(bug("[SM502] Got framebuffer @ %x (size=%x)\n", sd->framebuffer, size));

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base1, (APTR)&buf);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size1, &size);

    mappci.mID = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_MapPCI);
    mappci.PCIAddress = buf;
    mappci.Length = size;
    sd->mmio = (APTR)OOP_DoMethod(driver, (OOP_Msg)msg);

    D(bug("[SM502] Got mmio regs   @ %x (size=%x)\n", sd->mmio, size));

    AROS_USERFUNC_EXIT
}

static BOOL Find_PCI_Card(struct SM502_HWData *sd)
{
    OOP_Object *pci;

    D(bug("[SM502] Find_PCI_Card\n"));

    sd->framebuffer = sd->mmio = NULL;
    sd->pciDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    if (sd->pciDeviceAttrBase)
    {
        pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
        
        D(bug("[SM502] Creating PCI object\n"));

        if (pci)
        {
            struct Hook FindHook = {
                h_Entry:    (IPTR (*)())Enumerator,
                h_Data:     sd,
            };

            struct TagItem Requirements[] = {
                { tHidd_PCI_VendorID,   0x126f },
                { tHidd_PCI_ProductID,  0x0501 },
                { TAG_DONE, 0UL }
            };
        
            struct pHidd_PCI_EnumDevices enummsg = {
                mID:            OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
                callback:       &FindHook,
                requirements:   (struct TagItem*)&Requirements,
            }, *msg = &enummsg;
            D(bug("[SM502] Calling search Hook\n"));
            OOP_DoMethod(pci, (OOP_Msg)msg);
            OOP_DisposeObject(pci);
        }

        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    }

    return (sd->framebuffer != NULL && sd->mmio != NULL);
}

/*
** DACLoad --
**      load a palette
*/
void DACLoad(struct SM502Gfx_staticdata *xsd, UBYTE *DAC,
             unsigned char first, int num)
{
    int i, n;
    ULONG mode;

    mode = smread(&xsd->data, SM502_PDC);
    if ((mode & 3) != 0)
        return;

    n = first * 3;
    
    ObtainSemaphore(&xsd->HW_acc);
    for (i = 0; i < num; i++, n+=3) {
        ULONG rgb;

        rgb = (DAC[n+0] << 16) |
              (DAC[n+1] <<  8) |
              (DAC[n+2] <<  0);
        smwrite(&xsd->data, SM502_PPR + (i << 2), rgb);
    }
    ReleaseSemaphore(&xsd->HW_acc);
}

/*
** ClearBuffer --
**      clear the screen buffer
*/
void ClearBuffer(struct SM502_HWData *data)
{
    if (!data->owned)
    {
        RawPutChar(0x03);
        data->owned = TRUE;
    }

    memset(data->framebuffer, 0x55, data->height * data->bytesperline);
}
