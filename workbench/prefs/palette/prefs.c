/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
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

#if (NUMDRIPENS > 12)
STATIC CONST UWORD defaultpen8[NUMDRIPENS + 1] = { 1, 0, 1, 2, 1, 3, 1, 0, 2, 1, 2, 1 , 2 , 1, -1};
#else
STATIC CONST UWORD defaultpen8[NUMDRIPENS + 1] = { 1, 0, 1, 2, 1, 3, 1, 0, 2, 1, 2, 1, -1};
#endif

STATIC struct ColorSpec defaultcolor[MAXPENS * 4] =
{
    {0, 0xAAAA, 0xAAAA, 0xAAAA  },
    {1, 0x0000, 0x0000, 0x0000  },
    {2, 0xFFFF, 0xFFFF, 0xFFFF  },
    {3, 0x6666, 0x8888, 0xBBBB  },
    {4, 0xEEEE, 0x4444, 0x4444  },
    {5, 0x5555, 0xDDDD, 0x5555  },
    {6, 0x0000, 0x4444, 0xDDDD  },
    {7, 0xEEEE, 0x9999, 0x0000  },
    {65535                      }
};

/*********************************************************************************************/
struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
} __packed;

/*********************************************************************************************/

struct PalettePrefs paletteprefs;

/*********************************************************************************************/

static BOOL Prefs_Load(STRPTR from)
{
    BOOL retval = FALSE;

    D(bug("[Palette] %s('%s')\n", __func__, from));

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

    D(bug("[Palette] %s(0x%p)\n", __func__, fh));

    if ((iff = AllocIFF()))
    {
        D(bug("[Palette] %s:  iff allocated @ 0x%p\n", __func__, iff));

        iff->iff_Stream = (IPTR)fh;

        InitIFFasDOS(iff);

        if (!OpenIFF(iff, IFFF_READ))
        {
            D(bug("[Palette] %s:  iff opened for READ\n", __func__));

            if (!StopChunk(iff, ID_PREF, ID_PALT))
            {
                struct PalettePrefs loadprefs;
                D(bug("[Palette] %s:  ID_PREF:ID_PALT\n", __func__));

                if (!ParseIFF(iff, IFFPARSE_SCAN))
                {
                    struct ContextNode *cn;

                    D(bug("[Palette] %s:  ParseIFF okay.\n", __func__));

                    cn = CurrentChunk(iff);

                    if (cn->cn_Size == sizeof(struct PalettePrefs))
                    {
                        D(bug("[Palette] %s:  ID_PALT chunk size okay.\n", __func__));

                        if (ReadChunkBytes(iff, &loadprefs, sizeof(struct PalettePrefs)) == sizeof(struct PalettePrefs))
                        {
                            D(bug("[Palette] %s:  Reading chunk successful.\n", __func__));

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
                            CopyMem(&loadprefs, &paletteprefs, sizeof(paletteprefs));
#endif
                            for (i = 0; i < MAXPENS; i++)
                            {
                                paletteprefs.pap_4ColorPens[i] = AROS_BE2WORD(loadprefs.pap_4ColorPens[i]);
                                paletteprefs.pap_8ColorPens[i] = AROS_BE2WORD(loadprefs.pap_8ColorPens[i]);
                                D(
                                    bug("[Palette] %s: #%02d    4c = %04x, 8c = %04x\n", __func__, i, paletteprefs.pap_4ColorPens[i], paletteprefs.pap_8ColorPens[i]);
                                    bug("[Palette] %s:    :%02d r:%04x g:%04x b:%04x\n", __func__, paletteprefs.pap_Colors[i].ColorIndex, paletteprefs.pap_Colors[i].Red, paletteprefs.pap_Colors[i].Green, paletteprefs.pap_Colors[i].Blue);
                                )
                                if (paletteprefs.pap_8ColorPens[i] >= PEN_C3)
                                    paletteprefs.pap_8ColorPens[i] -= (PEN_C3 - 4);
                            }
                            D(bug("[Palette] %s:  ID_PALT chunk parsed.\n", __func__));

                            retval = TRUE;
                        }
                    }
                } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
            }
            else if (!StopChunk(iff, ID_ILBM, ID_CMAP))
            {
                struct ColorRegister colreg;
                D(bug("[Palette] %s:  ID_ILBM:ID_CMAP\n", __func__));

                for (i = 0; i < MAXPENS; i++)
                {
                    if (ReadChunkBytes(iff, &colreg, sizeof(struct ColorRegister)) != sizeof(struct ColorRegister))
                        break;
#if (0)
                    paletteprefs.pap_Colors[i].ColorIndex   = i;
#endif
#if (AROS_BIG_ENDIAN == 0)
                    paletteprefs.pap_Colors[i].Red   = AROS_BE2WORD(colreg.red);
                    paletteprefs.pap_Colors[i].Green = AROS_BE2WORD(colreg.green);
                    paletteprefs.pap_Colors[i].Blue  = AROS_BE2WORD(colreg.blue);
#else
                    paletteprefs.pap_Colors[i].Red   = (colreg.red << 24) | (colreg.red << 16) | (colreg.red << 8) | colreg.red;
                    paletteprefs.pap_Colors[i].Green = (colreg.green << 24) | (colreg.green << 16) | (colreg.green << 8) | colreg.green;
                    paletteprefs.pap_Colors[i].Blue  = (colreg.blue << 24) | (colreg.blue << 16) | (colreg.blue << 8) | colreg.blue;
#endif
                    D(bug("[Palette] %s: #%02d r:%04x g:%04x b:%04x\n", __func__, i, paletteprefs.pap_Colors[i].Red, paletteprefs.pap_Colors[i].Green, paletteprefs.pap_Colors[i].Blue);)
                }
                D(bug("[Palette] %s:  ID_CMAP chunk parsed.\n", __func__));
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

    D(bug("[Palette] %s(0x%p)\n", __func__, fh));

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
    CopyMem(&paletteprefs, &saveprefs, sizeof(paletteprefs));
    for (i = 0; i < MAXPENS; i++)
    {
        if (saveprefs.pap_8ColorPens[i] >= 4)
            saveprefs.pap_8ColorPens[i] += (PEN_C3 - 4);
    }
#endif


    if ((iff = AllocIFF()))
    {
        D(bug("[Palette] %s:  iff allocated @ 0x%p\n", __func__, iff));

        iff->iff_Stream = (IPTR) fh;

#if defined(DELETEONERROR)
        delete_if_error = TRUE;
#endif

        InitIFFasDOS(iff);

        if (!OpenIFF(iff, IFFF_WRITE))
        {
            D(bug("[Palette] %s:  iff opened for WRITE\n", __func__));

            if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN))
            {
                D(bug("[Palette] %s: pushed chunk ID_PREF:ID_FORM\n", __func__));

                if (!PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct FilePrefHeader)))
                {
                    struct FilePrefHeader head;

                    D(bug("[Palette] %s: pushed chunk ID_PREF:ID_PRHD\n", __func__));

                    head.ph_Version  = PHV_CURRENT;
                    head.ph_Type     = 0;
                    head.ph_Flags[0] =
                    head.ph_Flags[1] =
                    head.ph_Flags[2] =
                    head.ph_Flags[3] = 0;

                    if (WriteChunkBytes(iff, &head, sizeof(head)) == sizeof(head))
                    {
                        D(bug("[Palette] %s: wrote chunk bytes for ID_PREF:ID_PRHD\n", __func__));

                        PopChunk(iff);

                        if (!PushChunk(iff, ID_PREF, ID_PALT, sizeof(struct PalettePrefs)))
                        {
                            D(bug("[Palette] %s: pushed chunk ID_PREF:ID_PALT\n", __func__));

                            if (WriteChunkBytes(iff, &saveprefs, sizeof(saveprefs)) == sizeof(saveprefs))
                            {
                                D(bug("[Palette] %s: wrote chunk bytes for ID_PREF:ID_PALT\n", __func__));

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
    UWORD pen = 0;
    int i;

    paletteprefs.pap_Reserved[0] = 0;
    paletteprefs.pap_Reserved[1] = 0;
    paletteprefs.pap_Reserved[2] = 0;
    paletteprefs.pap_Reserved[3] = 0;

    for (i = 0; i < (sizeof(paletteprefs.pap_4ColorPens) >> 1); i++)
    {
        if (i >= NUMDRIPENS)
            pen = (UWORD)-1;
        if (pen != (UWORD)-1)
            pen = defaultpen8[i];
        paletteprefs.pap_4ColorPens[i] = pen;
        paletteprefs.pap_8ColorPens[i] = pen;
    }
    CopyMem(defaultcolor, paletteprefs.pap_Colors, sizeof paletteprefs.pap_Colors);

    return TRUE;
}
