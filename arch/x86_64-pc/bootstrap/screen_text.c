/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: text mode screen output functions.
*/

#include "screen.h"

struct scr
{
    unsigned char sign;
    unsigned char attr;
};

void txtClear()
{
    struct scr *view = fb;
    unsigned int i;

    for (i = 0; i < wc * hc; i++)
    {
        view[i].sign = ' ';
        view[i].attr = 7;
    }
}

void txtPutc(char chr)
{
    struct scr *view = fb;

    if (chr == '\n')
    {
        x = 0;
        y++;
    }
    else
    {
        unsigned int i = 80 * y + x;

        view[i].sign = chr;
        x++;

        if (x == wc)
        {
            x = 0;
            y++;
	}
    }
    if (y >= hc)
    {
        int i;

        y = hc - 1;
        
        for (i = 0; i < wc * y; i++)
            view[i].sign = view[i+80].sign;
        for (i = wc * y; i < wc * hc; i++)
            view[i].sign = ' ';
    }
}
