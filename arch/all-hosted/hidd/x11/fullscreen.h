/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Enable fullscreen mode.
    Lang: English.
*/

int x11_fullscreen_supported(Display *display);
void x11_fullscreen_switchmode(Display *display, int *w, int *h);
