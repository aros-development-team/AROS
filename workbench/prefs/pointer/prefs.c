/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

/*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <aros/macros.h>

/* #define DEBUG 1 */
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/iffparse.h>
#include <proto/dos.h>

#include <prefs/prefhdr.h>

#include "prefs.h"
#include "misc.h"

#ifdef BIGENDIAN_PREFS
#define GET_WORD AROS_BE2WORD
#define GET_LONG AROS_BE2LONG
#else
#define GET_WORD(x) x
#define GET_LONG(x) x
#endif

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

struct NewPointerPrefs pointerprefs[MAXPOINTER];

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
    struct NewPointerPrefs  loadprefs;
    struct IFFHandle       *iff;
    BOOL                    retval = FALSE;

#if 0
    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR)fh;

        InitIFFasDOS(iff);

        if (!OpenIFF(iff, IFFF_READ))
        {
            D(bug("LoadPrefs: OpenIFF okay.\n"));

            if (!StopChunk(iff, ID_PREF, ID_SERL))
            {
                D(bug("LoadPrefs: StopChunk okay.\n"));

                if (!ParseIFF(iff, IFFPARSE_SCAN))
                {
                    struct ContextNode *cn;

                    D(bug("LoadPrefs: ParseIFF okay.\n"));

                    cn = CurrentChunk(iff);

                    if (cn->cn_Size == sizeof(struct SerialPrefs))
                    {
                        D(bug("LoadPrefs: ID_SERL chunk size okay.\n"));

                        if (ReadChunkBytes(iff, &loadprefs, sizeof(struct SerialPrefs)) == sizeof(struct SerialPrefs))
                        {
                            D(bug("LoadPrefs: Reading chunk successful.\n"));

                            CopyMemQuick(loadprefs.sp_Reserved, serialprefs.sp_Reserved, sizeof(serialprefs.sp_Reserved));
                            serialprefs.sp_Unit0Map        = GET_LONG(loadprefs.sp_Unit0Map);
                            serialprefs.sp_BaudRate        = GET_LONG(loadprefs.sp_BaudRate);
                            serialprefs.sp_InputBuffer     = GET_LONG(loadprefs.sp_InputBuffer);
                            serialprefs.sp_OutputBuffer    = GET_LONG(loadprefs.sp_OutputBuffer);
                            serialprefs.sp_InputHandshake  = loadprefs.sp_InputHandshake;
                            serialprefs.sp_OutputHandshake = loadprefs.sp_OutputHandshake;
                            serialprefs.sp_Parity	   = loadprefs.sp_Parity;
                            serialprefs.sp_BitsPerChar	   = loadprefs.sp_BitsPerChar;
                            serialprefs.sp_StopBits	   = loadprefs.sp_StopBits;

                            D(bug("LoadPrefs: Everything okay :-)\n"));

                            retval = TRUE;
                        }
                    }
                } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
            } /* if (!StopChunk(iff, ID_PREF, ID_SERL)) */
            CloseIFF(iff);
        } /* if (!OpenIFF(iff, IFFF_READ)) */
        FreeIFF(iff);
    } /* if ((iff = AllocIFF())) */
#endif
    return retval;
}

/*********************************************************************************************/

BOOL Prefs_ExportFH(BPTR fh)
{
    struct NewPointerPrefs  saveprefs;
    struct IFFHandle       *iff;
    BOOL                    retval = FALSE;
    BOOL                    delete_if_error = FALSE;

#if 0
    CopyMemQuick(serialprefs.sp_Reserved, saveprefs.sp_Reserved, sizeof(serialprefs.sp_Reserved));
    saveprefs.sp_Unit0Map           = GET_LONG(serialprefs.sp_Unit0Map);
    saveprefs.sp_BaudRate           = GET_LONG(serialprefs.sp_BaudRate);
    saveprefs.sp_InputBuffer        = GET_LONG(serialprefs.sp_InputBuffer);
    saveprefs.sp_OutputBuffer       = GET_LONG(serialprefs.sp_OutputBuffer);
    saveprefs.sp_InputHandshake     = serialprefs.sp_InputHandshake;
    saveprefs.sp_OutputHandshake    = serialprefs.sp_OutputHandshake;
    saveprefs.sp_Parity             = serialprefs.sp_Parity;
    saveprefs.sp_BitsPerChar        = serialprefs.sp_BitsPerChar;
    saveprefs.sp_StopBits           = serialprefs.sp_StopBits;

    D(bug("SavePrefsFH: fh: %lx\n", fh));

    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR) fh;
        D(bug("SavePrefsFH: stream opened.\n"));

        delete_if_error = TRUE;

        InitIFFasDOS(iff);

        if (!OpenIFF(iff, IFFF_WRITE))
        {
            D(bug("SavePrefsFH: OpenIFF okay.\n"));

            if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN))
            {
                D(bug("SavePrefsFH: PushChunk(FORM) okay.\n"));

                if (!PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct FilePrefHeader)))
                {
                    struct FilePrefHeader head;

                    D(bug("SavePrefsFH: PushChunk(PRHD) okay.\n"));

                    head.ph_Version  = PHV_CURRENT;
                    head.ph_Type     = 0;
                    head.ph_Flags[0] =
                    head.ph_Flags[1] =
                    head.ph_Flags[2] =
                    head.ph_Flags[3] = 0;

                    if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head))
                    {
                        D(bug("SavePrefsFH: WriteChunkBytes(PRHD) okay.\n"));

                        PopChunk(iff);

                        if (!PushChunk(iff, ID_PREF, ID_SERL, sizeof(struct SerialPrefs)))
                        {
                            D(bug("SavePrefsFH: PushChunk(LCLE) okay.\n"));

                            if (WriteChunkBytes(iff, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
                            {
                                D(bug("SavePrefsFH: WriteChunkBytes(SERL) okay.\n"));
                                D(bug("SavePrefsFH: Everything okay :-)\n"));

                                retval = TRUE;
                            }
                            PopChunk(iff);
                        } /* if (!PushChunk(iff, ID_PREF, ID_SERL, sizeof(struct LocalePrefs))) */
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

    return TRUE;
}

