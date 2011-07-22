/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hardware text mode (IBM-compatible) screen console.
*/

#include <bootconsole.h>

#include "console.h"

struct scr
{
    unsigned char sign;
    unsigned char attr;
};

/*
 * There's no init function for text mode console.
 * Just set scr_Framebuffer, scr_Width and scr_Height and you are ready to go.
 */

void txt_Clear()
{
    struct scr *view = scr_FrameBuffer;
    unsigned int i;

    scr_XPos = 0;
    scr_YPos = 0;

    for (i = 0; i < scr_Width * scr_Height; i++)
    {
        view[i].sign = ' ';
        view[i].attr = 7;
    }
}

void txt_Putc(char chr)
{
    struct scr *view = scr_FrameBuffer;
    unsigned int i;

    /* Ignore null bytes, they are output by formatting routines as terminators */
    if (chr == 0)
    	return;

    /* Reached end of line ? New line if so. */
    if ((chr == '\n') || (scr_XPos >= scr_Width))
    {
        scr_XPos = 0;
        scr_YPos++;
    }

    if (scr_YPos >= scr_Height)
    {
        scr_YPos = scr_Height - 1;

        for (i = 0; i < scr_Width * scr_YPos; i++)
            view[i].sign = view[i+80].sign;
        for (i = scr_Width * scr_YPos; i < scr_Width * scr_Height; i++)
            view[i].sign = ' ';
    }

    if (chr == '\n')
    	return;

    i = 80 * scr_YPos + scr_XPos;
    view[i].sign = chr;
    scr_XPos++;
}
