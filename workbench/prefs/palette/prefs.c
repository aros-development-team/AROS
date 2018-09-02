/*
    Copyright © 2010-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/iffparse.h>
#include <proto/dos.h>

#include <aros/macros.h>
#include <prefs/prefhdr.h>
#include <datatypes/pictureclass.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "prefs.h"
#include "misc.h"

//#define DELETEONERROR

/*********************************************************************************************/

#define PREFS_PATH_ENVARC "ENVARC:SYS/palette.prefs"
#define PREFS_PATH_ENV    "ENV:SYS/palette.prefs"

/*********************************************************************************************/

#define MAXPENS       8

STATIC UWORD defaultpen[MAXPENS * 4] =
{
    0, 1, 1, 2, 1, 3, 1, 0, 2, 1, 2, 1, 65535
};

STATIC struct ColorSpec defaultcolor[MAXPENS * 4] =
{
    {0, 43690,  43690,  43690   },
    {1, 0,      0,      0       },
    {2, 65535,  65535,  65535   },
    {3, 21845,  30583,  43690   },
    {4, 26214,  26214,  26214   },
    {5, 61166,  61166,  61166   },
    {6, 56797,  30583,  17476   },
    {7, 65535,  61166,  4369    },
    {65535                      }
};

/*********************************************************************************************/
struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

/*********************************************************************************************/

struct PalettePrefs paletteprefs;

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
    struct IFFHandle   *iff;
    BOOL                retval = FALSE;
    LONG                i;

    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR)fh;

        InitIFFasDOS(iff);

        if (!OpenIFF(iff, IFFF_READ))
        {
            D(bug("Prefs_ImportFH: OpenIFF okay.\n"));

            if (!StopChunk(iff, ID_PREF, ID_PALT))
            {
                struct PalettePrefs loadprefs;
                D(bug("Prefs_ImportFH: ID_PREF->ID_PALT\n"));

                if (!ParseIFF(iff, IFFPARSE_SCAN))
                {
                    struct ContextNode *cn;

                    D(bug("Prefs_ImportFH: ParseIFF okay.\n"));

                    cn = CurrentChunk(iff);

                    if (cn->cn_Size == sizeof(struct PalettePrefs))
                    {
                        D(bug("Prefs_ImportFH: ID_PALT chunk size okay.\n"));

                        if (ReadChunkBytes(iff, &loadprefs, sizeof(struct PalettePrefs)) == sizeof(struct PalettePrefs))
                        {
                            D(bug("Prefs_ImportFH: Reading chunk successful.\n"));

#if (AROS_BIG_ENDIAN == 0)
                            CopyMem(loadprefs.pap_Reserved, paletteprefs.pap_Reserved, sizeof(paletteprefs.pap_Reserved));
                            for (i = 0; i < 32; i++)
                            {
                                paletteprefs.pap_4ColorPens[i] = AROS_BE2WORD(loadprefs.pap_4ColorPens[i]);

                                paletteprefs.pap_8ColorPens[i] = AROS_BE2WORD(loadprefs.pap_8ColorPens[i]);

                                paletteprefs.pap_Colors[i].ColorIndex   = AROS_BE2WORD(loadprefs.pap_Colors[i].ColorIndex);
                                paletteprefs.pap_Colors[i].Red          = AROS_BE2WORD(loadprefs.pap_Colors[i].Red);
                                paletteprefs.pap_Colors[i].Green        = AROS_BE2WORD(loadprefs.pap_Colors[i].Green);
                                paletteprefs.pap_Colors[i].Blue         = AROS_BE2WORD(loadprefs.pap_Colors[i].Blue);
                            }
#else
                            CopyMem(loadprefs, paletteprefs, sizeof(paletteprefs));
#endif
                            for (i = 0; i < MAXPENS; i++)
                            {
                                if (paletteprefs.pap_8ColorPens[i] >= PEN_C3)
                                    paletteprefs.pap_8ColorPens[i] -= (PEN_C3 - 4);
                            }
                            D(bug("Prefs_ImportFH: Everything okay :-)\n"));

                            retval = TRUE;
                        }
                    }
                } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
            }
            else if (!StopChunk(iff, ID_ILBM, ID_CMAP))
            {
                struct ColorRegister colreg;
                D(bug("Prefs_ImportFH: ID_ILBM->ID_CMAP\n"));

                for (i = 0; i < MAXPENS; i++)
                {
                    if (ReadChunkBytes(iff, &colreg, sizeof(struct ColorRegister)) != sizeof(struct ColorRegister))
                        break;
#if (AROS_BIG_ENDIAN == 0)
                    paletteprefs.pap_Colors[i].Red   = AROS_BE2WORD(colreg.red);
                    paletteprefs.pap_Colors[i].Green = AROS_BE2WORD(colreg.green);
                    paletteprefs.pap_Colors[i].Blue  = AROS_BE2WORD(colreg.blue);
#else
                    paletteprefs.pap_Colors[i].Red   = (colreg.red << 24) | (colreg.red << 16) | (colreg.red << 8) | colreg.red;
                    paletteprefs.pap_Colors[i].Green = (colreg.green << 24) | (colreg.green << 16) | (colreg.green << 8) | colreg.green;
                    paletteprefs.pap_Colors[i].Blue  = (colreg.blue << 24) | (colreg.blue << 16) | (colreg.blue << 8) | colreg.blue;
#endif
                }
            }

            CloseIFF(iff);
        } /* if (!OpenIFF(iff, IFFF_READ)) */
        FreeIFF(iff);
    } /* if ((iff = AllocIFF())) */

    return retval;
}

/*********************************************************************************************/

BOOL Prefs_ExportFH(BPTR fh)
{
    struct PalettePrefs saveprefs;
    struct IFFHandle   *iff;
    BOOL                retval = FALSE;
#if defined(DELETEONERROR)
    BOOL                delete_if_error = FALSE;
#endif
    LONG                i;

#if (AROS_BIG_ENDIAN == 0)
    CopyMem(paletteprefs.pap_Reserved, saveprefs.pap_Reserved, sizeof(paletteprefs.pap_Reserved));
    for (i = 0; i < 32; i++)
    {
        saveprefs.pap_4ColorPens[i] = AROS_WORD2BE(paletteprefs.pap_4ColorPens[i]);
        if ((i >= MAXPENS) || (paletteprefs.pap_8ColorPens[i] < 4))
            saveprefs.pap_8ColorPens[i] = AROS_WORD2BE(paletteprefs.pap_8ColorPens[i]);
        else
            saveprefs.pap_8ColorPens[i] = AROS_WORD2BE(paletteprefs.pap_8ColorPens[i] + (PEN_C3 - 4));
        saveprefs.pap_Colors[i].ColorIndex = AROS_WORD2BE(paletteprefs.pap_Colors[i].ColorIndex);
        saveprefs.pap_Colors[i].Red = AROS_WORD2BE(paletteprefs.pap_Colors[i].Red);
        saveprefs.pap_Colors[i].Green = AROS_WORD2BE(paletteprefs.pap_Colors[i].Green);
        saveprefs.pap_Colors[i].Blue = AROS_WORD2BE(paletteprefs.pap_Colors[i].Blue);
    }
#else
    CopyMem(paletteprefs, saveprefs, sizeof(paletteprefs));
    for (i = 0; i < MAXPENS; i++)
    {
        if (saveprefs.pap_8ColorPens[i] >= 4)
            saveprefs.pap_8ColorPens[i] += (PEN_C3 - 4);
    }
#endif

    D(bug("Prefs_ExportFH: fh: %lx\n", fh));

    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR) fh;
        D(bug("Prefs_ExportFH: stream opened.\n"));

#if defined(DELETEONERROR)
        delete_if_error = TRUE;
#endif

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

                        if (!PushChunk(iff, ID_PREF, ID_PALT, sizeof(struct PalettePrefs)))
                        {
                            D(bug("Prefs_ExportFH: PushChunk(LCLE) okay.\n"));

                            if (WriteChunkBytes(iff, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
                            {
                                D(bug("Prefs_ExportFH: WriteChunkBytes(PALT) okay.\n"));
                                D(bug("Prefs_ExportFH: Everything okay :-)\n"));

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

#if defined(DELETEONERROR)
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
    paletteprefs.pap_Reserved[0] = 0;
    paletteprefs.pap_Reserved[1] = 0;
    paletteprefs.pap_Reserved[2] = 0;
    paletteprefs.pap_Reserved[3] = 0;

    CopyMem(defaultpen, paletteprefs.pap_4ColorPens, sizeof paletteprefs.pap_4ColorPens);
    CopyMem(defaultpen, paletteprefs.pap_8ColorPens, sizeof paletteprefs.pap_8ColorPens);
    CopyMem(defaultcolor, paletteprefs.pap_Colors, sizeof paletteprefs.pap_Colors);

    return TRUE;
}
