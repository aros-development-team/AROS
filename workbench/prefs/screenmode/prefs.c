/*
    Copyright � 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <prefs/screenmode.h>
#include <prefs/prefhdr.h>
#include <graphics/modeid.h>

#include <proto/dos.h>
#include <proto/iffparse.h>

#include <stdio.h>
#include <string.h>

#include "prefs.h"
#include "misc.h"

/*********************************************************************************************/

#define PREFS_PATH_ENVARC "ENVARC:SYS/screenmode.prefs"
#define PREFS_PATH_ENV    "ENV:SYS/screenmode.prefs"

/*********************************************************************************************/

struct ScreenModePrefs screenmodeprefs;

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
    struct ContextNode     *context;
    struct IFFHandle       *handle;
    struct ScreenModePrefs  loadprefs = {{0},0};
    BOOL                    success = TRUE;
    LONG                    error;

    if (!(handle = AllocIFF()))
        return FALSE;

    handle->iff_Stream = (IPTR)fh;
    InitIFFasDOS(handle);

    if ((error = OpenIFF(handle, IFFF_READ)) == 0)
    {
        // FIXME: We want some sanity checking here!
        if ((error = StopChunk(handle, ID_PREF, ID_SCRM)) == 0)
        {
            if ((error = ParseIFF(handle, IFFPARSE_SCAN)) == 0)
            {
                context = CurrentChunk(handle);

                error = ReadChunkBytes
                (
                    handle, &loadprefs, sizeof(struct ScreenModePrefs)
                );

                if (error < 0)
                {
                    printf("Error: ReadChunkBytes() returned %d!\n", error);
                }
                else
                {
		    /* FIXME: in original prefs file all values are bigendian! */
		    screenmodeprefs.smp_DisplayID = loadprefs.smp_DisplayID;
		    screenmodeprefs.smp_Width     = loadprefs.smp_Width;
		    screenmodeprefs.smp_Height    = loadprefs.smp_Height;
		    screenmodeprefs.smp_Depth     = loadprefs.smp_Depth;
		    screenmodeprefs.smp_Control   = AROS_BE2WORD(loadprefs.smp_Control);
                }
            }
            else
            {
                printf("ParseIFF() failed, returncode %d!\n", error);
                success = FALSE;
            }
        }
        else
        {
            printf("StopChunk() failed, returncode %d!\n", error);
            success = FALSE;
        }

        CloseIFF(handle);
    }
    else
    {
        //ShowError(_(MSG_CANT_OPEN_STREAM));
    }

    FreeIFF(handle);

    return success;
}

/*********************************************************************************************/

BOOL Prefs_ExportFH(BPTR fh)
{
    struct PrefHeader       header = { 0 };
    struct IFFHandle       *handle;
    struct ScreenModePrefs  saveprefs;
    BOOL                    success = TRUE;
    LONG                    error   = 0;

    /* FIXME: in original prefs file all values are bigendian! */
    saveprefs.smp_DisplayID = screenmodeprefs.smp_DisplayID;
    saveprefs.smp_Width     = screenmodeprefs.smp_Width;
    saveprefs.smp_Height    = screenmodeprefs.smp_Height;
    saveprefs.smp_Depth     = screenmodeprefs.smp_Depth;
    saveprefs.smp_Control   = AROS_WORD2BE(screenmodeprefs.smp_Control);

    if ((handle = AllocIFF()))
    {
        handle->iff_Stream = (IPTR)fh;

        InitIFFasDOS(handle);

        if (!(error = OpenIFF(handle, IFFF_WRITE))) /* NULL = successful! */
        {
            PushChunk(handle, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN); /* FIXME: IFFSIZE_UNKNOWN? */

            header.ph_Version = PHV_CURRENT;
            header.ph_Type    = 0;

            PushChunk(handle, ID_PREF, ID_PRHD, IFFSIZE_UNKNOWN); /* FIXME: IFFSIZE_UNKNOWN? */

            WriteChunkBytes(handle, &header, sizeof(struct PrefHeader));

            PopChunk(handle);

            error = PushChunk(handle, ID_PREF, ID_SCRM, sizeof(struct ScreenModePrefs));

            if (error != 0) // TODO: We need some error checking here!
            {
                printf("error: PushChunk() = %d ", error);
            }

            error = WriteChunkBytes(handle, &saveprefs, sizeof(struct ScreenModePrefs));
            error = PopChunk(handle);

            if (error != 0) // TODO: We need some error checking here!
            {
                printf("error: PopChunk() = %d ", error);
            }

            // Terminate the FORM
            PopChunk(handle);
        }
        else
        {
            //ShowError(_(MSG_CANT_OPEN_STREAM));
            success = FALSE;
        }

        CloseIFF(handle);
        FreeIFF(handle);
    }
    else // AllocIFF()
    {
        // Do something more here - if IFF allocation has failed, something isn't right
        //ShowError(_(MSG_CANT_ALLOCATE_IFFPTR));
        success = FALSE;
    }

    return success;
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
            ShowMessage("Can't open " PREFS_PATH_ENV " for writing.");
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
            ShowMessage("Can't open " PREFS_PATH_ENVARC " for writing.");
        }
    }
    return TRUE;
}

/*********************************************************************************************/

BOOL Prefs_Default(VOID)
{
    screenmodeprefs.smp_Reserved[0] = 0;
    screenmodeprefs.smp_Reserved[1] = 0;
    screenmodeprefs.smp_Reserved[2] = 0;
    screenmodeprefs.smp_Reserved[3] = 0;
    screenmodeprefs.smp_DisplayID   = 0; // FIXME ???
    screenmodeprefs.smp_Width       = AROS_DEFAULT_WBWIDTH;
    screenmodeprefs.smp_Height      = AROS_DEFAULT_WBHEIGHT;
    screenmodeprefs.smp_Depth       = AROS_DEFAULT_WBDEPTH;
    screenmodeprefs.smp_Control     = 0;

    return TRUE;
}
