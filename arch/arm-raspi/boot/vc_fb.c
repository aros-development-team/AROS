/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VideoCore framebuffer routines
    Lang: english
*/

#include "bootconsole.h"
#include "vc_mb.h"
#include "vc_fb.h"
#include "vc_tags.h"

int vcfb_init(void)
{
    unsigned int fb_width, fb_height, fb_depth, fb_pitch;
    unsigned int count;
    volatile unsigned int *vcmb_msg = (unsigned int *) MESSAGE_BUFFER;

    scr_Type = SCR_UNKNOWN;

    /* query the display dimensions */
    {
        vcmb_msg[0] = 8 * 4;
        vcmb_msg[1] = VCTAG_REQ;
        vcmb_msg[2] = VCTAG_GETRES;
        vcmb_msg[3] = 8;
        vcmb_msg[4] = 0;
        vcmb_msg[5] = 0;
        vcmb_msg[6] = 0;
        vcmb_msg[7] = 0;		        // terminate tag

        vcmb_write(VCMB_BASE, VCMB_FBCHAN, vcmb_msg);
        vcmb_msg = vcmb_read(VCMB_BASE, VCMB_FBCHAN);

        if (!vcmb_msg || (vcmb_msg[1] != VCTAG_RESP))
            return 0;

        if (((fb_width = vcmb_msg[5]) == 0) || ((fb_height = vcmb_msg[6]) == 0))
            return 0;
    }

    /* fill in our framebuffer configuration/allocation request */
    {
        unsigned int c = 1;
        vcmb_msg[c++] = VCTAG_REQ;

        vcmb_msg[c++] = VCTAG_SETRES;
        vcmb_msg[c++] = 8;
        vcmb_msg[c++] = 8;
        vcmb_msg[c++] = fb_width;
        vcmb_msg[c++] = fb_height;

        vcmb_msg[c++] = VCTAG_SETVRES;          // duplicate physical size...
        vcmb_msg[c++] = 8;
        vcmb_msg[c++] = 8;
        vcmb_msg[c++] = fb_width;
        vcmb_msg[c++] = fb_height;

        vcmb_msg[c++] = VCTAG_SETDEPTH;
        vcmb_msg[c++] = 4;
        vcmb_msg[c++] = 4;
        
        fb_depth = 16;

        vcmb_msg[c++] = fb_depth;

        vcmb_msg[c++] = VCTAG_FBALLOC;
        vcmb_msg[c++] = 8;
        vcmb_msg[c++] = 4;
        vcmb_msg[c++] = 16;
        vcmb_msg[c++] = 0;

        vcmb_msg[c++] = 0;                      // terminate tags

        vcmb_msg[0] = (c << 2);                 // fill in request size

        vcmb_write(VCMB_BASE, VCMB_FBCHAN, (unsigned int)vcmb_msg);
        vcmb_msg = vcmb_read(VCMB_BASE, VCMB_FBCHAN);

        if (!vcmb_msg || (vcmb_msg[1] != VCTAG_RESP))
            return 0;

        count = 2;	                        // locate the allocation request
        while((vcmb_msg[count]))
        {
            if (vcmb_msg[count] == VCTAG_FBALLOC)
                    break;

            count += 3 + (vcmb_msg[count + 1] >> 2);

            if (count > c)
                return 0;
        }

        if (vcmb_msg[count + 2] != (VCTAG_RESP + 8))
            return 0;

        if (((scr_FrameBuffer = vcmb_msg[count + 3]) == 0) || (vcmb_msg[count + 4] == 0))
            return 0;
    }

    /* query the framebuffer pitch */
    {
        vcmb_msg[0] = 7 * 4;
        vcmb_msg[1] = VCTAG_REQ;
        vcmb_msg[2] = VCTAG_GETPITCH;
        vcmb_msg[3] = 4;
        vcmb_msg[4] = 0;
        vcmb_msg[5] = 0;
        vcmb_msg[6] = 0;		        // terminate tag

        vcmb_write(VCMB_BASE, VCMB_FBCHAN, (unsigned int)vcmb_msg);
        vcmb_msg = vcmb_read(VCMB_BASE, VCMB_FBCHAN);

        if (!vcmb_msg || (vcmb_msg[4] != (VCTAG_RESP + 4)))
            return 0;

        if ((fb_pitch = vcmb_msg[5]) == 0)
            return 0;
    }

    scr_Type = SCR_GFX;

    fb_Init(fb_width, fb_height, fb_depth, fb_pitch);

    return 1;
}
