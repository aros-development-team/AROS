/*
   Copyright © 1995-2010, The AROS Development Team. All rights reserved.
   $Id$

   Desc:
   Lang: English
*/

/*********************************************************************************************/

#include <aros/macros.h>

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <prefs/prefhdr.h>
#include <devices/inputevent.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "prefs.h"
#include "misc.h"

/*********************************************************************************************/

#define PREFS_PATH_ENVARC "ENVARC:SYS/icontrol.prefs"
#define PREFS_PATH_ENV    "ENV:SYS/icontrol.prefs"

/*********************************************************************************************/

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

/*********************************************************************************************/

struct IControlPrefs icontrolprefs;

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
    struct IControlPrefs    loadprefs;
    struct IFFHandle       *iff;
    BOOL                    retval = FALSE;

    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR)fh;

        InitIFFasDOS(iff);

        if (!OpenIFF(iff, IFFF_READ))
        {
            D(bug("LoadPrefs: OpenIFF okay.\n"));

            if (!StopChunk(iff, ID_PREF, ID_ICTL))
            {
                D(bug("LoadPrefs: StopChunk okay.\n"));

                if (!ParseIFF(iff, IFFPARSE_SCAN))
                {
                    struct ContextNode *cn;

                    D(bug("LoadPrefs: ParseIFF okay.\n"));

                    cn = CurrentChunk(iff);

                    if (cn->cn_Size == sizeof(struct IControlPrefs))
                    {
                        D(bug("LoadPrefs: ID_ICTL chunk size okay.\n"));

                        if (ReadChunkBytes(iff, &loadprefs, sizeof(struct IControlPrefs)) == sizeof(struct IControlPrefs))
                        {
                            D(bug("LoadPrefs: Reading chunk successful.\n"));

                            CopyMem(loadprefs.ic_Reserved, icontrolprefs.ic_Reserved, sizeof(icontrolprefs.ic_Reserved));

                            icontrolprefs.ic_TimeOut        = AROS_BE2WORD(loadprefs.ic_TimeOut);
                            icontrolprefs.ic_MetaDrag       = AROS_BE2WORD(loadprefs.ic_MetaDrag);
                            icontrolprefs.ic_Flags          = AROS_BE2LONG(loadprefs.ic_Flags);
                            icontrolprefs.ic_WBtoFront      = loadprefs.ic_WBtoFront;
                            icontrolprefs.ic_FrontToBack    = loadprefs.ic_FrontToBack;
                            icontrolprefs.ic_ReqTrue        = loadprefs.ic_ReqTrue;
                            icontrolprefs.ic_ReqFalse       = loadprefs.ic_ReqFalse;
                            icontrolprefs.ic_Reserved2      = AROS_BE2WORD(loadprefs.ic_Reserved2);
                            icontrolprefs.ic_VDragModes[0]  = AROS_BE2WORD(loadprefs.ic_VDragModes[0]);
                            icontrolprefs.ic_VDragModes[1]  = AROS_BE2WORD(loadprefs.ic_VDragModes[1]);

                            D(bug("LoadPrefs: Everything okay :-)\n"));

                            retval = TRUE;
                        }
                    }
                } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
            } /* if (!StopChunk(iff, ID_PREF, ID_ICTL)) */
            CloseIFF(iff);
        } /* if (!OpenIFF(iff, IFFF_READ)) */
        FreeIFF(iff);
    } /* if ((iff = AllocIFF())) */
    return retval;
}

/*********************************************************************************************/

BOOL Prefs_ExportFH(BPTR fh)
{
    struct IControlPrefs    saveprefs;
    struct IFFHandle       *iff;
    BOOL                    retval = FALSE;
#if 0 /* unused */
    BOOL                    delete_if_error = FALSE;
#endif

    CopyMem(icontrolprefs.ic_Reserved, saveprefs.ic_Reserved, sizeof(icontrolprefs.ic_Reserved));

    saveprefs.ic_TimeOut        = AROS_WORD2BE(icontrolprefs.ic_TimeOut);
    saveprefs.ic_MetaDrag       = AROS_WORD2BE(icontrolprefs.ic_MetaDrag);
    saveprefs.ic_Flags          = AROS_LONG2BE(icontrolprefs.ic_Flags);
    saveprefs.ic_WBtoFront      = icontrolprefs.ic_WBtoFront;
    saveprefs.ic_FrontToBack    = icontrolprefs.ic_FrontToBack;
    saveprefs.ic_ReqTrue        = icontrolprefs.ic_ReqTrue;
    saveprefs.ic_ReqFalse       = icontrolprefs.ic_ReqFalse;
    saveprefs.ic_Reserved2      = AROS_WORD2BE(icontrolprefs.ic_Reserved2);
    saveprefs.ic_VDragModes[0]  = AROS_WORD2BE(icontrolprefs.ic_VDragModes[0]);
    saveprefs.ic_VDragModes[1]  = AROS_WORD2BE(icontrolprefs.ic_VDragModes[1]);

    D(bug("SavePrefsFH: fh: %lx\n", fh));

    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR) fh;
        D(bug("SavePrefsFH: stream opened.\n"));

#if 0 /* unused */
        delete_if_error = TRUE;
#endif

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

                        if (!PushChunk(iff, ID_PREF, ID_ICTL, sizeof(struct IControlPrefs)))
                        {
                            D(bug("SavePrefsFH: PushChunk(LCLE) okay.\n"));

                            if (WriteChunkBytes(iff, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
                            {
                                D(bug("SavePrefsFH: WriteChunkBytes(ICTL) okay.\n"));
                                D(bug("SavePrefsFH: Everything okay :-)\n"));

                                retval = TRUE;
                            }
                            PopChunk(iff);
                        } /* if (!PushChunk(iff, ID_PREF, ID_ICTL, sizeof(struct IControlPrefs))) */
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

#if 0 /* unused */
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
    icontrolprefs.ic_Reserved[0] = 0;
    icontrolprefs.ic_Reserved[1] = 0;
    icontrolprefs.ic_Reserved[2] = 0;
    icontrolprefs.ic_Reserved[3] = 0;
    icontrolprefs.ic_TimeOut = 50;
    icontrolprefs.ic_MetaDrag = IEQUALIFIER_LCOMMAND;
    icontrolprefs.ic_Flags = ICF_3DMENUS |
        ICF_MODEPROMOTE |
        ICF_MENUSNAP |
        ICF_STRGAD_FILTER |
        ICF_COERCE_LACE |
        ICF_OFFSCREENLAYERS;
        /* FIXME: check whether ICF_DEFPUBSCREEN is set as default */
    icontrolprefs.ic_WBtoFront = 'N';
    icontrolprefs.ic_FrontToBack = 'M';
    icontrolprefs.ic_ReqTrue = 'V';
    icontrolprefs.ic_ReqFalse = 'B';
    icontrolprefs.ic_VDragModes[0] = 0;
    icontrolprefs.ic_VDragModes[1] = 0;

    return TRUE;
}
