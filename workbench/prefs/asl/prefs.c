/*
   Copyright (C) 2022, The AROS Development Team. All rights reserved.

   Desc:
*/

/*********************************************************************************************/

#define DEBUG 0
#include <aros/debug.h>

#include <prefs/prefhdr.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "prefs.h"
#include "misc.h"
#include "locale.h"

/*********************************************************************************************/

#define PREFS_PATH_ENVARC "ENVARC:SYS/Asl.prefs"
#define PREFS_PATH_ENV    "ENV:SYS/Asl.prefs"

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
    struct IFFHandle    *handle;
    BOOL                 success = TRUE;


    if (!(handle = AllocIFF()))
    {
        ShowMessage(_(MSG_CANT_ALLOCATE_IFFPTR));
        return(FALSE);
    }

    handle->iff_Stream = (IPTR) fh;
    InitIFFasDOS(handle);

    if ((OpenIFF(handle, IFFF_READ)) == 0)
    {
        LONG error;
        // FIXME: We want some sanity checking here!

        if ((error = StopChunk(handle, ID_PREF, ID_ASL)) == 0)
        {
            if ((error = ParseIFF(handle, IFFPARSE_SCAN)) == 0)
            {
                struct AslPrefs loadprefs;

                error = ReadChunkBytes(handle, &loadprefs, sizeof(struct AslPrefs));

                if (error < 0)
                {
                    printf("Error: ReadChunkBytes() returned %d!\n", (int)error);
                }

                CopyMem(&loadprefs, &aslprefs, sizeof(struct AslPrefs));
                aslprefs.ap_RelativeLeft    = AROS_BE2WORD(loadprefs.ap_RelativeLeft);
                aslprefs.ap_RelativeTop     = AROS_BE2WORD(loadprefs.ap_RelativeTop);
            }
            else
            {
                printf("ParseIFF() failed, returncode %d!\n", (int)error);
                success = FALSE;
            }
        }
        else
        {
            printf("StopChunk() failed, returncode %d!\n", (int)error);
            success = FALSE;
        }

        CloseIFF(handle);
    }
    else
    {
        ShowMessage(_(MSG_CANT_OPEN_STREAM));
    }

    FreeIFF(handle);

    return success;
}

/*********************************************************************************************/

BOOL Prefs_ExportFH(BPTR fh)
{
    struct PrefHeader header;
    struct IFFHandle *handle;
    BOOL              success = TRUE;

    memset(&header, 0, sizeof(struct PrefHeader));

    if ((handle = AllocIFF()))
    {
        handle->iff_Stream = (IPTR)fh;

        InitIFFasDOS(handle);

        if (!(OpenIFF(handle, IFFF_WRITE))) /* NULL = successful! */
        {
            struct AslPrefs saveprefs = {0};
            LONG            error;

            PushChunk(handle, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN); /* FIXME: IFFSIZE_UNKNOWN? */

            header.ph_Version = PHV_CURRENT;
            header.ph_Type    = 0;

            PushChunk(handle, ID_PREF, ID_PRHD, IFFSIZE_UNKNOWN); /* FIXME: IFFSIZE_UNKNOWN? */

            WriteChunkBytes(handle, &header, sizeof(struct PrefHeader));

            PopChunk(handle);

            error = PushChunk(handle, ID_PREF, ID_ASL, sizeof(struct AslPrefs));

            if (error != 0) // TODO: We need some error checking here!
            {
                printf("error: PushChunk() = %d\n", (int)error);
            }

            CopyMem(&aslprefs, &saveprefs, sizeof(struct AslPrefs));
            saveprefs.ap_RelativeLeft   = AROS_WORD2BE(aslprefs.ap_RelativeLeft);
            saveprefs.ap_RelativeTop    = AROS_WORD2BE(aslprefs.ap_RelativeTop);

            WriteChunkBytes(handle, &saveprefs, sizeof(struct AslPrefs));
            error = PopChunk(handle);

            if (error != 0) // TODO: We need some error checking here!
            {
                printf("error: PopChunk() = %d\n", (int)error);
            }

            // Terminate the FORM
            PopChunk(handle);
        }
        else
        {
            ShowMessage(_(MSG_CANT_OPEN_STREAM));
            success = FALSE;
        }

        CloseIFF(handle);
        FreeIFF(handle);
    }
    else // AllocIFF()
    {
        // Do something more here - if IFF allocation has failed, something isn't right
        ShowMessage(_(MSG_CANT_ALLOCATE_IFFPTR));
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

    aslprefs.ap_SortBy              = 0;
    aslprefs.ap_SortDrawers         = 0; /* 0 - First, 1 - Mix, 2 - Last */
    aslprefs.ap_SortOrder           = 0;
    aslprefs.ap_SizePosition        = 0x40 /* override */ | 0x10 /* relative */ | 0x02 /* center on screen */;
    aslprefs.ap_RelativeLeft        = 0;
    aslprefs.ap_RelativeTop         = 0;
    aslprefs.ap_RelativeWidth       = 30;
    aslprefs.ap_RelativeHeight      = 75;

    return TRUE;
}
