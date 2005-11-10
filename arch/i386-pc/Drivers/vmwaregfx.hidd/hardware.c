/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: vmware svga hardware functions
    Lang: English
*/


#include <asm/io.h>
#define DEBUG 0 /* no SysBase */
#include <aros/debug.h>

#include "hardware.h"
#include "svga_reg.h"
#include "vmwaregfxclass.h"

ULONG vmwareReadReg(struct HWData *data, ULONG reg) {
	outl(reg, data->indexReg);
	return inl(data->valueReg);
}

void vmwareWriteReg(struct HWData *data, ULONG reg, ULONG val) {
	outl(reg, data->indexReg);
	outl(val, data->valueReg);
}

#undef SysBase
extern struct ExecBase *SysBase;

ULONG getVMWareSVGAID(struct HWData *data) {
ULONG id;

	vmwareWriteReg(data, SVGA_REG_ID, SVGA_ID_2);
	id = vmwareReadReg(data, SVGA_REG_ID);
	if (id == SVGA_ID_2)
		return id;
	vmwareWriteReg(data, SVGA_REG_ID, SVGA_ID_1);
	id = vmwareReadReg(data, SVGA_REG_ID);
	if (id == SVGA_ID_1)
		return id;
//	vmwareWriteReg(data, SVGA_REG_ID, SVGA_ID_0);
//	id = vmwareReadReg(data, SVGA_REG_ID);
	if (id == SVGA_ID_0)
		return id;
	return SVGA_ID_INVALID;
}

VOID initVMWareGfxFIFO(struct HWData *data) {
ULONG *fifo;

	fifo = data->mmiobase = vmwareReadReg(data, SVGA_REG_MEM_START);
	data->mmiosize = vmwareReadReg(data, SVGA_REG_MEM_SIZE) & ~3;
	fifo[SVGA_FIFO_MIN] = 16;
	fifo[SVGA_FIFO_MAX] = data->mmiosize;
	fifo[SVGA_FIFO_NEXT_CMD] = 16;
	fifo[SVGA_FIFO_STOP] = 16;
	vmwareWriteReg(data, SVGA_REG_CONFIG_DONE, 1);
}

VOID syncVMWareGfxFIFO(struct HWData *data) {

	vmwareWriteReg(data, SVGA_REG_SYNC, 1);
	while (vmwareReadReg(data, SVGA_REG_BUSY) != 0);
#warning "maybe wait (delay) some time"
}

VOID writeVMWareGfxFIFO(struct HWData *data, ULONG val) {
ULONG *fifo;

	fifo = data->mmiobase;
	if (
			(fifo[SVGA_FIFO_NEXT_CMD]+4 == fifo[SVGA_FIFO_STOP]) ||
			(
				(fifo[SVGA_FIFO_NEXT_CMD] == (fifo[SVGA_FIFO_MAX]-4)) &&
				(fifo[SVGA_FIFO_STOP] == fifo[SVGA_FIFO_MIN])
			)
		)
		syncVMWareGfxFIFO(data);
	fifo[fifo[SVGA_FIFO_NEXT_CMD] / 4] = val;
	fifo[SVGA_FIFO_NEXT_CMD] += 4;
	if (fifo[SVGA_FIFO_NEXT_CMD] == fifo[SVGA_FIFO_MAX])
		fifo[SVGA_FIFO_NEXT_CMD] = fifo[SVGA_FIFO_MIN];
}

BOOL initVMWareGfxHW(struct HWData *data, OOP_Object *device) {
ULONG *ba;
ULONG id;

	id = getVMWareSVGAID(data);
	if ((id == SVGA_ID_0) || (id == SVGA_ID_INVALID))
	{
		return FALSE;
	}
	initVMWareGfxFIFO(data);
	data->depth = vmwareReadReg(data, SVGA_REG_DEPTH);
	data->redmask = vmwareReadReg(data, SVGA_REG_RED_MASK);
	data->greenmask = vmwareReadReg(data, SVGA_REG_GREEN_MASK);
	data->bluemask = vmwareReadReg(data, SVGA_REG_BLUE_MASK);
	data->bytesperpixel = 1;
	if (data->depth>16)
		data->bytesperpixel = 4;
	else if (data->depth>8)
		data->bytesperpixel = 2;
	data->bitsperpixel = vmwareReadReg(data, SVGA_REG_BITS_PER_PIXEL);
	data->vramsize = vmwareReadReg(data, SVGA_REG_FB_MAX_SIZE);
	data->vrambase = vmwareReadReg(data, SVGA_REG_FB_START);
	data->pseudocolor = vmwareReadReg(data, SVGA_REG_PSEUDOCOLOR);
	return TRUE;
}

VOID setModeVMWareGfx(struct HWData *data, ULONG width, ULONG height) {

	vmwareWriteReg(data, SVGA_REG_WIDTH, width);
	vmwareWriteReg(data, SVGA_REG_HEIGHT, height);
	data->fboffset = vmwareReadReg(data, SVGA_REG_FB_OFFSET);
	/* bytes per line, green mask, red mask? */
	vmwareWriteReg(data, SVGA_REG_ENABLE, 1);
	data->bytesperline = vmwareReadReg(data, SVGA_REG_BYTES_PER_LINE);
}

VOID refreshAreaVMWareGfx(struct HWData *data, struct Box *box) {
	writeVMWareGfxFIFO(data, SVGA_CMD_UPDATE);
	writeVMWareGfxFIFO(data, box->x1);
	writeVMWareGfxFIFO(data, box->y1);
	writeVMWareGfxFIFO(data, box->x2-box->x1+1);
	writeVMWareGfxFIFO(data, box->y2-box->y1+1);
}

VOID rectFillVMWareGfx(struct HWData *data, ULONG color, LONG x, LONG y, LONG width, LONG height) {

	writeVMWareGfxFIFO(data, SVGA_CMD_RECT_FILL);
	writeVMWareGfxFIFO(data, color);
	writeVMWareGfxFIFO(data, x);
	writeVMWareGfxFIFO(data, y);
	writeVMWareGfxFIFO(data, width);
	writeVMWareGfxFIFO(data, height);
	syncVMWareGfxFIFO(data);
}

VOID ropFillVMWareGfx(struct HWData *data, ULONG color, LONG x, LONG y, LONG width, LONG height, ULONG mode) {

	writeVMWareGfxFIFO(data, SVGA_CMD_RECT_ROP_FILL);
	writeVMWareGfxFIFO(data, color);
	writeVMWareGfxFIFO(data, x);
	writeVMWareGfxFIFO(data, y);
	writeVMWareGfxFIFO(data, width);
	writeVMWareGfxFIFO(data, height);
	writeVMWareGfxFIFO(data, mode);
	syncVMWareGfxFIFO(data);
}

VOID ropCopyVMWareGfx(struct HWData *data, LONG sx, LONG sy, LONG dx, LONG dy, ULONG width, ULONG height, ULONG mode) {

	writeVMWareGfxFIFO(data, SVGA_CMD_RECT_ROP_COPY);
	writeVMWareGfxFIFO(data, sx);
	writeVMWareGfxFIFO(data, sy);
	writeVMWareGfxFIFO(data, dx);
	writeVMWareGfxFIFO(data, dy);
	writeVMWareGfxFIFO(data, width);
	writeVMWareGfxFIFO(data, height);
	writeVMWareGfxFIFO(data, mode);
	syncVMWareGfxFIFO(data);
}

VOID defineCursorVMWareGfx(struct HWData *data, struct MouseData *mouse) {
int i;
ULONG *cshape = mouse->shape;
struct Box box;
ULONG andmask[SVGA_PIXMAP_SIZE(mouse->width, mouse->height, data->bitsperpixel)];
ULONG *a;
ULONG *b;


#warning "convert mouse shape to current depth"
	writeVMWareGfxFIFO(data, SVGA_CMD_DEFINE_CURSOR);
	writeVMWareGfxFIFO(data, 1);
	writeVMWareGfxFIFO(data, 0); /* hot x value */
	writeVMWareGfxFIFO(data, 0); /* hot y value */
	writeVMWareGfxFIFO(data, mouse->width); /* width */
	writeVMWareGfxFIFO(data, mouse->height); /* height */
	writeVMWareGfxFIFO(data, data->bitsperpixel); /* bits per pixel */
	writeVMWareGfxFIFO(data, data->bitsperpixel); /* bits per pixel */
	b = cshape;
	a = andmask;
	for (i = 0; i<(SVGA_PIXMAP_SIZE(mouse->width, mouse->height, data->bitsperpixel)*2);i++)
		*((UWORD *)a)++ = *((UWORD *)b)++ ? 0 : ~0;
	a = andmask;
	for (i = 0; i<SVGA_PIXMAP_SIZE(mouse->width, mouse->height, data->bitsperpixel);i++)
		writeVMWareGfxFIFO(data, *a++);
	for (i = 0; i<SVGA_PIXMAP_SIZE(mouse->width, mouse->height, data->bitsperpixel);i++)
		writeVMWareGfxFIFO(data, *cshape++);
	syncVMWareGfxFIFO(data);
}

VOID displayCursorVMWareGfx(struct HWData *data, LONG mode) {
#if 0
	writeVMWareGfxFIFO(data, SVGA_CMD_DISPLAY_CURSOR);
	writeVMWareGfxFIFO(data, 1);
	writeVMWareGfxFIFO(data, mode);
	syncVMWareGfxFIFO(data);
#else
	vmwareWriteReg(data, SVGA_REG_CURSOR_ID, 1);
	vmwareWriteReg(data, SVGA_REG_CURSOR_ON, mode);
#endif
}

VOID moveCursorVMWareGfx(struct HWData *data, LONG x, LONG y) {
#if 0
	writeVMWareGfxFIFO(data, SVGA_CMD_MOVE_CURSOR);
	writeVMWareGfxFIFO(data, x);
	writeVMWareGfxFIFO(data, y);
	syncVMWareGfxFIFO(data);
#else
	vmwareWriteReg(data, SVGA_REG_CURSOR_ID, 1);
	vmwareWriteReg(data, SVGA_REG_CURSOR_X, x);
	vmwareWriteReg(data, SVGA_REG_CURSOR_Y, y);
	vmwareWriteReg(data, SVGA_REG_CURSOR_ON, 1);
#endif
}
