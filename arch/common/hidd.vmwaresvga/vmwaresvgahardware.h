#ifndef _VMWARESVGA_HARDWARE_H
#define _VMWARESVGA_HARDWARE_H

#include <exec/libraries.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include "vmwaresvgabitmap.h"
#include "vmwaresvgamouse.h"
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
	ULONG maxheight;
    ULONG displaycount;
	ULONG mmiosize;
	APTR mmiobase;
	ULONG fboffset; /* last byte in framebuffer of current screen mode */
	ULONG pseudocolor;
	ULONG capabilities;
};

#define clearCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_CLEAR)
#define andCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_AND)
#define andReverseCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_AND_REVERSE)
#define copyCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_COPY)
#define andInvertedCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_AND_INVERTED)
#define noOpCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_NOOP)
#define xorCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_XOR)
#define orCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_OR)
#define norCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_NOR)
#define equivCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_EQUIV)
#define invertCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_INVERT)
#define orReverseCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_OR_REVERSE)
#define copyInvertedCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_COPY_INVERTED)
#define orInvertedCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_OR_INVERTED)
#define nandCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_NAND)
#define setCopyVMWareSVGA(d, sx, sy, dx, dy, w, h) \
	ropCopyVMWareSVGA(d, sx, sy, dx, dy, w, h, SVGA_ROP_SET)

#define clearFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_CLEAR)
#define andFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_AND)
#define andReverseFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_AND_REVERSE)
#define copyFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_COPY)
#define andInvertedFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_AND_INVERTED)
#define noOpFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_NOOP)
#define xorFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_XOR)
#define orFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_OR)
#define norFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_NOR)
#define equivFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_EQUIV)
#define invertFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_INVERT)
#define orReverseFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_OR_REVERSE)
#define copyInvertedFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_COPY_INVERTED)
#define orInvertedFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_OR_INVERTED)
#define nandFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_NAND)
#define setFillVMWareSVGA(d, c, x, y, w, h) \
	ropFillVMWareSVGA(d, c, x, y, w, h, SVGA_ROP_SET)


void vmwareWriteReg(struct HWData *, ULONG, ULONG);
VOID writeVMWareSVGAFIFO(struct HWData *, ULONG);
VOID syncVMWareSVGAFIFO(struct HWData *);
BOOL initVMWareSVGAHW(struct HWData *, OOP_Object *);
VOID setModeVMWareSVGA(struct HWData *, ULONG, ULONG);
VOID refreshAreaVMWareSVGA(struct HWData *, struct Box *);
VOID rectFillVMWareSVGA(struct HWData *, ULONG, LONG, LONG, LONG, LONG);
VOID ropFillVMWareSVGA(struct HWData *, ULONG, LONG, LONG, LONG, LONG, ULONG);
VOID ropCopyVMWareSVGA(struct HWData *, LONG, LONG, LONG, LONG, ULONG, ULONG, ULONG);
VOID defineCursorVMWareSVGA(struct HWData *, struct MouseData *);
VOID displayCursorVMWareSVGA(struct HWData *, LONG);
VOID moveCursorVMWareSVGA(struct HWData *, LONG, LONG);

#endif /* _VMWARESVGA_HARDWARE_H */
