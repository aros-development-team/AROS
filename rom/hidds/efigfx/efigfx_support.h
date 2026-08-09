#ifndef EFIGFX_SUPPORT_H
#define EFIGFX_SUPPORT_H

#include <exec/types.h>
#include <oop/oop.h>

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
    UBYTE	 palettewidth;
};

struct EFIGfx_staticdata;
struct EFIGfxBitMapData;

BOOL initEFIGfxHW(struct HWData *);
void DACLoad(struct EFIGfx_staticdata *, UBYTE *, unsigned char, int);
void ClearBuffer(struct HWData *data);
void efiDoRefreshArea(struct HWData *hwdata, struct EFIGfxBitMapData *data,
		      LONG x1, LONG y1, LONG x2, LONG y2);

#endif /* EFIGFX_SUPPORT_H */
