#ifndef PREFS_POINTER_H
#define PREFS_POINTER_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Pointer prefs definitions
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif

#define ID_PNTR MAKE_ID('P','N','T','R')

struct PointerPrefs
{
    ULONG pp_Reserved[4];
    UWORD pp_Which;
    UWORD pp_Size;
    UWORD pp_Width;
    UWORD pp_Height;
    UWORD pp_Depth;
    UWORD pp_YSize;
    UWORD pp_X, pp_Y;
};

#define	WBP_NORMAL	0
#define	WBP_BUSY	1

struct RGBTable
{
    UBYTE t_Red;
    UBYTE t_Green;
    UBYTE t_Blue;
};

#endif /* PREFS_POINTER_H */
