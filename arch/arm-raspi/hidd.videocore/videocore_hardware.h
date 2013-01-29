#ifndef _VIDEOCORE_HARDWARE_H
#define _VIDEOCORE_HARDWARE_H

#include <exec/libraries.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <asm/bcm2835.h>
#include <hardware/videocore.h>

#include "videocore_bitmap.h"
#include "videocore_mouse.h"

#ifdef VCMBoxBase
#undef VCMBoxBase
#endif

#define VCMBoxBase      xsd->vcsd_VCMBoxBase
#define VCMsg           xsd->vcsd_VCMBoxMessage
#define VCMB_FBCHAN     8

struct HWData  {
	UWORD indexReg;
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
	ULONG bytesperline;
	ULONG vramsize;
	APTR vrambase;
	ULONG maxwidth;
	ULONG maxheight;
        ULONG displaycount;
	ULONG mmiosize;
	APTR mmiobase;
	ULONG fboffset; /* last byte in framebuffer of current screen mode */
	ULONG pseudocolor;
	ULONG capabilities;
};

#define clearCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_CLEAR)
#define andCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_AND)
#define andReverseCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_AND_REVERSE)
#define copyCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_COPY)
#define andInvertedCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_AND_INVERTED)
#define noOpCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_NOOP)
#define xorCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_XOR)
#define orCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_OR)
#define norCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_NOR)
#define equivCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_EQUIV)
#define invertCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_INVERT)
#define orReverseCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_OR_REVERSE)
#define copyInvertedCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_COPY_INVERTED)
#define orInvertedCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_OR_INVERTED)
#define nandCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_NAND)
#define setCopyVideoCore(d, sx, sy, dx, dy, w, h) \
	ropCopyVideoCore(d, sx, sy, dx, dy, w, h, SVGA_ROP_SET)

#define clearFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_CLEAR)
#define andFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_AND)
#define andReverseFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_AND_REVERSE)
#define copyFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_COPY)
#define andInvertedFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_AND_INVERTED)
#define noOpFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_NOOP)
#define xorFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_XOR)
#define orFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_OR)
#define norFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_NOR)
#define equivFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_EQUIV)
#define invertFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_INVERT)
#define orReverseFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_OR_REVERSE)
#define copyInvertedFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_COPY_INVERTED)
#define orInvertedFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_OR_INVERTED)
#define nandFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_NAND)
#define setFillVideoCore(d, c, x, y, w, h) \
	ropFillVideoCore(d, c, x, y, w, h, SVGA_ROP_SET)


VOID writeVideoCoreFIFO(struct HWData *, ULONG);
VOID syncVideoCoreFIFO(struct HWData *);
BOOL initVideoCoreHW(struct HWData *, OOP_Object *);
VOID setModeVideoCore(struct HWData *, ULONG, ULONG);
VOID refreshAreaVideoCore(struct HWData *, struct Box *);
VOID rectFillVideoCore(struct HWData *, ULONG, LONG, LONG, LONG, LONG);
VOID ropFillVideoCore(struct HWData *, ULONG, LONG, LONG, LONG, LONG, ULONG);
VOID ropCopyVideoCore(struct HWData *, LONG, LONG, LONG, LONG, ULONG, ULONG, ULONG);
VOID defineCursorVideoCore(struct HWData *, struct MouseData *);
VOID displayCursorVideoCore(struct HWData *, LONG);
VOID moveCursorVideoCore(struct HWData *, LONG, LONG);

#endif /* _VIDEOCORE_HARDWARE_H */
