/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: vmware svga hardware functions
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1 /* no SysBase */
#include <aros/debug.h>

#include <asm/io.h>

#include <limits.h>
#include <exec/ports.h>
#include <devices/timer.h>
#include <proto/exec.h>

#include "vmwaresvga_intern.h"

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

    D(bug("[VMWareSVGA:HW] %s: VRAM at 0x%08x size %d\n", __func__, data->vrambase, data->vramsize));
    D(bug("[VMWareSVGA:HW] %s: no.displays: %d\n", __func__, data->displaycount));
    D(bug("[VMWareSVGA:HW] %s: caps : 0x%08x\n", __func__, data->capabilities));
    D(bug("[VMWareSVGA:HW] %s: no.displays: %d\n", __func__, data->displaycount));
    D(bug("[VMWareSVGA:HW] %s: depth: %d\n", __func__, data->depth));
    D(bug("[VMWareSVGA:HW] %s: bpp  : %d\n", __func__, data->bitsperpixel));
    D(bug("[VMWareSVGA:HW] %s: maxw: %d\n", __func__, data->maxwidth));
    D(bug("[VMWareSVGA:HW] %s: maxh: %d\n", __func__, data->maxheight));

    VMWareSVGA_RestartRenderTask(data);

    return TRUE;
}

VOID setModeVMWareSVGA(struct HWData *data, ULONG width, ULONG height)
{
    D(bug("[VMWareSVGA:HW] %s(%dx%d)\n", __func__, width, height));

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

    data->display_width = vmwareReadReg(data, SVGA_REG_WIDTH);
    data->display_height = vmwareReadReg(data, SVGA_REG_HEIGHT);
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
    ULONG size = mouse->width * mouse->height;
    // TODO: This is utterly disgusting. Should be moved to either a proper static data area, or dynamic area down to the render task
    ULONG xorMask[size];
    ULONG *image = mouse->shape;
    ULONG *xorBuffer;

    writeVMWareSVGAFIFO(data, SVGA_CMD_DEFINE_ALPHA_CURSOR);
    writeVMWareSVGAFIFO(data, 0); // id
    writeVMWareSVGAFIFO(data, 0); // hotspot x
    writeVMWareSVGAFIFO(data, 0); // hotspot y
    writeVMWareSVGAFIFO(data, mouse->width); // width
    writeVMWareSVGAFIFO(data, mouse->height); // height

    xorBuffer = xorMask;

    for (i = 0; i < size; i++)
    {
        ULONG pixel = *image ++;
        *xorBuffer = pixel;
        xorBuffer++;
    }

    xorBuffer = xorMask;
    for (i = 0; i < size; i++)
    {
        writeVMWareSVGAFIFO(data, *xorBuffer++);
    }

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

VOID VMWareSVGA_Damage_Reset(struct HWData *hwdata)
{
    ObtainSemaphore(&hwdata->damage_control);

    hwdata->delta_damage.x1 = INT_MAX;
    hwdata->delta_damage.y1 = INT_MAX;
    hwdata->delta_damage.x2 = INT_MIN;
    hwdata->delta_damage.y2 = INT_MIN;

    ReleaseSemaphore(&hwdata->damage_control);
}

VOID VMWareSVGA_Damage_DeltaAdd(struct HWData *hwdata, struct Box box)
{
    ObtainSemaphore(&hwdata->damage_control);

    if (box.x1 < hwdata->delta_damage.x1)
    {
        hwdata->delta_damage.x1 = box.x1;
    }
    if (box.y1 < hwdata->delta_damage.y1)
    {
        hwdata->delta_damage.y1 = box.y1;
    }
    if (box.x2 > hwdata->delta_damage.x2)
    {
        hwdata->delta_damage.x2 = box.x2;
    }
    if (box.y2 > hwdata->delta_damage.y2)
    {
        hwdata->delta_damage.y2 = box.y2;
    }
    ReleaseSemaphore(&hwdata->damage_control);
}

void VMWareSVGA_RenderTask(struct HWData *hwdata)
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
            struct Box damage = hwdata->delta_damage;
            struct Box all_damage = {0, 0, hwdata->display_width, hwdata->display_height};

            if (damage.x2 > damage.x1 && damage.y2 > damage.y1)
            {
                refreshAreaVMWareSVGA(hwdata, &all_damage);
                VMWareSVGA_Damage_Reset(hwdata);
                vmwareWriteReg(hwdata, SVGA_REG_SYNC, 1);
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

VOID VMWareSVGA_RestartRenderTask(struct HWData *hwdata)
{
    // TODO: CleanUp and add defenses here

    InitSemaphore(&hwdata->damage_control);

    hwdata->render_task = NewCreateTask(TASKTAG_PC,
                                        VMWareSVGA_RenderTask,
                                        TASKTAG_NAME,	"VMWare Render Task",
                                        TASKTAG_PRI,	1,
                                        TASKTAG_ARG1,	hwdata,
                                        TAG_DONE);
}
