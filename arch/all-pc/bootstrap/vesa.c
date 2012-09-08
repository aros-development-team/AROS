/*
    Copyright © 2007-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Real-mode code to set VBE mode.
    Lang: english
*/

#define _IMPLEMENTATION_

asm ("begin:");

#include "vesa.h"
#define ABS(x) (((x) >= 0) ? (x) : -(x))

asm (".long getControllerInfo");
asm (".long getModeInfo");
asm (".long findMode");
asm (".long setVbeMode");
asm (".long paletteWidth");
asm (".long controllerinfo");
asm (".long modeinfo");
asm (".long timings");

short getControllerInfo(void)
{
    short retval;

    controllerinfo.signature[0] = 'V';
    controllerinfo.signature[1] = 'B';
    controllerinfo.signature[2] = 'E';
    controllerinfo.signature[3] = '2';
    asm volatile("call go16 \n\t.code16 \n\t"
                "movw $0x4f00, %%ax\n\t"
                "int $0x10\n\t"
                "movw %%ax, %0\n\t"
                "DATA32 call go32\n\t.code32\n\t":"=b"(retval):"D"(&controllerinfo):"eax","ecx","cc");
    return retval;
}

/* In VBE 1.1 information about standard modes was optional,
   so we use a hardcoded table here (we rely on this information) */
struct vesa11Info vesa11Modes[] = {
    {640,  400,  8, 4},
    {640,  480,  8, 4},
    {800,  600,  4, 3},
    {800,  600,  8, 4},
    {1024, 768,  4, 3},
    {1024, 768,  8, 4},
    {1280, 1024, 4, 3},
    {1280, 1024, 8, 4}
};

short getModeInfo(long mode)
{
    short retval;
    long i;
    char *ptr = (char *)&modeinfo;
    for (i = 0; i < sizeof(modeinfo); i++)
	*ptr++ = 0;
    asm volatile("call go16 \n\t.code16 \n\t"
                "movw $0x4f01, %%ax\n\t"
                "int $0x10\n\t"
                "movw %%ax, %0\n\t"
                "DATA32 call go32\n\t.code32\n\t":"=b"(retval):"c"(mode),"D"(&modeinfo):"eax","cc");
    if ((controllerinfo.version < 0x0102) && (mode > 0x0FF) && (mode < 0x108)) {
	i = mode - 0x100;
	modeinfo.x_resolution = vesa11Modes[i].x_resolution;
	modeinfo.y_resolution = vesa11Modes[i].y_resolution;
	modeinfo.bits_per_pixel = vesa11Modes[i].bits_per_pixel;
	modeinfo.memory_model = vesa11Modes[i].memory_model;
    }
    return retval;
}

short setVbeMode(long mode, BOOL set_refresh)
{
    short retval;

    /* Enable custom timings if possible */
    if (set_refresh && controllerinfo.version >= 0x0300)
        mode |= 0x800;
    else
        mode &= 0xf7ff;

    asm volatile("call go16 \n\t.code16 \n\t"
                "movw $0x4f02, %%ax\n\t"
                "int $0x10\n\t"
                "movw %%ax, %0\n\t"
                "DATA32 call go32\n\t.code32\n\t":"=b"(retval):"0"(mode),"D"(&timings):"eax","ecx","cc");
    return retval;
}

short paletteWidth(long req, unsigned char* width)
{
    short retval;
    unsigned char reswidth;
    
    asm volatile("call go16\n\t.code16\n\t"
		"movw $0x4f08, %%ax\n\t"
		"int $0x10\n\t"
		"movb %%bh, %1\n\t"
		"movw %%ax, %0\n\t"
		"DATA32 call go32\n\t.code32\n\t":"=b"(retval),"=c"(reswidth):"0"(req):"eax","cc");
    *width = reswidth;
    return retval;
}

/* Definitions used in CVT formula */
#define M 600
#define C 40
#define K 128
#define J 20
#define DUTY_CYCLE(period) \
    (((C - J) / 2 + J) * 1000 - (M / 2 * (period) / 1000))
#define MIN_DUTY_CYCLE 20 /* % */
#define MIN_V_PORCH 3 /* lines */
#define MIN_V_PORCH_TIME 550 /* us */
#define CLOCK_STEP 250000 /* Hz */

/* Partial implementation of CVT formula */
void calcTimings(int vfreq)
{
    ULONG x, y, h_period, h_freq, h_total, h_blank, h_front, h_sync, h_back,
        v_freq, v_total, v_front, v_sync, v_back, duty_cycle, pixel_freq;

    x = modeinfo.x_resolution;
    y = modeinfo.y_resolution;

    /* Get horizontal period in microseconds */
    h_period = (1000000000 / vfreq - MIN_V_PORCH_TIME * 1000)
        / (y + MIN_V_PORCH);

    /* Vertical front porch is fixed */
    v_front = MIN_V_PORCH;

    /* Use aspect ratio to determine V-sync lines */
    if (x == y * 4 / 3)
        v_sync = 4;
    else if (x == y * 16 / 9)
        v_sync = 5;
    else if (x == y * 16 / 10)
        v_sync = 6;
    else if (x == y * 5 / 4)
        v_sync = 7;
    else if (x == y * 15 / 9)
        v_sync = 7;
    else
        v_sync = 10;

    /* Get vertical back porch */
    v_back = MIN_V_PORCH_TIME * 1000 / h_period + 1;
    if (v_back < MIN_V_PORCH)
        v_back = MIN_V_PORCH;
    v_back -= v_sync;

    /* Get total lines per frame */
    v_total = y + v_front + v_sync + v_back;

    /* Get horizontal blanking pixels */
    duty_cycle = DUTY_CYCLE(h_period);
    if (duty_cycle < MIN_DUTY_CYCLE)
        duty_cycle = MIN_DUTY_CYCLE;

    h_blank = 10 * x * duty_cycle / (100000 - duty_cycle);
    h_blank /= 2 * 8 * 10;
    h_blank = h_blank * (2 * 8);

    /* Get total pixels in a line */
    h_total = x + h_blank;

    /* Calculate frequencies for each pixel, line and field */
    h_freq = 1000000000 / h_period;
    pixel_freq = h_freq * h_total / CLOCK_STEP * CLOCK_STEP;
    h_freq = pixel_freq / h_total;
    v_freq = 100 * h_freq / v_total;

    /* Back porch is half of H-blank */
    h_back = h_blank / 2;

    /* H-sync is a fixed percentage of H-total */
    h_sync = h_total / 100 * 8;

    /* Front porch is whatever's left */
    h_front = h_blank - h_sync - h_back;

    /* Fill in VBE timings structure */
    timings.h_total = h_total;
    timings.h_sync_start = x + h_front;
    timings.h_sync_end = h_total - h_back;
    timings.v_total = v_total;
    timings.v_sync_start = y + v_front;
    timings.v_sync_end = v_total - v_back;
    timings.flags = 0x4;
    timings.pixel_clock = pixel_freq;
    timings.refresh_rate = v_freq;
}

short findMode(int x, int y, int d, int vfreq, BOOL prioritise_depth)
{
    unsigned long match, bestmatch = 0, matchd, bestmatchd = 0;
    unsigned short bestmode = 0xffff, mode_attrs;
    int bestd = 0;

    if (getControllerInfo() == 0x4f)
    {
        unsigned short *modes = (unsigned short *)
            (((controllerinfo.video_mode & 0xffff0000) >> 12) + (controllerinfo.video_mode & 0xffff));

        int i;

	if (controllerinfo.version < 0x0200)
	    mode_attrs = 0x11;
	else
	    mode_attrs = 0x91;

        for (i=0; modes[i] != 0xffff; ++i)
        {
            if (getModeInfo(modes[i])!= 0x4f) continue;
            if ((modeinfo.mode_attributes & mode_attrs) != mode_attrs) continue;
            if ((modeinfo.memory_model != 6) && (modeinfo.memory_model != 4))
		continue;
	    if ((modeinfo.memory_model == 4) && (modeinfo.mode_attributes & 0x20))
		continue;

            /* Return immediately if an exactly matching mode is found
             * (otherwise we could potentially return a mode with the right
             * area but different dimensions) */
            if (modeinfo.x_resolution == x &&
                modeinfo.y_resolution == y &&
                modeinfo.bits_per_pixel == d)
            {
                bestmode = modes[i];
                break;
            }

            match = ABS(modeinfo.x_resolution*modeinfo.y_resolution - x*y);
            matchd = modeinfo.bits_per_pixel >= d ? modeinfo.bits_per_pixel-d: (d-modeinfo.bits_per_pixel)*2;

            if (prioritise_depth)
            {
                /* Check if current mode is the best so far at the desired
                 * depth, or has a higher depth than previously found */
                if (bestmode == 0xffff || (match < bestmatch
                    && modeinfo.bits_per_pixel == bestd)
                    || (bestd < d && modeinfo.bits_per_pixel > bestd
                    && modeinfo.bits_per_pixel <= d))
                {
                    bestmode = modes[i];
                    bestmatch = match;
                    bestd = modeinfo.bits_per_pixel;
                }
            }
            else
            {
                /* Check if current mode either has the closest resolution
                 * so far to that requested, or is equally close as the
                 * previous best but has closer colour depth */
                if (bestmode == 0xffff || match < bestmatch
                    || (match == bestmatch && matchd < bestmatchd))
                {
                    bestmode = modes[i];
                    bestmatch = match;
                    bestmatchd = matchd;
                }
            }
        }
    }

    /* Set up timings to achieve the desired refresh rate */
    if (controllerinfo.version >= 0x0300 && getModeInfo(bestmode) == 0x4f)
        calcTimings(vfreq);

    return bestmode;
}

asm(
        "               .code16\n\t.globl go32\n\t.type go32,@function\n"
        "go32:          DATA32 ADDR32 lgdt GDT_reg\n"
        "               movl %cr0, %eax\n"
        "               bts $0, %eax\n"
        "               movl %eax, %cr0\n"
        "               ljmp $0x08, $1f\n"
        "               .code32\n"
        "1:             movw $0x10, %ax\n"
        "               movw %ax, %ds\n"
        "               movw %ax, %es\n"
        "               movw %ax, %fs\n"
        "               movw %ax, %gs\n"
        "               movw %ax, %ss\n"
        "               movl (%esp), %ecx\n"
        "               movl stack32, %eax\n"
        "               movl %eax, %esp\n"
        "               movl %ecx, (%esp)\n"
        "               xorl %eax, %eax\n"
        "               sidt IDT_reg16\n"
        "               lidt IDT_reg32\n"
        "               ret\n"
        "\n"
        "               .code32\n\t.globl go16\n\t.type go16,@function\n"
        "go16:          lgdt GDT_reg\n"
        "               sidt IDT_reg32\n"
        "               lidt IDT_reg16\n"
        "               movl %esp, stack32\n"
        "               movl (%esp), %eax\n"
        "               movl %eax, scratch + 63\n"
        "               movl $scratch + 63, %esp\n"
        "               movw $0x20, %ax\n"
        "               movw %ax, %ds\n"
        "               movw %ax, %es\n"
        "               movw %ax, %fs\n"
        "               movw %ax, %gs\n"
        "               movw %ax, %ss\n"
        "               ljmp $0x18, $1f\n\t.code16\n"
        "1:\n"
        "               movl %cr0, %eax\n"
        "               btc $0, %eax\n"
        "               movl %eax, %cr0\n"
        "               DATA32 ljmp $0x00, $1f\n"
        "1:\n"
        "               xorl %eax,%eax\n"
        "               movw %ax, %ds\n"
        "               movw %ax, %es\n"
        "               movw %ax, %fs\n"
        "               movw %ax, %gs\n"
        "               movw %ax, %ss\n"
        "               DATA32 ret\n"
        ".code32");

const unsigned long long GDT_Table[] = {
        0x0000000000000000ULL,
        0x00cf9a000000ffffULL,     /* Code32 */
        0x00cf92000000ffffULL,     /* Data32 */
        0x00009e000000ffffULL,     /* Code16 */
        0x000092000000ffffULL      /* Data16 */
};

const struct
{
    unsigned short l1 __attribute__((packed));
    const void *l3 __attribute__((packed));
}
GDT_reg = {sizeof(GDT_Table)-1, GDT_Table},
IDT_reg16 = {0x400, 0},
IDT_reg32;

unsigned long           stack32;
unsigned long           scratch[64];
struct vbe_controller   controllerinfo;
struct vbe_mode         modeinfo;
struct CRTCInfoBlock    timings;

