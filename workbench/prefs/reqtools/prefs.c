/*
   Copyright © 2013, The AROS Development Team. All rights reserved.
   $Id$

   Desc:
   Lang: English
*/

/*********************************************************************************************/

#include <aros/macros.h>

#define DEBUG 1
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

#define PREFS_PATH_ENVARC "ENVARC:ReqTools.prefs"
#define PREFS_PATH_ENV    "ENV:ReqTools.prefs"

/*********************************************************************************************/

struct ReqToolsPrefs reqtoolsprefs;

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
#define loadprefs reqtoolsprefs
#else
    struct ReqToolsPrefs    loadprefs;
#endif
    BOOL                    retval = FALSE;
    int                     i;

    if (Read(fh, &loadprefs, sizeof(loadprefs)) == sizeof(loadprefs))
    {
#if (!(AROS_BIG_ENDIAN))
        reqtoolsprefs.Flags = AROS_BE2LONG(loadprefs.Flags);
	for(i = 0;i < RTPREF_NR_OF_REQ; i++)
	{
	    reqtoolsprefs.ReqDefaults[i].Size = AROS_BE2LONG(loadprefs.ReqDefaults[i].Size);
	    reqtoolsprefs.ReqDefaults[i].ReqPos = AROS_BE2LONG(loadprefs.ReqDefaults[i].ReqPos);
	    reqtoolsprefs.ReqDefaults[i].LeftOffset = AROS_BE2WORD(loadprefs.ReqDefaults[i].LeftOffset);
	    reqtoolsprefs.ReqDefaults[i].TopOffset = AROS_BE2WORD(loadprefs.ReqDefaults[i].TopOffset);
	    reqtoolsprefs.ReqDefaults[i].MinEntries = AROS_BE2WORD(loadprefs.ReqDefaults[i].MinEntries);
	    reqtoolsprefs.ReqDefaults[i].MaxEntries = AROS_BE2WORD(loadprefs.ReqDefaults[i].MaxEntries);	    
	}
#endif
        retval = TRUE;
    }
    
    return retval;
}

/*********************************************************************************************/

BOOL Prefs_ExportFH(BPTR fh)
{
#if (AROS_BIG_ENDIAN)
#define saveprefs reqtoolsprefs
#else
    struct ReqToolsPrefs    saveprefs;
#endif
    BOOL                    retval = FALSE;
    int                     i;

    D(bug("SavePrefsFH: fh: %lx\n", fh));

#if (!(AROS_BIG_ENDIAN))
    saveprefs.Flags = AROS_LONG2BE(reqtoolsprefs.Flags);
    for(i = 0; i < RTPREF_NR_OF_REQ; i++)
    {
        saveprefs.ReqDefaults[i].Size = AROS_LONG2BE(reqtoolsprefs.ReqDefaults[i].Size);
        saveprefs.ReqDefaults[i].ReqPos = AROS_LONG2BE(reqtoolsprefs.ReqDefaults[i].ReqPos);
        saveprefs.ReqDefaults[i].LeftOffset = AROS_WORD2BE(reqtoolsprefs.ReqDefaults[i].LeftOffset);
        saveprefs.ReqDefaults[i].TopOffset = AROS_WORD2BE(reqtoolsprefs.ReqDefaults[i].TopOffset);
        saveprefs.ReqDefaults[i].MinEntries = AROS_WORD2BE(reqtoolsprefs.ReqDefaults[i].MinEntries);
        saveprefs.ReqDefaults[i].MaxEntries = AROS_WORD2BE(reqtoolsprefs.ReqDefaults[i].MaxEntries);	    
    }
#endif
    if (Write(fh, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
    {
        retval = TRUE;
    }

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
    reqtoolsprefs.Flags = 0;
    
    reqtoolsprefs.ReqDefaults[ RTPREF_OTHERREQ      ].ReqPos = REQPOS_POINTER;
    reqtoolsprefs.ReqDefaults[ RTPREF_FILEREQ       ].Size = 75;
    reqtoolsprefs.ReqDefaults[ RTPREF_FONTREQ       ].Size =
    reqtoolsprefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].Size =
    reqtoolsprefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].Size = 65;
    reqtoolsprefs.ReqDefaults[ RTPREF_FILEREQ       ].ReqPos =
    reqtoolsprefs.ReqDefaults[ RTPREF_FONTREQ       ].ReqPos =
    reqtoolsprefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].ReqPos =
    reqtoolsprefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].ReqPos =
    reqtoolsprefs.ReqDefaults[ RTPREF_PALETTEREQ    ].ReqPos = REQPOS_TOPLEFTSCR;
    reqtoolsprefs.ReqDefaults[ RTPREF_FILEREQ       ].LeftOffset =
    reqtoolsprefs.ReqDefaults[ RTPREF_FONTREQ       ].LeftOffset =
    reqtoolsprefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].LeftOffset =
    reqtoolsprefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].LeftOffset =
    reqtoolsprefs.ReqDefaults[ RTPREF_PALETTEREQ    ].LeftOffset =
    reqtoolsprefs.ReqDefaults[ RTPREF_OTHERREQ      ].LeftOffset = 25;
    reqtoolsprefs.ReqDefaults[ RTPREF_FILEREQ       ].TopOffset =
    reqtoolsprefs.ReqDefaults[ RTPREF_FONTREQ       ].TopOffset =
    reqtoolsprefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].TopOffset =
    reqtoolsprefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].TopOffset =
    reqtoolsprefs.ReqDefaults[ RTPREF_PALETTEREQ    ].TopOffset =
    reqtoolsprefs.ReqDefaults[ RTPREF_OTHERREQ      ].TopOffset = 18;
    reqtoolsprefs.ReqDefaults[ RTPREF_FILEREQ       ].MinEntries = 10;
    reqtoolsprefs.ReqDefaults[ RTPREF_FONTREQ       ].MinEntries =
    reqtoolsprefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].MinEntries =
    reqtoolsprefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].MinEntries = 6;
    reqtoolsprefs.ReqDefaults[ RTPREF_FILEREQ       ].MaxEntries = 50;
    reqtoolsprefs.ReqDefaults[ RTPREF_FONTREQ       ].MaxEntries =
    reqtoolsprefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].MaxEntries =
    reqtoolsprefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].MaxEntries = 10;

    return TRUE;
}
