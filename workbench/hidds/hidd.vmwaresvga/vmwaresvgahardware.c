/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: vmware svga hardware functions
    Lang: English
*/

#define DEBUG 1 /* no SysBase */
#include <aros/debug.h>

#include <asm/io.h>

#include "vmwaresvgahardware.h"
#include "svga_reg.h"
#include "vmwaresvgaclass.h"

ULONG vmwareReadReg(struct HWData *data, ULONG reg)
{
    outl(reg, data->indexReg);
    return inl(data->valueReg);
}

void vmwareWriteReg(struct HWData *data, ULONG reg, ULONG val)
{
    outl(reg, data->indexReg);
    outl(val, data->valueReg);
}

#undef SysBase
extern struct ExecBase *SysBase;

ULONG getVMWareSVGAID(struct HWData *data)
{
    ULONG id;

    vmwareWriteReg(data, SVGA_REG_ID, SVGA_ID_2);
    id = vmwareReadReg(data, SVGA_REG_ID);
    if (id == SVGA_ID_2)
        return id;
    vmwareWriteReg(data, SVGA_REG_ID, SVGA_ID_1);
    id = vmwareReadReg(data, SVGA_REG_ID);
    if (id == SVGA_ID_1)
        return id;
    if (id == SVGA_ID_0)
        return id;
    return SVGA_ID_INVALID;
}

VOID initVMWareSVGAFIFO(struct HWData *data)
{
    ULONG *fifo;
    ULONG fifomin;

    vmwareWriteReg(data, SVGA_REG_CONFIG_DONE, 0);		//Stop vmware from reading the fifo

    fifo = data->mmiobase = (APTR)(IPTR)vmwareReadReg(data, SVGA_REG_MEM_START);
    data->mmiosize = vmwareReadReg(data, SVGA_REG_MEM_SIZE) & ~3;

    if (data->capabilities & SVGA_CAP_EXTENDED_FIFO)
        fifomin = vmwareReadReg(data, SVGA_REG_MEM_REGS);
    else
        fifomin =4;
    
    fifo[SVGA_FIFO_MIN] = fifomin * sizeof(ULONG);

    /* reduce size of FIFO queue to prevent VMWare from failing */
    fifo[SVGA_FIFO_MAX] = (data->mmiosize > 65536) ? 65536 : data->mmiosize; 
    fifo[SVGA_FIFO_NEXT_CMD] = fifomin * sizeof(ULONG);
    fifo[SVGA_FIFO_STOP] = fifomin * sizeof(ULONG);

    vmwareWriteReg(data, SVGA_REG_CONFIG_DONE, 1);
}

VOID syncVMWareSVGAFIFO(struct HWData *data)
{
    vmwareWriteReg(data, SVGA_REG_SYNC, 1);
    while (vmwareReadReg(data, SVGA_REG_BUSY) != 0);
    /* FIXME: maybe wait (delay) some time */
}

VOID writeVMWareSVGAFIFO(struct HWData *data, ULONG val)
{
    ULONG *fifo;

    fifo = data->mmiobase;
    if (
            (fifo[SVGA_FIFO_NEXT_CMD]+4 == fifo[SVGA_FIFO_STOP]) ||
            (
                (fifo[SVGA_FIFO_NEXT_CMD] == (fifo[SVGA_FIFO_MAX]-4)) &&
                (fifo[SVGA_FIFO_STOP] == fifo[SVGA_FIFO_MIN])
            )
        )
        syncVMWareSVGAFIFO(data);

    fifo[fifo[SVGA_FIFO_NEXT_CMD] / 4] = val;
    fifo[SVGA_FIFO_NEXT_CMD] += 4;

    if (fifo[SVGA_FIFO_NEXT_CMD] == fifo[SVGA_FIFO_MAX])
        fifo[SVGA_FIFO_NEXT_CMD] = fifo[SVGA_FIFO_MIN];
}

BOOL initVMWareSVGAHW(struct HWData *data, OOP_Object *device)
{
    ULONG id;

    id = getVMWareSVGAID(data);
    if ((id == SVGA_ID_0) || (id == SVGA_ID_INVALID))
    {
        return FALSE;
    }

    initVMWareSVGAFIFO(data);

    data->capabilities = vmwareReadReg(data, SVGA_REG_CAPABILITIES);

    if (data->capabilities & SVGA_CAP_8BIT_EMULATION)
    {
        data->bitsperpixel = vmwareReadReg(data, SVGA_REG_HOST_BITS_PER_PIXEL);
        vmwareWriteReg(data,SVGA_REG_BITS_PER_PIXEL, data->bitsperpixel);
    }
    data->bitsperpixel = vmwareReadReg(data, SVGA_REG_BITS_PER_PIXEL);

    data->depth = vmwareReadReg(data, SVGA_REG_DEPTH);
    data->maxwidth = vmwareReadReg(data, SVGA_REG_MAX_WIDTH);
    data->maxheight = vmwareReadReg(data, SVGA_REG_MAX_HEIGHT);
    data->redmask = vmwareReadReg(data, SVGA_REG_RED_MASK);
    data->greenmask = vmwareReadReg(data, SVGA_REG_GREEN_MASK);
    data->bluemask = vmwareReadReg(data, SVGA_REG_BLUE_MASK);
    data->bytesperpixel = 1;

    if (data->depth>16)
        data->bytesperpixel = 4;
    else if (data->depth>8)
        data->bytesperpixel = 2;

    if (data->capabilities & SVGA_CAP_MULTIMON)
    {
        data->displaycount = vmwareReadReg(data, SVGA_REG_NUM_DISPLAYS);
    }
    else
    {
        data->displaycount = 1;
    }

    data->vramsize = vmwareReadReg(data, SVGA_REG_VRAM_SIZE);
    data->vrambase = (APTR)(IPTR)vmwareReadReg(data, SVGA_REG_FB_START);
    data->pseudocolor = vmwareReadReg(data, SVGA_REG_PSEUDOCOLOR);

    D(bug("[VMWareSVGA] Init: VRAM at 0x%08x size %d\n",data->vrambase, data->vramsize));
    D(bug("[VMWareSVGA] Init: no.displays: %d\n",data->displaycount));
    D(bug("[VMWareSVGA] Init: caps : 0x%08x\n",data->capabilities));
    D(bug("[VMWareSVGA] Init: no.displays: %d\n",data->displaycount));
    D(bug("[VMWareSVGA] Init: depth: %d\n",data->depth));
    D(bug("[VMWareSVGA] Init: bpp  : %d\n",data->bitsperpixel));
    D(bug("[VMWareSVGA] Init: maxw: %d\n",data->maxwidth));
    D(bug("[VMWareSVGA] Init: maxh: %d\n",data->maxheight));

    return TRUE;
}

VOID setModeVMWareSVGA(struct HWData *data, ULONG width, ULONG height)
{
    D(bug("[VMWareSVGA] SetMode: %dx%d\n",width,height));
    vmwareWriteReg(data, SVGA_REG_ENABLE, 0);
    vmwareWriteReg(data, SVGA_REG_WIDTH, width);
    vmwareWriteReg(data, SVGA_REG_HEIGHT, height);

    if (data->capabilities & SVGA_CAP_8BIT_EMULATION)
        vmwareWriteReg(data, SVGA_REG_BITS_PER_PIXEL,data->bitsperpixel);

    vmwareWriteReg(data, SVGA_REG_ENABLE, 1);

    data->fboffset = vmwareReadReg(data, SVGA_REG_FB_OFFSET);
    data->bytesperline = vmwareReadReg(data, SVGA_REG_BYTES_PER_LINE);
    data->depth = vmwareReadReg(data, SVGA_REG_DEPTH);
    data->redmask = vmwareReadReg(data, SVGA_REG_RED_MASK);
    data->greenmask = vmwareReadReg(data, SVGA_REG_GREEN_MASK);
    data->bluemask = vmwareReadReg(data, SVGA_REG_BLUE_MASK);
    data->pseudocolor = vmwareReadReg(data, SVGA_REG_PSEUDOCOLOR);
}

VOID refreshAreaVMWareSVGA(struct HWData *data, struct Box *box)
{
    writeVMWareSVGAFIFO(data, SVGA_CMD_UPDATE);
    writeVMWareSVGAFIFO(data, box->x1);
    writeVMWareSVGAFIFO(data, box->y1);
    writeVMWareSVGAFIFO(data, box->x2-box->x1+1);
    writeVMWareSVGAFIFO(data, box->y2-box->y1+1);
}

VOID rectFillVMWareSVGA(struct HWData *data, ULONG color, LONG x, LONG y, LONG width, LONG height)
{
    writeVMWareSVGAFIFO(data, SVGA_CMD_RECT_FILL);
    writeVMWareSVGAFIFO(data, color);
    writeVMWareSVGAFIFO(data, x);
    writeVMWareSVGAFIFO(data, y);
    writeVMWareSVGAFIFO(data, width);
    writeVMWareSVGAFIFO(data, height);
    syncVMWareSVGAFIFO(data);
}

VOID ropFillVMWareSVGA(struct HWData *data, ULONG color, LONG x, LONG y, LONG width, LONG height, ULONG mode)
{
    writeVMWareSVGAFIFO(data, SVGA_CMD_RECT_ROP_FILL);
    writeVMWareSVGAFIFO(data, color);
    writeVMWareSVGAFIFO(data, x);
    writeVMWareSVGAFIFO(data, y);
    writeVMWareSVGAFIFO(data, width);
    writeVMWareSVGAFIFO(data, height);
    writeVMWareSVGAFIFO(data, mode);
    syncVMWareSVGAFIFO(data);
}

VOID ropCopyVMWareSVGA(struct HWData *data, LONG sx, LONG sy, LONG dx, LONG dy, ULONG width, ULONG height, ULONG mode)
{
    writeVMWareSVGAFIFO(data, SVGA_CMD_RECT_ROP_COPY);
    writeVMWareSVGAFIFO(data, sx);
    writeVMWareSVGAFIFO(data, sy);
    writeVMWareSVGAFIFO(data, dx);
    writeVMWareSVGAFIFO(data, dy);
    writeVMWareSVGAFIFO(data, width);
    writeVMWareSVGAFIFO(data, height);
    writeVMWareSVGAFIFO(data, mode);
    syncVMWareSVGAFIFO(data);
}

VOID defineCursorVMWareSVGA(struct HWData *data, struct MouseData *mouse)
{
    int i;
    ULONG *cshape = mouse->shape;
    ULONG andmask[SVGA_PIXMAP_SIZE(mouse->width, mouse->height, data->bitsperpixel)];
    ULONG *a, *b;
    UWORD *aw, *bw;

    /* TODO: convert mouse shape to current depth */
    writeVMWareSVGAFIFO(data, SVGA_CMD_DEFINE_CURSOR);
    writeVMWareSVGAFIFO(data, 1);
    writeVMWareSVGAFIFO(data, 0); /* hot x value */
    writeVMWareSVGAFIFO(data, 0); /* hot y value */
    writeVMWareSVGAFIFO(data, mouse->width); /* width */
    writeVMWareSVGAFIFO(data, mouse->height); /* height */
    writeVMWareSVGAFIFO(data, data->bitsperpixel); /* bits per pixel */
    writeVMWareSVGAFIFO(data, data->bitsperpixel); /* bits per pixel */
    b = cshape;
    a = andmask;
    aw = (UWORD *)a;
    bw = (UWORD *)b;
    for (i = 0; i<(SVGA_PIXMAP_SIZE(mouse->width, mouse->height, data->bitsperpixel)*2);i++)
    {
        *aw = *bw ? 0 : ~0;
        
        aw++;
        bw++;
    }
    a = andmask;
    for (i = 0; i<SVGA_PIXMAP_SIZE(mouse->width, mouse->height, data->bitsperpixel);i++) {
        writeVMWareSVGAFIFO(data, *a++);
    }
    for (i = 0; i<SVGA_PIXMAP_SIZE(mouse->width, mouse->height, data->bitsperpixel);i++)
        writeVMWareSVGAFIFO(data, *cshape++);
    syncVMWareSVGAFIFO(data);
}

VOID displayCursorVMWareSVGA(struct HWData *data, LONG mode)
{
#if 0
    writeVMWareSVGAFIFO(data, SVGA_CMD_DISPLAY_CURSOR);
    writeVMWareSVGAFIFO(data, 1);
    writeVMWareSVGAFIFO(data, mode);
    syncVMWareSVGAFIFO(data);
#else
    vmwareWriteReg(data, SVGA_REG_CURSOR_ID, 1);
    vmwareWriteReg(data, SVGA_REG_CURSOR_ON, mode);
#endif
}

VOID moveCursorVMWareSVGA(struct HWData *data, LONG x, LONG y)
{
#if 0
    writeVMWareSVGAFIFO(data, SVGA_CMD_MOVE_CURSOR);
    writeVMWareSVGAFIFO(data, x);
    writeVMWareSVGAFIFO(data, y);
    syncVMWareSVGAFIFO(data);
#else
    vmwareWriteReg(data, SVGA_REG_CURSOR_ID, 1);
    vmwareWriteReg(data, SVGA_REG_CURSOR_X, x);
    vmwareWriteReg(data, SVGA_REG_CURSOR_Y, y);
    vmwareWriteReg(data, SVGA_REG_CURSOR_ON, 1);
#endif
}
