/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: vmware svga hardware functions
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0 /* no SysBase */
#include <aros/debug.h>

#include <asm/io.h>

#include <limits.h>
#include <exec/ports.h>
#include <devices/timer.h>
#include <proto/exec.h>

#include "vmwaresvga_intern.h"

#if (DEBUG0)
#define DIRQ(x)                         x
#define DDMG(x)
#define DFIFO(x)
#define DFIFOINF(x)                     x
#define DFENCE(x)                       x
#define DREFRESH(x)
#define DCURSOR(x)
#else
#define DIRQ(x)
#define DDMG(x)
#define DFIFO(x)
#define DFIFOINF(x)
#define DFENCE(x)
#define DREFRESH(x)
#define DCURSOR(x)
#endif

ULONG vmwareReadReg(struct HWData *data, ULONG reg)
{
    outl(reg, data->indexReg);
    return inl(data->valueReg);
}

VOID vmwareWriteReg(struct HWData *data, ULONG reg, ULONG val)
{
    outl(reg, data->indexReg);
    outl(val, data->valueReg);
}

VOID vmwareHandlerIRQ(struct HWData *data, void *unused)
{
    UWORD port = (UWORD)((IPTR)data->iobase + SVGA_IRQSTATUS_PORT);
    ULONG irqFlags = inl(port);
    outl(irqFlags, port);

    DIRQ(bug("[VMWareSVGA] %s(%08x)\n", __func__, irqFlags);)
    if (!irqFlags)
    {
        D(bug("[VMWareSVGA] %s: Spurrious IRQ!\n", __func__);)
    }
    else
    {
    }
}


#undef SysBase
extern struct ExecBase *SysBase;



APTR VMWareSVGA_MemAlloc(struct HWData *data, ULONG size)
{
    D(bug("[VMWareSVGA:HW] %s(%d)\n", __func__, size);)
    return AllocMem(size, MEMF_CLEAR|MEMF_ANY);
}

VOID VMWareSVGA_MemFree(struct HWData *data, APTR addr, ULONG size)
{
    D(bug("[VMWareSVGA:HW] %s(0x%p)\n", __func__, addr);)
    FreeMem(addr, size);
}


/**********/

ULONG getVMWareSVGAID(struct HWData *data)
{
    ULONG id;

    D(bug("[VMWareSVGA:HW] %s()\n", __func__);)

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

BOOL hasCapVMWareSVGAFIFO(struct HWData *data, ULONG fifocap)
{
    volatile ULONG *fifo = data->mmiobase;
    if ((data->capabilities & SVGA_CAP_EXTENDED_FIFO) &&
        (fifo[SVGA_FIFO_MIN] > (fifocap << 2)))
    {
        return TRUE;
    }
    return FALSE;
}

VOID initVMWareSVGAFIFO(struct HWData *data)
{
    volatile ULONG *fifo = data->mmiobase;

    DFIFOINF(bug("[VMWareSVGA:HW] %s()\n", __func__);)

    if (!data->fifomin)
    {
        if (data->capabilities & SVGA_CAP_EXTENDED_FIFO)
        {
            DFIFOINF(bug("[VMWareSVGA:HW] %s: Extended FIFO Capabilities\n", __func__);)
            data->fifomin = vmwareReadReg(data, SVGA_REG_MEM_REGS);
            data->fifocapabilities = fifo[SVGA_FIFO_CAPABILITIES];
            DFIFOINF(
              if (data->fifocapabilities & SVGA_FIFO_CAP_FENCE)
                  bug("[VMWareSVGA] %s:   Fence\n", __func__);
              if (data->fifocapabilities & SVGA_FIFO_CAP_ACCELFRONT)
                  bug("[VMWareSVGA] %s:   Accelerate Front\n", __func__);
              if (data->fifocapabilities & SVGA_FIFO_CAP_PITCHLOCK)
                  bug("[VMWareSVGA] %s:   Pitchlock\n", __func__);
              if (data->fifocapabilities & SVGA_FIFO_CAP_VIDEO)
                  bug("[VMWareSVGA] %s:   Video\n", __func__);
              if (data->fifocapabilities & SVGA_FIFO_CAP_CURSOR_BYPASS_3)
                  bug("[VMWareSVGA] %s:   Cursor-Bypass3\n", __func__);
              if (data->fifocapabilities & SVGA_FIFO_CAP_ESCAPE)
                  bug("[VMWareSVGA] %s:   Escape\n", __func__);
              if (data->fifocapabilities & SVGA_FIFO_CAP_RESERVE)
                  bug("[VMWareSVGA] %s:   FIFO Reserve\n", __func__);
              if (data->fifocapabilities & SVGA_FIFO_CAP_SCREEN_OBJECT)
                  bug("[VMWareSVGA] %s:   Screen-Object\n", __func__);
              if (data->fifocapabilities & SVGA_FIFO_CAP_GMR2)
                  bug("[VMWareSVGA] %s:   GMR2\n", __func__);
              if (data->fifocapabilities & SVGA_FIFO_CAP_3D_HWVERSION_REVISED)
                  bug("[VMWareSVGA] %s:   3DHWVersion-Revised\n", __func__);
              if (data->fifocapabilities & SVGA_FIFO_CAP_SCREEN_OBJECT_2)
                  bug("[VMWareSVGA] %s:   Screen-Object2\n", __func__);
             )
        }
        else
            data->fifomin = SVGA_FIFO_CAPABILITIES;
    }
    DFIFOINF(bug("[VMWareSVGA:HW] %s: FIFO Min Regs = %d\n", __func__, data->fifomin));

    data->mmiosize = vmwareReadReg(data, SVGA_REG_MEM_SIZE);

    vmwareWriteReg(data, SVGA_REG_CONFIG_DONE, 0);		//Stop vmware from reading the fifo

    fifo[SVGA_FIFO_MIN] = (data->fifomin << VMWFIFO_CMD_SIZESHIFT);
    fifo[SVGA_FIFO_MAX] = data->mmiosize; 
    fifo[SVGA_FIFO_NEXT_CMD] = fifo[SVGA_FIFO_MIN];
    fifo[SVGA_FIFO_STOP] = fifo[SVGA_FIFO_MIN];

    vmwareWriteReg(data, SVGA_REG_CONFIG_DONE, 1);

    if (hasCapVMWareSVGAFIFO(data, SVGA_FIFO_GUEST_3D_HWVERSION))
    {
        DFIFOINF(bug("[VMWareSVGA:HW] %s: Setting GUEST_3D_HWVERSION = %d\n", __func__, SVGA3D_HWVERSION_CURRENT));
        fifo[SVGA_FIFO_GUEST_3D_HWVERSION] = SVGA3D_HWVERSION_CURRENT;
    }
}

APTR reserveVMWareSVGAFIFO(struct HWData *data, ULONG size)
{
    volatile ULONG *fifo = data->mmiobase;
    ULONG max = fifo[SVGA_FIFO_MAX];
    ULONG min = fifo[SVGA_FIFO_MIN];
    ULONG cmdNext = fifo[SVGA_FIFO_NEXT_CMD];
    if (size % VMWFIFO_CMD_SIZE) {
       DFIFOINF(bug("[VMWareSVGA:HW] %s: size of %d not 32bit-aligned!!\n", __func__, size);)
   }
}

VOID commitVMWareSVGAFIFO(struct HWData *data, ULONG size)
{
    volatile ULONG *fifo = data->mmiobase;
    ULONG max = fifo[SVGA_FIFO_MAX];
    ULONG min = fifo[SVGA_FIFO_MIN];
    ULONG cmdNext = fifo[SVGA_FIFO_NEXT_CMD];
}

ULONG fenceVMWareSVGAFIFO(struct HWData *data)
{
    ULONG fence = 1;

    if (hasCapVMWareSVGAFIFO(data, SVGA_FIFO_FENCE) && (data->fifocapabilities & SVGA_FIFO_CAP_FENCE))
    {
        fence = data->fence++;
        if (fence == 0) fence = 1;

        DFENCE(bug("[VMWareSVGA:HW] %s: inserting fence #%d\n", __func__, fence);)

        writeVMWareSVGAFIFO(data, SVGA_CMD_FENCE);
        writeVMWareSVGAFIFO(data, fence);
        syncVMWareSVGAFIFO(data);
    }
    return fence;
}

VOID syncfenceVMWareSVGAFIFO(struct HWData *data, ULONG fence)
{
    volatile ULONG *fifo;

    if (hasCapVMWareSVGAFIFO(data, SVGA_FIFO_FENCE) && (data->fifocapabilities & SVGA_FIFO_CAP_FENCE))
    {
        fifo = data->mmiobase;
        if ((LONG)(fifo[SVGA_FIFO_FENCE] - fence) < 0)
        {
            DFENCE(bug("[VMWareSVGA:HW] %s: fence #%d hasnt been reached yet...\n", __func__, fence);)
            
        }
    }
}

VOID syncVMWareSVGAFIFO(struct HWData *data)
{
    DFIFO(bug("[VMWareSVGA:HW] %s()\n", __func__);)

    vmwareWriteReg(data, SVGA_REG_SYNC, 1);
}

VOID writeVMWareSVGAFIFO(struct HWData *data, ULONG val)
{
    volatile ULONG *fifo;

    DFIFO(bug("[VMWareSVGA:HW] %s()\n", __func__);)

    fifo = data->mmiobase;
    if (
            (fifo[SVGA_FIFO_NEXT_CMD] + VMWFIFO_CMD_SIZE == fifo[SVGA_FIFO_STOP]) ||
            (
                (fifo[SVGA_FIFO_NEXT_CMD] == (fifo[SVGA_FIFO_MAX] - VMWFIFO_CMD_SIZE)) &&
                (fifo[SVGA_FIFO_STOP] == fifo[SVGA_FIFO_MIN])
            )
        )
        syncVMWareSVGAFIFO(data);

    fifo[fifo[SVGA_FIFO_NEXT_CMD] >> VMWFIFO_CMD_SIZESHIFT] = val;
    fifo[SVGA_FIFO_NEXT_CMD] += VMWFIFO_CMD_SIZE;

    if (fifo[SVGA_FIFO_NEXT_CMD] == fifo[SVGA_FIFO_MAX])
        fifo[SVGA_FIFO_NEXT_CMD] = fifo[SVGA_FIFO_MIN];
}

BOOL initVMWareSVGAHW(struct HWData *data, OOP_Object *device)
{
    ULONG id;

    D(bug("[VMWareSVGA:HW] %s()\n", __func__);)

    id = getVMWareSVGAID(data);
    if (id == SVGA_ID_INVALID)
        return FALSE;

    data->maskPool = CreatePool(MEMF_ANY, (32 << 3), (32 << 2));

    if (id >= SVGA_ID_1)
    {
        volatile ULONG *fifo = data->mmiobase;
        data->capabilities = vmwareReadReg(data, SVGA_REG_CAPABILITIES);
    }

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

    D(
      bug("[VMWareSVGA:HW] %s: VRAM at 0x%08x size %d\n", __func__, data->vrambase, data->vramsize);
      bug("[VMWareSVGA:HW] %s: no.displays: %d\n", __func__, data->displaycount);
      bug("[VMWareSVGA:HW] %s: depth: %d\n", __func__, data->depth);
      bug("[VMWareSVGA:HW] %s: bpp  : %d\n", __func__, data->bitsperpixel);
     )

    VMWareSVGA_RestartRenderTask(data);

    return TRUE;
}

VOID getModeCfgVMWareSVGA(struct HWData *data)
{
    D(bug("[VMWareSVGA:HW] %s()\n", __func__);)

    data->fboffset = vmwareReadReg(data, SVGA_REG_FB_OFFSET);
    data->bytesperline = vmwareReadReg(data, SVGA_REG_BYTES_PER_LINE);
    data->depth = vmwareReadReg(data, SVGA_REG_DEPTH);
    data->redmask = vmwareReadReg(data, SVGA_REG_RED_MASK);
    data->greenmask = vmwareReadReg(data, SVGA_REG_GREEN_MASK);
    data->bluemask = vmwareReadReg(data, SVGA_REG_BLUE_MASK);
    data->pseudocolor = vmwareReadReg(data, SVGA_REG_PSEUDOCOLOR);

    data->display_width = vmwareReadReg(data, SVGA_REG_WIDTH);
    data->display_height = vmwareReadReg(data, SVGA_REG_HEIGHT);
    D(bug("[VMWareSVGA:HW] %s: %dx%d\n", __func__, data->display_width, data->display_height));
}

VOID enableVMWareSVGA(struct HWData *data)
{
    vmwareWriteReg(data, SVGA_REG_ENABLE, 1);
}

VOID disableVMWareSVGA(struct HWData *data)
{
    vmwareWriteReg(data, SVGA_REG_ENABLE, 0);
}

VOID initDisplayVMWareSVGA(struct HWData *data)
{
    enableVMWareSVGA(data);
    getModeCfgVMWareSVGA(data);
    initVMWareSVGAFIFO(data);
}

VOID setModeVMWareSVGA(struct HWData *data, ULONG width, ULONG height)
{
    D(bug("[VMWareSVGA:HW] %s(%dx%d)\n", __func__, width, height));

    disableVMWareSVGA(data);

    vmwareWriteReg(data, SVGA_REG_WIDTH, width);
    vmwareWriteReg(data, SVGA_REG_HEIGHT, height);

    if (data->capabilities & SVGA_CAP_DISPLAY_TOPOLOGY)
    {
        D(bug("[VMWareSVGA:HW] %s: Adjusting Display Topology\n", __func__);)

        vmwareWriteReg(data, SVGA_REG_DISPLAY_ID, 0);
        vmwareWriteReg(data, SVGA_REG_DISPLAY_IS_PRIMARY, TRUE);

        vmwareWriteReg(data, SVGA_REG_DISPLAY_POSITION_X, 0);
        vmwareWriteReg(data, SVGA_REG_DISPLAY_POSITION_Y, 0);
        vmwareWriteReg(data, SVGA_REG_DISPLAY_WIDTH, width);
        vmwareWriteReg(data, SVGA_REG_DISPLAY_HEIGHT, height);
    }

    if (data->capabilities & SVGA_CAP_8BIT_EMULATION)
        vmwareWriteReg(data, SVGA_REG_BITS_PER_PIXEL, data->bitsperpixel);

    initDisplayVMWareSVGA(data);
}

VOID refreshAreaVMWareSVGA(struct HWData *data, struct Box *box)
{
    DREFRESH(bug("[VMWareSVGA:HW] %s()\n", __func__);)

    writeVMWareSVGAFIFO(data, SVGA_CMD_UPDATE);
    writeVMWareSVGAFIFO(data, box->x1);
    writeVMWareSVGAFIFO(data, box->y1);
    writeVMWareSVGAFIFO(data, box->x2-box->x1+1);
    writeVMWareSVGAFIFO(data, box->y2-box->y1+1);
}

VOID rectFillVMWareSVGA(struct HWData *data, ULONG color, LONG x, LONG y, LONG width, LONG height)
{
    D(bug("[VMWareSVGA:HW] %s()\n", __func__);)

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
    D(bug("[VMWareSVGA:HW] %s()\n", __func__);)

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
    D(bug("[VMWareSVGA:HW] %s()\n", __func__);)

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
    ULONG size = mouse->width * mouse->height;
    ULONG *xorMask, *xorBuffer;
    ULONG *image = mouse->shape;
    int i;

    DCURSOR(bug("[VMWareSVGA:HW] %s()\n", __func__);)

    xorMask = (ULONG *)AllocVecPooled(data->maskPool, size << 2);
    if (xorMask)
    {
        DCURSOR(bug("[VMWareSVGA:HW] %s: xormask @ 0x%p\n", __func__, xorMask);)
        xorBuffer = xorMask;
        for (i = 0; i < size; i++)
        {
            ULONG pixel = *image ++;
            *xorBuffer = pixel;
            xorBuffer++;
        }

        if (data->capabilities & SVGA_CAP_ALPHA_CURSOR)
        {
            DCURSOR(bug("[VMWareSVGA:HW] %s: rendering Alpha Cursor\n", __func__);)
            writeVMWareSVGAFIFO(data, SVGA_CMD_DEFINE_ALPHA_CURSOR);
            writeVMWareSVGAFIFO(data, VMWCURSOR_ID); // id
            writeVMWareSVGAFIFO(data, 0); // hotspot x
            writeVMWareSVGAFIFO(data, 0); // hotspot y
            writeVMWareSVGAFIFO(data, mouse->width); // width
            writeVMWareSVGAFIFO(data, mouse->height); // height

            xorBuffer = xorMask;
            for (i = 0; i < size; i++)
            {
                writeVMWareSVGAFIFO(data, *xorBuffer++);
            }

            syncVMWareSVGAFIFO(data);
        }
        else
        {
#if (0)
            xandBuffer = (ULONG *)AllocVecPooled(data->maskPool, size << 2);
            if (xandBuffer)
            {
                DCURSOR(bug("[VMWareSVGA:HW] %s: rendering masked Cursor\n", __func__);)
                writeVMWareSVGAFIFO(data, SVGA_CMD_DEFINE_CURSOR);
                writeVMWareSVGAFIFO(data, VMWCURSOR_ID); // id
                writeVMWareSVGAFIFO(data, 0); // hotspot x
                writeVMWareSVGAFIFO(data, 0); // hotspot y
                writeVMWareSVGAFIFO(data, mouse->width); // width
                writeVMWareSVGAFIFO(data, mouse->height); // height

                for (i = 0; i < size; i++)
                {
                    writeVMWareSVGAFIFO(data, *xandBuffer++);
                }
                
                xorBuffer = xorMask;
                for (i = 0; i < size; i++)
                {
                    writeVMWareSVGAFIFO(data, *xorBuffer++);
                }

                syncVMWareSVGAFIFO(data);
                FreeVecPooled(data->maskPool, xandBuffer);
            }
#endif
        }
        FreeVecPooled(data->maskPool, xorMask);
    }
}

VOID displayCursorVMWareSVGA(struct HWData *data, LONG mode)
{
    volatile ULONG *fifo = data->mmiobase;

    DCURSOR(bug("[VMWareSVGA:HW] %s()\n", __func__);)

    if (hasCapVMWareSVGAFIFO(data, SVGA_FIFO_CURSOR_COUNT) && (data->fifocapabilities & SVGA_FIFO_CAP_CURSOR_BYPASS_3))
    {
        ULONG count;
        fifo[SVGA_FIFO_CURSOR_ON] = mode;
        count = fifo[SVGA_FIFO_CURSOR_COUNT];
        fifo[SVGA_FIFO_CURSOR_COUNT] = ++count;
    }
    else
    {
        if (data->capabilities & SVGA_CAP_CURSOR_BYPASS_2)
        {
            vmwareWriteReg(data, SVGA_REG_CURSOR_ID, VMWCURSOR_ID);
            vmwareWriteReg(data, SVGA_REG_CURSOR_ON, mode);
        }
        else
        {
            writeVMWareSVGAFIFO(data, SVGA_CMD_DISPLAY_CURSOR);
            writeVMWareSVGAFIFO(data, VMWCURSOR_ID);
            writeVMWareSVGAFIFO(data, mode);
            syncVMWareSVGAFIFO(data);
        }
    }
}

VOID moveCursorVMWareSVGA(struct HWData *data, LONG x, LONG y)
{
    volatile ULONG *fifo = data->mmiobase;

    DCURSOR(bug("[VMWareSVGA:HW] %s()\n", __func__);)

    if (hasCapVMWareSVGAFIFO(data, SVGA_FIFO_CURSOR_COUNT) && (data->fifocapabilities & SVGA_FIFO_CAP_CURSOR_BYPASS_3))
    {
        ULONG count;
        fifo[SVGA_FIFO_CURSOR_ON] = SVGA_CURSOR_ON_SHOW;
        fifo[SVGA_FIFO_CURSOR_X] = x;
        fifo[SVGA_FIFO_CURSOR_Y] = y;
        count = fifo[SVGA_FIFO_CURSOR_COUNT];
        fifo[SVGA_FIFO_CURSOR_COUNT] = ++count;
    }
    else
    {
        if (data->capabilities & SVGA_CAP_CURSOR_BYPASS_2)
        {
            vmwareWriteReg(data, SVGA_REG_CURSOR_ID, VMWCURSOR_ID);
            vmwareWriteReg(data, SVGA_REG_CURSOR_X, x);
            vmwareWriteReg(data, SVGA_REG_CURSOR_Y, y);
            vmwareWriteReg(data, SVGA_REG_CURSOR_ON, SVGA_CURSOR_ON_SHOW);
        }
        else
        {
            writeVMWareSVGAFIFO(data, SVGA_CMD_MOVE_CURSOR);
            writeVMWareSVGAFIFO(data, x);
            writeVMWareSVGAFIFO(data, y);
            syncVMWareSVGAFIFO(data);
        }
    }
}

VOID VMWareSVGA_Damage_Reset(struct HWData *hwdata)
{
    DDMG(bug("[VMWareSVGA:HW] %s()\n", __func__);)

    ObtainSemaphore(&hwdata->damage_control);

    hwdata->delta_damage.x1 = INT_MAX;
    hwdata->delta_damage.y1 = INT_MAX;
    hwdata->delta_damage.x2 = INT_MIN;
    hwdata->delta_damage.y2 = INT_MIN;

    ReleaseSemaphore(&hwdata->damage_control);
}

VOID VMWareSVGA_Damage_DeltaAdd(struct HWData *hwdata, struct Box *box)
{
    ULONG tmpval;

    DDMG(bug("[VMWareSVGA:HW] %s()\n", __func__);)

    if (box->x1 > box->x2)
    {
        tmpval = box->x2;
        box->x2 = box->x1;
        box->x1 = tmpval;
    }
    if (box->y1 > box->y2)
    {
        tmpval = box->y2;
        box->y2 = box->y1;
        box->y1 = tmpval;
    }

    ObtainSemaphore(&hwdata->damage_control);
    if (box->x1 < hwdata->delta_damage.x1)
    {
        hwdata->delta_damage.x1 = box->x1;
    }
    if (box->y1 < hwdata->delta_damage.y1)
    {
        hwdata->delta_damage.y1 = box->y1;
    }
    if (box->x2 > hwdata->delta_damage.x2)
    {
        hwdata->delta_damage.x2 = box->x2;
    }
    if (box->y2 > hwdata->delta_damage.y2)
    {
        hwdata->delta_damage.y2 = box->y2;
    }
#if defined(VMWAREGFX_IMMEDIATEDRAW)
    if (hwdata->shown)
    {
        refreshAreaVMWareSVGA(hwdata, &hwdata->delta_damage);
        VMWareSVGA_Damage_Reset(hwdata);
        syncVMWareSVGAFIFO(hwdata);
    }
#endif
    ReleaseSemaphore(&hwdata->damage_control);
}

#if !defined(VMWAREGFX_IMMEDIATEDRAW)
VOID VMWareSVGA_RenderTask(struct HWData *hwdata)
{
    struct MsgPort render_thread_message_port;
    struct timerequest *timer_request;
    struct IORequest *timer_request_as_io_request;
    ULONG request_size = sizeof(struct timerequest);
    BYTE running;

    render_thread_message_port.mp_Flags = PA_SIGNAL;
    render_thread_message_port.mp_Node.ln_Type = NT_MSGPORT;
    render_thread_message_port.mp_MsgList.lh_TailPred = (struct Node *)&render_thread_message_port.mp_MsgList;
    render_thread_message_port.mp_MsgList.lh_Tail = 0;
    render_thread_message_port.mp_MsgList.lh_Head = (struct Node *)&render_thread_message_port.mp_MsgList.lh_Tail;

    render_thread_message_port.mp_SigBit = AllocSignal(-1);
    render_thread_message_port.mp_SigTask = FindTask(0);

    timer_request = AllocMem(request_size, MEMF_CLEAR | MEMF_PUBLIC);
    timer_request_as_io_request = (void *)timer_request;

    timer_request_as_io_request->io_Message.mn_Node.ln_Type 	= NT_MESSAGE;
    timer_request_as_io_request->io_Message.mn_ReplyPort	  	= &render_thread_message_port;
    timer_request_as_io_request->io_Message.mn_Length	  		= request_size;

    OpenDevice("timer.device", UNIT_MICROHZ, timer_request_as_io_request, 0);

    timer_request->tr_node.io_Command = TR_ADDREQUEST;
    timer_request->tr_time.tv_secs = 0;
    timer_request->tr_time.tv_micro = 20000;
    SendIO(timer_request_as_io_request);

    running = 1;
    while (running) // TODO: If you'll ever implement GFX Driver hot swap, you will want to unlock this condition and let the RenderTask terminate
    {
        if (!vmwareReadReg(hwdata, SVGA_REG_BUSY))
        {
            ObtainSemaphore(&hwdata->damage_control);
            struct Box *damage = &hwdata->delta_damage;

            if ((damage->x2 > damage->x1) && (damage->y2 > damage->y1))
            {
                refreshAreaVMWareSVGA(hwdata, &hwdata->delta_damage);
                VMWareSVGA_Damage_Reset(hwdata);
                syncVMWareSVGAFIFO(hwdata);
            }
            ReleaseSemaphore(&hwdata->damage_control);
        }

        WaitIO(timer_request_as_io_request);
        GetMsg(&render_thread_message_port); // TODO: Did we have to reply to this? Oh memory ...

        timer_request->tr_node.io_Command = TR_ADDREQUEST;
        timer_request->tr_time.tv_secs = 0;
        timer_request->tr_time.tv_micro = 20000; // TODO: This should be adaptive. We would need to know the CPU load and increase the delay to avoid burning all of the CPU time
        SendIO(timer_request_as_io_request);
    }

    CloseDevice(timer_request_as_io_request);
}
#endif

VOID VMWareSVGA_RestartRenderTask(struct HWData *hwdata)
{
    // TODO: CleanUp and add defenses here

    InitSemaphore(&hwdata->damage_control);
#if !defined(VMWAREGFX_IMMEDIATEDRAW)
    hwdata->render_task = NewCreateTask(TASKTAG_PC,
                                        VMWareSVGA_RenderTask,
                                        TASKTAG_NAME,	"VMWare Render Task",
                                        TASKTAG_PRI,	1,
                                        TASKTAG_ARG1,	hwdata,
                                        TAG_DONE);
#endif
}
