#ifndef GRAPHICS_RPATTR_H
#define GRAPHICS_RPATTR_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Miscellaneous graphics tags
    Lang: english
*/

#define RPTAG_Font       0x80000000
#define RPTAG_APen       0x80000002
#define RPTAG_BPen       0x80000003
#define RPTAG_DrMd       0x80000004
#define RPTAG_OutlinePen 0x80000005
#define RPTAG_WriteMask  0x80000006
#define RPTAG_MaxPen     0x80000007
#define RPTAG_DrawBounds 0x80000008

/* Extensions taken over from MorphOS */
#define RPTAG_PenMode	 0x80000080
#define RPTAG_FgColor	 0x80000081
#define RPTAG_BgColor	 0x80000082

/* Extensions invented by AROS */
#define RPTAG_PatternOriginX 	    0x800000C0 /* WORD */
#define RPTAG_PatternOriginY 	    0x800000C1 /* WORD */
#define RPTAG_ClipRectangle  	    0x800000C2 /* struct Rectangle *. Clones *rectangle. */
#define RPTAG_ClipRectangleFlags    0x800000C3 /* ULONG */


/* Flags for ClipRectangleFlags */
#define RPCRF_RELRIGHT	    	    0x01       /* ClipRectangle.MaxX is relative to right of layer/bitmap */
#define RPCRF_RELBOTTOM     	    0x02       /* ClipRectangle.MaxY is relative to bottom of layer/bitmap */
#define RPCRF_VALID 	    	    0x04       /* private */

#endif /* GRAPHICS_RPATTR_H */
