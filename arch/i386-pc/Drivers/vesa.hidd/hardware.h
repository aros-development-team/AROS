#ifndef HARDWARE_H
#define HARDWARE_H

#include <exec/types.h>

#define PCI_VENDOR_S3 0x5333

#define vgaIOBase 0x3d0

struct HWData
{
    APTR	framebuffer;
    ULONG	fbsize;
    ULONG	width;
    ULONG	height;
    ULONG	depth;
    ULONG	bytesperpixel;
    ULONG	bitsperpixel;
    ULONG	redmask;
    ULONG	greenmask;
    ULONG	bluemask;
    ULONG	redshift;
    ULONG	greenshift;
    ULONG	blueshift;
    ULONG	bytesperline;
    UBYTE	palettewidth;
    UBYTE	DAC[768];
};

extern OOP_AttrBase HiddPCIDeviceAttrBase;

BOOL initVesaGfxHW(struct HWData *);
void DACLoad(struct HWData *, unsigned char, int);

#if BUFFERED_VRAM

struct BitmapData;

void vesaRefreshArea(struct BitmapData *data, LONG x1, LONG y1, LONG x2, LONG y2);
#endif

#endif
