#ifndef PREFS_ICONTROL_H
#define PREFS_ICONTROL_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Icontrol prefs definitions
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif


#define ID_ICTL MAKE_ID('I','C','T','L')

struct IControlPrefs {
    LONG  ic_Reserved[4];
    UWORD ic_TimeOut;
    WORD  ic_MetaDrag;
    ULONG ic_Flags;
    UBYTE ic_WBtoFront;
    UBYTE ic_FrontToBack;
    UBYTE ic_ReqTrue;
    UBYTE ic_ReqFalse;
};

/* Values for ic_Flags */
#define ICB_COERCE_COLORS 0
#define ICB_COERCE_LACE   1
#define ICB_STRGAD_FILTER 2
#define ICB_MENUSNAP	  3
#define ICB_MODEPROMOTE   4

#define ICF_COERCE_COLORS (1<<0)
#define ICF_COERCE_LACE   (1<<1)
#define ICF_STRGAD_FILTER (1<<2)
#define ICF_MENUSNAP	  (1<<3)
#define ICF_MODEPROMOTE   (1<<4)

// FIXME: do we want these MOS extensions?
// FIXME: what are the correct values?
#define ICF_OPAQUEMOVE          (1<<30)
#define ICF_PRIVILEDGEDREFRESH  (1<<29)
#define ICF_OFFSCREENLAYERS     (1<<28)
#define ICF_DEFPUBSCREEN        (1<<27)
#define ICF_SCREENACTIVATION    (1<<26)

/* AROS extension */
#define ICF_3DMENUS     	(1<<16)
#define ICF_POPUPMENUS	    	(1<<15)
#define ICF_AVOIDWINBORDERERASE (1<<14)

#endif /* PREFS_ICONTROL_H */
