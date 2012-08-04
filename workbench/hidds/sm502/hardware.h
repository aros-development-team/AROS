#ifndef HARDWARE_H
#define HARDWARE_H

#include <exec/types.h>
#include <oop/oop.h>

struct SM502_HWData
{
    APTR	 mmio;
    APTR	 framebuffer;
    ULONG	 fbsize;
    ULONG	 width;
    ULONG	 height;
    ULONG	 disp_width;
    ULONG	 disp_height;
    ULONG	 xoffset;
    ULONG	 yoffset;
    ULONG	 depth;
    ULONG	 bytesperpixel;
    ULONG	 bitsperpixel;
    ULONG	 redmask;
    ULONG	 greenmask;
    ULONG	 bluemask;
    ULONG	 redshift;
    ULONG	 greenshift;
    ULONG	 blueshift;
    ULONG	 bytesperline;
    BOOL	 owned;
    UWORD	 palettewidth;
    /* Used by PCI scanning routine */
    OOP_AttrBase pciDeviceAttrBase;
};

static inline ULONG smread(struct SM502_HWData *hw, ULONG reg)
{
	return (ULONG)AROS_LE2LONG(*(volatile LONG *)(hw->mmio + reg));
}

static inline VOID smwrite(struct SM502_HWData *hw, ULONG reg, ULONG val)
{
	*(volatile LONG *)(hw->mmio + reg) = AROS_LONG2LE((LONG)val);
}

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase sd->pciDeviceAttrBase

struct SM502Gfx_staticdata;
struct BitmapData;

BOOL initSM502GfxHW(struct SM502_HWData *);
void DACLoad(struct SM502Gfx_staticdata *, UBYTE *, unsigned char, int);
void ClearBuffer(struct SM502_HWData *data);
void sm502DoRefreshArea(struct SM502_HWData *hwdata, struct BitmapData *data,
		       LONG x1, LONG y1, LONG x2, LONG y2);

#endif
