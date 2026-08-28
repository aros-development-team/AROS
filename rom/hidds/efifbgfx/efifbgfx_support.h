#ifndef EFIFBGFX_SUPPORT_H
#define EFIFBGFX_SUPPORT_H

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

struct EFIFBGfx_staticdata;
struct EFIFBGfxBitMapData;

BOOL initEFIFBGfxHW(struct HWData *);
void DACLoad(struct EFIFBGfx_staticdata *, UBYTE *, unsigned char, int);
void ClearBuffer(struct HWData *data);
void efifbDoRefreshArea(struct HWData *hwdata, struct EFIFBGfxBitMapData *data,
		      LONG x1, LONG y1, LONG x2, LONG y2);

#endif /* EFIFBGFX_SUPPORT_H */
