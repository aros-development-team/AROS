#ifndef PREFS_PREFHDR_H
#define PREFS_PREFHDR_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    File format for preferences header.
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

/* ---------------------------------------------------------------------- */

#define ID_PREF     MAKE_ID('P','R','E','F')
#define ID_PRHD     MAKE_ID('P','R','H','D')

/*
    Preferences header which must exist at the start of all preferences files.
    The ph_Type and ph_Flags are unused at the moment.
*/

struct PrefHeader 
{
    UBYTE   ph_Version;     /* The version of the PrefHeader data */
    UBYTE   ph_Type;        /* The type of the PrefHeader data */
    ULONG   ph_Flags;       /* Flags, set to 0 for now */
} __packed;

#define PHV_AMIGAOS     0               /* Format from AmigaOS v36+ */
#define PHV_CURRENT     PHV_AMIGAOS     /* The current version */

#endif /* PREFS_PREFHDR_H */
