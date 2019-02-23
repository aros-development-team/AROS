/*
    Copyright © 2013-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VideoCore framebuffer routines
    Lang: english
*/

#include <aros/macros.h>

#include <hardware/bcm2708.h>
#include <hardware/bcm2708_boot.h>
#undef ARM_PERIIOBASE

#include <hardware/videocore.h>

#include "bootconsole.h"
#include "vc_mb.h"
#include "vc_fb.h"
#include "boot.h"

#undef ARM_PERIIOBASE
#define ARM_PERIIOBASE (__arm_periiobase)
extern uint32_t __arm_periiobase;

#define D(x) /* x */

int vcfb_init(void)
{
    unsigned int fb_width, fb_height, fb_depth, fb_pitch;
    unsigned int count;
    volatile unsigned int *vcmb_msg = (unsigned int *)BOOTMEMADDR(bm_mboxmsg);
    scr_FrameBuffer = 0;

    D(kprintf("[VCFB] vcfb_init()\n"));

    scr_Type = SCR_UNKNOWN;

    /* query the display dimensions */
    {
        vcmb_msg[0] = AROS_LONG2LE(8 * 4);
        vcmb_msg[1] = AROS_LONG2LE(VCTAG_REQ);
        vcmb_msg[2] = AROS_LONG2LE(VCTAG_GETRES);
        vcmb_msg[3] = AROS_LONG2LE(8);
        vcmb_msg[4] = 0;
        vcmb_msg[5] = 0;
        vcmb_msg[6] = 0;
        vcmb_msg[7] = 0;		        // terminate tag

        vcmb_write(VCMB_BASE, VCMB_PROPCHAN, (void *)vcmb_msg);
        vcmb_msg = vcmb_read(VCMB_BASE, VCMB_PROPCHAN);

        if (!vcmb_msg || (vcmb_msg[1] != AROS_LONG2LE(VCTAG_RESP)))
            return 0;

        if (((fb_width = AROS_LE2LONG(vcmb_msg[5])) == 0) || ((fb_height = AROS_LE2LONG(vcmb_msg[6])) == 0))
        {
            fb_width = 1024;
            fb_height = 768;
        }

        D(kprintf("[VCFB] fb_width=%d, fb_height=%d\n", fb_width, fb_height));
    }

    /* fill in our framebuffer configuration/allocation request */
    {
        unsigned int c = 1;
        vcmb_msg[c++] = AROS_LONG2LE(VCTAG_REQ);

        vcmb_msg[c++] = AROS_LONG2LE(VCTAG_SETRES);
        vcmb_msg[c++] = AROS_LONG2LE(8);
        vcmb_msg[c++] = AROS_LONG2LE(0);
        vcmb_msg[c++] = AROS_LONG2LE(fb_width);
        vcmb_msg[c++] = AROS_LONG2LE(fb_height);

        vcmb_msg[c++] = AROS_LONG2LE(VCTAG_SETVRES);          // duplicate physical size...
        vcmb_msg[c++] = AROS_LONG2LE(8);
        vcmb_msg[c++] = AROS_LONG2LE(0);
        vcmb_msg[c++] = AROS_LONG2LE(fb_width);
        vcmb_msg[c++] = AROS_LONG2LE(fb_height);

        vcmb_msg[c++] = AROS_LONG2LE(VCTAG_SETDEPTH);
        vcmb_msg[c++] = AROS_LONG2LE(4);
        vcmb_msg[c++] = AROS_LONG2LE(0);

        fb_depth = 16;

        vcmb_msg[c++] = AROS_LONG2LE(fb_depth);

        vcmb_msg[c++] = AROS_LONG2LE(VCTAG_FBALLOC);
        vcmb_msg[c++] = AROS_LONG2LE(8);
        vcmb_msg[c++] = AROS_LONG2LE(0);
        vcmb_msg[c++] = AROS_LONG2LE(64);
        vcmb_msg[c++] = AROS_LONG2LE(0);

        vcmb_msg[c++] = AROS_LONG2LE(0);                      // terminate tags

        vcmb_msg[0] = AROS_LONG2LE((c << 2));                 // fill in request size

        vcmb_write(VCMB_BASE, VCMB_PROPCHAN, (void *)vcmb_msg);
        vcmb_msg = vcmb_read(VCMB_BASE, VCMB_PROPCHAN);

        if (!vcmb_msg || (vcmb_msg[1] != AROS_LONG2LE(VCTAG_RESP)))
            return 0;

        count = 2;	                        // locate the allocation request
        while((AROS_LE2LONG(vcmb_msg[count])))
        {
            if (vcmb_msg[count] == AROS_LONG2LE(VCTAG_FBALLOC))
                break;

            count += 3 + (AROS_LE2LONG(vcmb_msg[count + 1]) >> 2);

            if (count > c)
                return 0;
        }

        if (AROS_LE2LONG(vcmb_msg[count + 2]) != (VCTAG_RESP + 8))
            return 0;

        D(kprintf("%p, %p, %p, %p, %p\n", AROS_LE2LONG(vcmb_msg[count]), AROS_LE2LONG(vcmb_msg[count+1]), AROS_LE2LONG(vcmb_msg[count+2]),
        AROS_LE2LONG(vcmb_msg[count+3]),AROS_LE2LONG(vcmb_msg[count+4])));

        if (((scr_FrameBuffer = (void *)(AROS_LE2LONG(vcmb_msg[count + 3]))) == 0) || (AROS_LE2LONG(vcmb_msg[count + 4]) == 0))
            return 0;

        D(kprintf("[VCFB] scr_Framebuffer=%p, %p\n", scr_FrameBuffer, (intptr_t)scr_FrameBuffer & 0xc0000000));

        if (((intptr_t)scr_FrameBuffer & 0xc0000000) == 0x40000000)
        {
            D(kprintf("[VCFB] Buffer in L2 cache\n"));
        }
        else if (((intptr_t)scr_FrameBuffer & 0xc0000000) == 0xc0000000)
        {
            D(kprintf("[VCFB] Buffer uncached\n"));
        }
        scr_FrameBuffer = (void*)((intptr_t)scr_FrameBuffer & ~0xc0000000);
    }

    /* query the framebuffer pitch */
    {
        vcmb_msg[0] = AROS_LONG2LE(7 * 4);
        vcmb_msg[1] = AROS_LONG2LE(VCTAG_REQ);
        vcmb_msg[2] = AROS_LONG2LE(VCTAG_GETPITCH);
        vcmb_msg[3] = AROS_LONG2LE(4);
        vcmb_msg[4] = 0;
        vcmb_msg[5] = 0;
        vcmb_msg[6] = 0;		        // terminate tag

        vcmb_write(VCMB_BASE, VCMB_PROPCHAN, (void *)vcmb_msg);
        vcmb_msg = vcmb_read(VCMB_BASE, VCMB_PROPCHAN);

        if (!vcmb_msg || (vcmb_msg[4] != AROS_LONG2LE(VCTAG_RESP + 4)))
            return 0;

        if ((fb_pitch = AROS_LE2LONG(vcmb_msg[5])) == 0)
            return 0;

        D(kprintf("[VCFB] fb_pitch=%d\n", fb_pitch));
    }

    scr_Type = SCR_GFX;

    fb_Init(fb_width, fb_height, fb_depth, fb_pitch);

    return 1;
}
