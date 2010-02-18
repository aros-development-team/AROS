/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

/*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <aros/macros.h>

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/iffparse.h>
#include <proto/dos.h>

#include <prefs/prefhdr.h>

#include "prefs.h"
#include "misc.h"

/*********************************************************************************************/

#define PREFS_PATH_ENVARC "ENVARC:SYS/pointer.prefs"
#define PREFS_PATH_ENV    "ENV:SYS/pointer.prefs"

/*********************************************************************************************/

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

/*********************************************************************************************/

struct ExtPointerPrefs pointerprefs[MAXPOINTER];

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
    struct ExtPointerPrefs  loadprefs;
    struct IFFHandle       *iff;
    BOOL                    retval = FALSE;

    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR)fh;

        InitIFFasDOS(iff);

        if (!OpenIFF(iff, IFFF_READ))
        {
            D(bug("Prefs_ImportFH: OpenIFF okay.\n"));

            if (!StopChunk(iff, ID_PREF, ID_NPTR))
            {
                D(bug("Prefs_ImportFH: StopChunk okay.\n"));

                while (!ParseIFF(iff, IFFPARSE_SCAN))
                {
                    struct ContextNode *cn;

                    D(bug("Prefs_ImportFH: ParseIFF okay.\n"));

                    memset(&loadprefs, 0, sizeof(loadprefs));
                    cn = CurrentChunk(iff);

                    if (cn->cn_Size >= sizeof(struct NewPointerPrefs))
                    {
                        D(bug("Prefs_ImportFH: ID_NPTR chunk size okay.\n"));

                        if (ReadChunkBytes(iff, &loadprefs, sizeof(struct ExtPointerPrefs)) >= sizeof(struct NewPointerPrefs))
                        {
                            D(bug("Prefs_ImportFH: Reading chunk successful.\n"));

                            UWORD which = AROS_BE2WORD(loadprefs.npp.npp_Which);
                            if (which < MAXPOINTER)
                            {
                                pointerprefs[which].npp.npp_Which = which;
                                pointerprefs[which].npp.npp_AlphaValue = AROS_BE2WORD(loadprefs.npp.npp_AlphaValue);
                                pointerprefs[which].npp.npp_WhichInFile = AROS_BE2LONG(loadprefs.npp.npp_WhichInFile);
                                pointerprefs[which].npp.npp_X = AROS_BE2WORD(loadprefs.npp.npp_X);
                                pointerprefs[which].npp.npp_Y = AROS_BE2WORD(loadprefs.npp.npp_Y);
                                strlcpy(pointerprefs[which].filename, loadprefs.filename, NAMEBUFLEN);

                                D(bug("Prefs_ImportFH: which %d name %s\n", which, pointerprefs[which].filename));

                            }
                            D(bug("Prefs_ImportFH: Everything okay :-)\n"));

                            retval = TRUE;
                        }
                    }
                } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
            } /* if (!StopChunk(iff, ID_PREF, ID_SERL)) */
            CloseIFF(iff);
        } /* if (!OpenIFF(iff, IFFF_READ)) */
        FreeIFF(iff);
    } /* if ((iff = AllocIFF())) */

    return retval;
}

/*********************************************************************************************/

BOOL Prefs_ExportFH(BPTR fh)
{
    struct ExtPointerPrefs  saveprefs;
    struct IFFHandle       *iff;
    BOOL                    retval = FALSE;
    BOOL                    delete_if_error = FALSE;
    LONG                    i;

    D(bug("Prefs_ExportFH: fh: %lx\n", fh));

    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR) fh;
        D(bug("Prefs_ExportFH: stream opened.\n"));

        delete_if_error = TRUE;

        InitIFFasDOS(iff);

        if (!OpenIFF(iff, IFFF_WRITE))
        {
            D(bug("Prefs_ExportFH: OpenIFF okay.\n"));

            if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN))
            {
                D(bug("Prefs_ExportFH: PushChunk(FORM) okay.\n"));

                if (!PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct FilePrefHeader)))
                {
                    struct FilePrefHeader head;

                    D(bug("Prefs_ExportFH: PushChunk(PRHD) okay.\n"));

                    head.ph_Version  = PHV_CURRENT;
                    head.ph_Type     = 0;
                    head.ph_Flags[0] =
                    head.ph_Flags[1] =
                    head.ph_Flags[2] =
                    head.ph_Flags[3] = 0;

                    if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head))
                    {
                        D(bug("Prefs_ExportFH: WriteChunkBytes(PRHD) okay.\n"));

                        PopChunk(iff);

                        for (i = 0; i < MAXPOINTER; i++)
                        {
                            saveprefs.npp.npp_Which = AROS_WORD2BE(i);
                            saveprefs.npp.npp_AlphaValue = AROS_WORD2BE(pointerprefs[i].npp.npp_AlphaValue);
                            saveprefs.npp.npp_WhichInFile = AROS_LONG2BE(pointerprefs[i].npp.npp_WhichInFile);
                            saveprefs.npp.npp_X = AROS_WORD2BE(pointerprefs[i].npp.npp_X);
                            saveprefs.npp.npp_Y = AROS_WORD2BE(pointerprefs[i].npp.npp_Y);
                            strlcpy(saveprefs.filename, pointerprefs[i].filename, NAMEBUFLEN);

                            ULONG chunksize = sizeof(struct NewPointerPrefs) + strlen(saveprefs.filename) + 1;
                            D(bug("Prefs_ExportFH: size %d name %s\n", chunksize, saveprefs.filename));

                            if (!PushChunk(iff, ID_PREF, ID_NPTR, chunksize))
                            {
                                D(bug("Prefs_ExportFH: PushChunk(NPTR) okay.\n"));

                                if (WriteChunkBytes(iff, &saveprefs, chunksize) == chunksize)
                                {
                                    D(bug("Prefs_ExportFH: WriteChunkBytes(NPTR) okay.\n"));
                                    D(bug("Prefs_ExportFH: Everything okay :-)\n"));

                                    retval = TRUE;
                                }
                                PopChunk(iff);
                            } /* if (!PushChunk(iff, ID_PREF, ID_SERL, sizeof(struct LocalePrefs))) */
                        } /* for */
                    } /* if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head)) */
                    else
                    {
                        PopChunk(iff);
                    }
                } /* if (!PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct PrefHeader))) */
                PopChunk(iff);
            } /* if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN)) */
            CloseIFF(iff);
        } /* if (!OpenIFF(iff, IFFFWRITE)) */
        FreeIFF(iff);

    } /* if ((iff = AllocIFF())) */

    #if 0
    if (!retval && delete_if_error)
    {
        DeleteFile(filename);
    }
    #endif

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
    ULONG i;

    memset(pointerprefs, 0, sizeof(pointerprefs));
    for (i = 0; i < MAXPOINTER; i++)
    {
        pointerprefs[i].npp.npp_Which = i;
        pointerprefs[i].npp.npp_AlphaValue = 0xffff;
        strcpy(pointerprefs[i].filename, "Images:Pointers/");
    }

    return TRUE;
}
