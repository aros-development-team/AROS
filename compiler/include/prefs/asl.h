#ifndef PREFS_ASL_H
#define PREFS_ASL_H

/*
    Copyright © 2022, The AROS Development Team. All rights reserved.

    Desc: Asl prefs definitions
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif

#define ID_ASL MAKE_ID('A','S','L',' ')

struct AslPrefs
{
    LONG    ap_Reserved[4];
    UBYTE   ap_SortBy;
    UBYTE   ap_SortDrawers;
    UBYTE   ap_SortOrder;
    UBYTE   ap_SizePosition;
    WORD    ap_RelativeLeft;
    WORD    ap_RelativeTop;
    UBYTE   ap_RelativeWidth;
    UBYTE   ap_RelativeHeight;
} __packed;

#endif /* PREFS_ASL_H */
