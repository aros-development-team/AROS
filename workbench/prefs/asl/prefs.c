/*
   Copyright (C) 2022, The AROS Development Team. All rights reserved.

   Desc:
*/

/*********************************************************************************************/

#include <aros/macros.h>

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <aros/cpu.h>
#include <devices/inputevent.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "prefs.h"
#include "misc.h"

/*********************************************************************************************/

#define PREFS_PATH_ENVARC "ENVARC:Asl.prefs"
#define PREFS_PATH_ENV    "ENV:Asl.prefs"

/*********************************************************************************************/

struct AslPrefs aslprefs;

/*********************************************************************************************/

static BOOL Prefs_Load(STRPTR from)
{
    BOOL retval = FALSE;

    BPTR fh = Open(from, MODE_OLDFILE);
    if (fh)
    {
        retval = Prefs_ImportFH(fh);
        Close(fh);
    }

    return retval;
}

/*********************************************************************************************/

BOOL Prefs_ImportFH(BPTR fh)
{
#if (AROS_BIG_ENDIAN)
#define loadprefs aslprefs
#else
    struct AslPrefs    loadprefs;
    int                     i;
#endif
    BOOL                    retval = FALSE;

//     if (Read(fh, &loadprefs, sizeof(loadprefs)) == sizeof(loadprefs))
//     {
// #if (!(AROS_BIG_ENDIAN))
//         aslprefs.Flags = AROS_BE2LONG(loadprefs.Flags);
//         for(i = 0;i < RTPREF_NR_OF_REQ; i++)
//         {
//             aslprefs.ReqDefaults[i].Size = AROS_BE2LONG(loadprefs.ReqDefaults[i].Size);
//             aslprefs.ReqDefaults[i].ReqPos = AROS_BE2LONG(loadprefs.ReqDefaults[i].ReqPos);
//             aslprefs.ReqDefaults[i].LeftOffset = AROS_BE2WORD(loadprefs.ReqDefaults[i].LeftOffset);
//             aslprefs.ReqDefaults[i].TopOffset = AROS_BE2WORD(loadprefs.ReqDefaults[i].TopOffset);
//             aslprefs.ReqDefaults[i].MinEntries = AROS_BE2WORD(loadprefs.ReqDefaults[i].MinEntries);
//             aslprefs.ReqDefaults[i].MaxEntries = AROS_BE2WORD(loadprefs.ReqDefaults[i].MaxEntries);
//         }
// #endif
//         retval = TRUE;
//     }
    
    return retval;
}

/*********************************************************************************************/

BOOL Prefs_ExportFH(BPTR fh)
{
#if (AROS_BIG_ENDIAN)
#define saveprefs aslprefs
#else
    struct AslPrefs    saveprefs;
    int                     i;
#endif
    BOOL                    retval = FALSE;

    D(bug("SavePrefsFH: fh: %lx\n", fh));

// #if (!(AROS_BIG_ENDIAN))
//     saveprefs.Flags = AROS_LONG2BE(aslprefs.Flags);
//     for(i = 0; i < RTPREF_NR_OF_REQ; i++)
//     {
//         saveprefs.ReqDefaults[i].Size = AROS_LONG2BE(aslprefs.ReqDefaults[i].Size);
//         saveprefs.ReqDefaults[i].ReqPos = AROS_LONG2BE(aslprefs.ReqDefaults[i].ReqPos);
//         saveprefs.ReqDefaults[i].LeftOffset = AROS_WORD2BE(aslprefs.ReqDefaults[i].LeftOffset);
//         saveprefs.ReqDefaults[i].TopOffset = AROS_WORD2BE(aslprefs.ReqDefaults[i].TopOffset);
//         saveprefs.ReqDefaults[i].MinEntries = AROS_WORD2BE(aslprefs.ReqDefaults[i].MinEntries);
//         saveprefs.ReqDefaults[i].MaxEntries = AROS_WORD2BE(aslprefs.ReqDefaults[i].MaxEntries);
//     }
// #endif
//     if (Write(fh, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
//     {
//         retval = TRUE;
//     }

    return retval;
}

/*********************************************************************************************/

BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save)
{
    BPTR fh;

    if (from)
    {
        if (!Prefs_Load(from))
        {
            ShowMessage("Can't read from input file");
            return FALSE;
        }
    }
    else
    {
        if (!Prefs_Load(PREFS_PATH_ENV))
        {
            if (!Prefs_Load(PREFS_PATH_ENVARC))
            {
                ShowMessage
                (
                    "Can't read from file " PREFS_PATH_ENVARC
                    ".\nUsing default values."
                );
                Prefs_Default();
            }
        }
    }

    if (use || save)
    {
        fh = Open(PREFS_PATH_ENV, MODE_NEWFILE);
        if (fh)
        {
            Prefs_ExportFH(fh);
            Close(fh);
        }
        else
        {
            ShowMessage("Cant' open " PREFS_PATH_ENV " for writing.");
        }
    }
    if (save)
    {
        fh = Open(PREFS_PATH_ENVARC, MODE_NEWFILE);
        if (fh)
        {
            Prefs_ExportFH(fh);
            Close(fh);
        }
        else
        {
            ShowMessage("Cant' open " PREFS_PATH_ENVARC " for writing.");
        }
    }
    return TRUE;
}

/*********************************************************************************************/

BOOL Prefs_Default(VOID)
{
    // aslprefs.Flags = 0;
    
    // aslprefs.ReqDefaults[ RTPREF_OTHERREQ      ].ReqPos = REQPOS_POINTER;
    // aslprefs.ReqDefaults[ RTPREF_FILEREQ       ].Size = 75;
    // aslprefs.ReqDefaults[ RTPREF_FONTREQ       ].Size =
    // aslprefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].Size =
    // aslprefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].Size = 65;
    // aslprefs.ReqDefaults[ RTPREF_FILEREQ       ].ReqPos =
    // aslprefs.ReqDefaults[ RTPREF_FONTREQ       ].ReqPos =
    // aslprefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].ReqPos =
    // aslprefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].ReqPos =
    // aslprefs.ReqDefaults[ RTPREF_PALETTEREQ    ].ReqPos = REQPOS_TOPLEFTSCR;
    // aslprefs.ReqDefaults[ RTPREF_FILEREQ       ].LeftOffset =
    // aslprefs.ReqDefaults[ RTPREF_FONTREQ       ].LeftOffset =
    // aslprefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].LeftOffset =
    // aslprefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].LeftOffset =
    // aslprefs.ReqDefaults[ RTPREF_PALETTEREQ    ].LeftOffset =
    // aslprefs.ReqDefaults[ RTPREF_OTHERREQ      ].LeftOffset = 25;
    // aslprefs.ReqDefaults[ RTPREF_FILEREQ       ].TopOffset =
    // aslprefs.ReqDefaults[ RTPREF_FONTREQ       ].TopOffset =
    // aslprefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].TopOffset =
    // aslprefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].TopOffset =
    // aslprefs.ReqDefaults[ RTPREF_PALETTEREQ    ].TopOffset =
    // aslprefs.ReqDefaults[ RTPREF_OTHERREQ      ].TopOffset = 18;
    // aslprefs.ReqDefaults[ RTPREF_FILEREQ       ].MinEntries = 10;
    // aslprefs.ReqDefaults[ RTPREF_FONTREQ       ].MinEntries =
    // aslprefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].MinEntries =
    // aslprefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].MinEntries = 6;
    // aslprefs.ReqDefaults[ RTPREF_FILEREQ       ].MaxEntries = 50;
    // aslprefs.ReqDefaults[ RTPREF_FONTREQ       ].MaxEntries =
    // aslprefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].MaxEntries =
    // aslprefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].MaxEntries = 10;

    return TRUE;
}
