/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */
#include <proto/graphics.h>
#include <proto/layers.h>

#include "workbook_intern.h"

struct Region *wbClipWindow(struct WorkbookBase *wb, struct Window *win)
{
    struct Region *clip;

    /* Install new clip region */
    if ((clip = NewRegion())) {
        struct Rectangle rect = {
        	.MinX = win->BorderLeft,
        	.MinY = win->BorderTop,
        	.MaxX = win->Width - win->BorderRight - 1,
        	.MaxY = win->Height - win->BorderBottom - 1,
	};
        if (!OrRectRegion(clip, &rect)) {
        	DisposeRegion(clip);
        	clip = NULL;
	}
    }

    /* Install new clip region */
    return InstallClipRegion(win->WLayer, clip);
}

void wbUnclipWindow(struct WorkbookBase *wb, struct Window *win, struct Region *clip)
{
    clip = InstallClipRegion(win->WLayer, clip);
    if (clip)
    	    DisposeRegion(clip);
}
