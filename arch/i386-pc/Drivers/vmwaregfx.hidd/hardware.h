#ifndef HARDWARE_H
#define HARDWARE_H

#include <exec/libraries.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include "bitmap.h"
#include "mouse.h"
#include "svga_reg.h"

#define VENDOR_VMWARE 0x15ad
#define DEVICE_VMWARE0710   0x0710
#define DEVICE_VMWARE0405   0x0405

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
	ULONG maxheigt;
	ULONG mmiosize;
	APTR mmiobase;
	ULONG fboffset; /* last byte in framebuffer of current screen mode */
	ULONG pseudocolor;
	ULONG capabilities;
};

#define clearCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_CLEAR)
#define andCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_AND)
#define andReverseCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_AND_REVERSE)
#define copyCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_COPY)
#define andInvertedCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_AND_INVERTED)
#define noOpCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_NOOP)
#define xorCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_XOR)
#define orCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_OR)
#define norCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_NOR)
#define equivCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_EQUIV)
#define invertCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_INVERT)
#define orReverseCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_OR_REVERSE)
#define copyInvertedCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_COPY_INVERTED)
#define orInvertedCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_OR_INVERTED)
#define nandCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_NAND)
#define setCopyVMWareGfx(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareGfx(d, sx, sy, dx, dy, w, h, SVGA_ROP_SET)

#define clearFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_CLEAR)
#define andFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_AND)
#define andReverseFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_AND_REVERSE)
#define copyFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_COPY)
#define andInvertedFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_AND_INVERTED)
#define noOpFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_NOOP)
#define xorFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_XOR)
#define orFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_OR)
#define norFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_NOR)
#define equivFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_EQUIV)
#define invertFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_INVERT)
#define orReverseFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_OR_REVERSE)
#define copyInvertedFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_COPY_INVERTED)
#define orInvertedFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_OR_INVERTED)
#define nandFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_NAND)
#define setFillVMWareGfx(d, c, x, y, w, h) \
	ropFillVMWareGfx(d, c, x, y, w, h, SVGA_ROP_SET)


void vmwareWriteReg(struct HWData *, ULONG, ULONG);
VOID writeVMWareGfxFIFO(struct HWData *, ULONG);
VOID syncVMWareGfxFIFO(struct HWData *);
BOOL initVMWareGfxHW(struct HWData *, OOP_Object *);
VOID setModeVMWareGfx(struct HWData *, ULONG, ULONG);
VOID refreshAreaVMWareGfx(struct HWData *, struct Box *);
VOID rectFillVMWareGfx(struct HWData *, ULONG, LONG, LONG, LONG, LONG);
VOID ropFillVMWareGfx(struct HWData *, ULONG, LONG, LONG, LONG, LONG, ULONG);
VOID ropCopyVMWareGfx(struct HWData *, LONG, LONG, LONG, LONG, ULONG, ULONG, ULONG);
VOID defineCursorVMWareGfx(struct HWData *, struct MouseData *);
VOID displayCursorVMWareGfx(struct HWData *, LONG);
VOID moveCursorVMWareGfx(struct HWData *, LONG, LONG);

#endif
