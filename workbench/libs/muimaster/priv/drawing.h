/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __DRAWING_H__
#define __DRAWING_H__

void
zune_draw_image (struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img,
		 LONG left, LONG top, LONG width, LONG height,
		 LONG xoffset, LONG yoffset, LONG flags);

void
__zune_images_init(void);


#endif

