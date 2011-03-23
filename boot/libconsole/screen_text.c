/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: text mode screen output functions.
*/

#include <bootconsole.h>

#include "console.h"

struct scr
{
    unsigned char sign;
    unsigned char attr;
};

void txtClear()
{
    struct scr *view = scr_FrameBuffer;
    unsigned int i;

    for (i = 0; i < scr_Width * scr_Height; i++)
    {
        view[i].sign = ' ';
        view[i].attr = 7;
    }
}

void txtPutc(char chr)
{
    struct scr *view = scr_FrameBuffer;

    if (chr == '\n')
    {
        scr_XPos = 0;
        scr_YPos++;
    }
    else
    {
        unsigned int i = 80 * scr_YPos + scr_XPos;

        view[i].sign = chr;
        scr_XPos++;

        if (scr_XPos == scr_Width)
        {
            scr_XPos = 0;
            scr_YPos++;
	}
    }
    if (scr_YPos >= scr_Height)
    {
        int i;

        scr_YPos = scr_Height - 1;
        
        for (i = 0; i < scr_Width * scr_YPos; i++)
            view[i].sign = view[i+80].sign;
        for (i = scr_Width * scr_YPos; i < scr_Width * scr_Height; i++)
            view[i].sign = ' ';
    }
}
