#ifndef PREFS_WBPATTERN_H
#define PREFS_WBPATTERN_H

/*
    (C) 1997 AROS - The Amiga Research OS
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

struct WBPatternPrefs
{
    ULONG	 wbp_Reserved[4];
    UWORD	 wbp_Which;
    UWORD	 wbp_Flags;
    BYTE	 wbp_Revision;
    BYTE	 wbp_Depth;
    UWORD	 wbp_DataLength;
};

#define	WBP_ROOT	0
#define	WBP_DRAWER	1
#define	WBP_SCREEN	2

#define	WBPF_PATTERN	0x0001

#define	WBPF_NOREMAP	0x0010


#define MAXDEPTH	3
#define DEFPATDEPTH	2

#define PAT_WIDTH	16
#define PAT_HEIGHT	16


#endif /* PREFS_WBPATTERN_H */
