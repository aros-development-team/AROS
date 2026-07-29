#ifndef VCFBGFX_SUPPORT_H
#define VCFBGFX_SUPPORT_H

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

struct VCGfx_staticdata;
struct VCGfxBitMapData;

BOOL initVCGfxHW(struct HWData *);
void DACLoad(struct VCGfx_staticdata *, UBYTE *, unsigned char, int);
void ClearBuffer(struct HWData *data);
void vcfbDoRefreshArea(struct HWData *hwdata, struct VCGfxBitMapData *data,
		       LONG x1, LONG y1, LONG x2, LONG y2);

#endif /* VCFBGFX_SUPPORT_H */
