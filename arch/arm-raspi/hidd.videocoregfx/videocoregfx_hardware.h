#ifndef _VIDEOCOREGFX_HARDWARE_H
#define _VIDEOCOREGFX_HARDWARE_H

#include <exec/libraries.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <asm/bcm2835.h>
#include <hardware/videocore.h>

#include "videocoregfx_class.h"
#include "videocoregfx_bitmap.h"
#include "videocoregfx_mouse.h"

#define VCMB_FBCHAN     8

struct HWData  {
	ULONG bytesperline;
	APTR vrambase;
/*	UWORD indexReg;
	UWORD valueReg;
	ULONG depth;
	ULONG redmask;
	ULONG greenmask;
	ULONG bluemask;
	ULONG redshift;
	ULONG greenshift;
	ULONG blueshift;
	ULONG bytesperpixel;
	ULONG bitsperpixel;
	ULONG vramsize;
	ULONG maxwidth;
	ULONG maxheight;
        ULONG displaycount;
	ULONG mmiosize;
	APTR mmiobase;
	ULONG fboffset; // last byte in framebuffer of current screen mode
	ULONG pseudocolor;
	ULONG capabilities;*/
};

#define VCDATA(x) ((struct HWData *)x->data)

#define FNAME_HW(x) VideoCoreGfx__HW__ ## x

BOOL FNAME_HW(InitGfxHW)(APTR param0);
void FNAME_HW(RefreshArea)(struct HWData *hwdata,
                            struct BitmapData *data,
                            LONG x1, LONG y1, LONG x2, LONG y2);

#endif /* _VIDEOCOREGFX_HARDWARE_H */
