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

#endif /* GRAPHICS_RPATTR_H */
