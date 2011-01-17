#ifndef PREFS_WBPATTERN_H
#define PREFS_WBPATTERN_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: WBPattern prefs definitions
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif

#define ID_PTRN MAKE_ID('P','T','R','N')

struct WBPatternPrefs {
    ULONG   wbp_Reserved[4];
    UWORD   wbp_Which;
    UWORD   wbp_Flags;
    BYTE    wbp_Revision;
    BYTE    wbp_Depth;
    UWORD   wbp_DataLength;
};

/* Values for wbp_Which */
#define WBP_ROOT        0
#define WBP_DRAWER      1
#define WBP_SCREEN      2

/* Values for wbp_Flags */
#define WBPF_PATTERN            0x0001
#define WBPF_NOREMAP            0x0010

#define WBPF_DITHER_MASK        0x0300
#define WBPF_DITHER_DEF         0x0000
#define WBPF_DITHER_BAD         0x0100
#define WBPF_DITHER_GOOD        0x0200
#define WBPF_DITHER_BEST        0x0300

#define WBPF_PRECISION_MASK     0x0C00
#define WBPF_PRECISION_DEF      0x0000
#define WBPF_PRECISION_ICON     0x0400
#define WBPF_PRECISION_IMAGE    0x0800
#define WBPF_PRECISION_EXACT    0x0C00

/* Other defines */
#define MAXDEPTH        3
#define DEFPATDEPTH     2

#define PAT_WIDTH       16
#define PAT_HEIGHT      16

#endif /* PREFS_WBPATTERN_H */
