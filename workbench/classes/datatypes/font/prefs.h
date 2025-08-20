/*
 * prefs.h - preferences handling interface for Font DataTypes class
 * Copyright © 1995-96 Michael Letowski
 */

#ifndef PREFS_H
#define PREFS_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef DOS_RDARGS_H
#include <dos/rdargs.h>
#endif

#ifndef CLASSBASE_H
#include "classbase.h"
#endif

/*
 * Public constants
 */
#define TEMPLATE        "STRINGS/M,CENTER=CENTRE/S,INV=INVERSE/S,FN=FONTNAME/S,"\
                            "DPI/K,FG=FOREGROUND/K,BG=BACKGROUND/K,TRUECOLOR/S"
#define TEMPLATEDPI     "XDPI/A/N,YDPI/A/N"
#define TEMPLATECOL     "R=RED/A/N,G=GREEN/A/N,B=BLUE/A/N"

/*
 * Public structures
 */
/* User preferences for FontDT */
struct Opts {
    /* These fields are filled when parsing with TEMPLATE */
    STRPTR              *opt_Strings;                   /* Strings to display */
    SIPTR               opt_Center;                     /* Center lines */
    SIPTR               opt_Inverse;                    /* Inverted colors */
    SIPTR               opt_FontName;                   /* Use font's name as text */
    STRPTR              opt_DPIStr;                     /* DPI resolutions string */
    STRPTR              opt_ForeStr;                    /* Foreground color string */
    STRPTR              opt_BackStr;                    /* Background color string */
    PBOOL               opt_TrueColor;
    /* The following fields are filled by custom code: */
    SIPTR               opt_XDPI;                       /* XDPI */
    SIPTR               opt_YDPI;                       /* YDPI */
    SIPTR               opt_ForeCol[3];                 /* Foreground RGB */
    SIPTR               opt_BackCol[3];                 /* Background RGB */
    PBOOL               opt_DPIFlag;                    /* DPI/K parsed */
    PBOOL               opt_ForeFlag;                   /* FOREGROUND/K parsed */
    PBOOL               opt_BackFlag;                   /* BACKGROUND/K parsed */
}; /* Opts */

struct PrefsHandle {
    struct RDArgs       *ph_Args;                       /* Result of ReadArgs() call */
    struct RDArgs       *ph_RDA1;                       /* Preallocated RDArgs structures */
    struct RDArgs       *ph_RDA2;                       /* Preallocated RDArgs structures */
}; /* PrefsHandle */

/*
 * Public functions
 */
struct PrefsHandle *GetFontPrefs(struct Opts *opts);
VOID FreeFontPrefs(struct PrefsHandle *ph);

#endif	/* PREFS_H */
