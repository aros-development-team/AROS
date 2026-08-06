#ifndef FBGFX_SUPPORT_H
#define FBGFX_SUPPORT_H

#include <exec/types.h>
#include <oop/oop.h>

#define PCI_VENDOR_S3 0x5333

#define vgaIOBase 0x3d0

struct HWData
{
    APTR	 framebuffer;
    ULONG	 fbsize;
    ULONG	 width;
    ULONG	 height;
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
    UBYTE	 palettewidth;
    UBYTE	 DAC[768];
    /* Used by PCI scanning routine */
    OOP_AttrBase pciDeviceAttrBase;
};

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase sd->pciDeviceAttrBase

struct FBGfx_staticdata;
struct FBGfxBitMapData;

BOOL initFBGfxHW(struct HWData *);
void DACLoad(struct FBGfx_staticdata *, UBYTE *, unsigned char, int);
void ClearBuffer(struct HWData *data);
void fbDoRefreshArea(struct HWData *hwdata, struct FBGfxBitMapData *data,
		       LONG x1, LONG y1, LONG x2, LONG y2);

#endif /* FBGFX_SUPPORT_H */
