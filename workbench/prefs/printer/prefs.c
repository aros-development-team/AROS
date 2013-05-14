/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <aros/macros.h>

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

#define PREFS_PATH_ENVARC "ENVARC:SYS/printer.prefs"
#define PREFS_PATH_ENV    "ENV:SYS/printer.prefs"

/*********************************************************************************************/

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

/*********************************************************************************************/

struct PrinterPrefs printerprefs;

/*********************************************************************************************/

/* DEFAULTs */

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

#define IMPORT_WORD(x)  do { x = AROS_BE2WORD(x); } while (0)
#define IMPORT_LONG(x)  do { x = AROS_BE2LONG(x); } while (0)

BOOL Prefs_ImportFH(BPTR fh)
{
    struct PrinterTxtPrefs txt = printerprefs.pp_Txt;
    struct PrinterUnitPrefs unit = printerprefs.pp_Unit;
    struct PrinterDeviceUnitPrefs devunit = printerprefs.pp_DeviceUnit;
    struct PrinterGfxPrefs gfx = printerprefs.pp_Gfx;

    struct IFFHandle   *iff;
    LONG chunk_map = 0;
    LONG stop_chunks[] = {
        ID_PREF, ID_PTXT,
        ID_PREF, ID_PUNT,
        ID_PREF, ID_PDEV,
        ID_PREF, ID_PGFX,
    };


    D(bug("LoadPrefs: Begin\n"));

    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR)fh;

        InitIFFasDOS(iff);

        if (!OpenIFF(iff, IFFF_READ))
        {
            D(bug("LoadPrefs: OpenIFF okay.\n"));

            if (!StopChunks(iff, stop_chunks, 4))
            {
                D(bug("LoadPrefs: StopChunks okay.\n"));

                while (ParseIFF(iff, IFFPARSE_SCAN) == 0)
                {
                    struct ContextNode *cn;

                    cn = CurrentChunk(iff);

                    D(bug("LoadPrefs: ParseIFF okay: 0x%04x 0x%04x\n", cn->cn_ID, cn->cn_Type));

                    if (cn->cn_ID == ID_PTXT && cn->cn_Size == sizeof(txt))
                    {
                        D(bug("LoadPrefs: ID_PTXT chunk size okay.\n"));
                        if (ReadChunkBytes(iff, &txt, sizeof(txt)) == sizeof(txt))
                        {
                            D(bug("LoadPrefs: Reading chunk successful.\n"));

                            chunk_map |= (1 << 0);
                        }
                    }
                    if (cn->cn_ID == ID_PUNT && cn->cn_Size == sizeof(unit))
                    {
                        D(bug("LoadPrefs: ID_PUNT chunk size okay.\n"));
                        if (ReadChunkBytes(iff, &unit, sizeof(unit)) == sizeof(unit))
                        {
                            D(bug("LoadPrefs: Reading chunk successful.\n"));

                            chunk_map |= (1 << 1);
                        }
                    }
                    if (cn->cn_ID == ID_PDEV && cn->cn_Size == sizeof(devunit))
                    {
                        D(bug("LoadPrefs: ID_PDEV chunk size okay.\n"));
                        if (ReadChunkBytes(iff, &devunit, sizeof(devunit)) == sizeof(devunit))
                        {
                            D(bug("LoadPrefs: Reading chunk successful.\n"));

                            chunk_map |= (1 << 2);
                        }
                    }
                    if (cn->cn_ID == ID_PGFX && cn->cn_Size == sizeof(gfx))
                    {
                        D(bug("LoadPrefs: ID_PGFX chunk size okay.\n"));
                        if (ReadChunkBytes(iff, &gfx, sizeof(gfx)) == sizeof(gfx))
                        {
                            D(bug("LoadPrefs: Reading chunk successful.\n"));

                            chunk_map |= (1 << 3);
                        }
                    }
                }
            }
            CloseIFF(iff);
        }
        FreeIFF(iff);
    }

    if (chunk_map & (1 << 0)) {
        D(bug("LoadPrefs: PTXT\n"));

        IMPORT_WORD(txt.pt_PaperType); 
        IMPORT_WORD(txt.pt_PaperSize);
        IMPORT_WORD(txt.pt_PaperLength);
        IMPORT_WORD(txt.pt_Pitch);
        IMPORT_WORD(txt.pt_Spacing);
        IMPORT_WORD(txt.pt_LeftMargin);
        IMPORT_WORD(txt.pt_RightMargin);
        IMPORT_WORD(txt.pt_Quality);
        printerprefs.pp_Txt = txt;
    }

    if (chunk_map & (1 << 1)) {
        D(bug("LoadPrefs: PUNT\n"));

        IMPORT_LONG(unit.pu_UnitNum);
        IMPORT_LONG(unit.pu_OpenDeviceFlags);
        printerprefs.pp_Unit = unit;
    }

    if (chunk_map & (1 << 2)) {
        D(bug("LoadPrefs: PDEV\n"));

        IMPORT_LONG(devunit.pd_UnitNum);
        printerprefs.pp_DeviceUnit = devunit;
    }

    if (chunk_map & (1 << 3)) {
        D(bug("LoadPrefs: PGFX\n"));

        IMPORT_WORD(gfx.pg_Aspect);
        IMPORT_WORD(gfx.pg_Shade);
        IMPORT_WORD(gfx.pg_Image);
        IMPORT_WORD(gfx.pg_Threshold);
        IMPORT_WORD(gfx.pg_GraphicFlags);
        IMPORT_WORD(gfx.pg_PrintMaxWidth);
        IMPORT_WORD(gfx.pg_PrintMaxHeight);

        printerprefs.pp_Gfx = gfx;
    }

    D(bug("LoadPrefs: Done\n"));
    return (chunk_map & (1 << 0)) ? TRUE : FALSE;
}

/*********************************************************************************************/

#define EXPORT_WORD(x)  do { x = AROS_WORD2BE(x); } while (0)
#define EXPORT_LONG(x)  do { x = AROS_LONG2BE(x); } while (0)

BOOL Prefs_ExportFH(BPTR fh)
{
    struct IFFHandle   *iff;
    BOOL                retval = FALSE;

    struct PrinterTxtPrefs txt = printerprefs.pp_Txt;
    struct PrinterUnitPrefs unit = printerprefs.pp_Unit;
    struct PrinterDeviceUnitPrefs devunit = printerprefs.pp_DeviceUnit;
    struct PrinterGfxPrefs gfx = printerprefs.pp_Gfx;

    EXPORT_WORD(txt.pt_PaperType); 
    EXPORT_WORD(txt.pt_PaperSize);
    EXPORT_WORD(txt.pt_PaperLength);
    EXPORT_WORD(txt.pt_Pitch);
    EXPORT_WORD(txt.pt_Spacing);
    EXPORT_WORD(txt.pt_LeftMargin);
    EXPORT_WORD(txt.pt_RightMargin);
    EXPORT_WORD(txt.pt_Quality);

    EXPORT_LONG(unit.pu_UnitNum);
    EXPORT_LONG(unit.pu_OpenDeviceFlags);

    EXPORT_LONG(devunit.pd_UnitNum);

    EXPORT_WORD(gfx.pg_Aspect);
    EXPORT_WORD(gfx.pg_Shade);
    EXPORT_WORD(gfx.pg_Image);
    EXPORT_WORD(gfx.pg_Threshold);
    EXPORT_WORD(gfx.pg_GraphicFlags);
    EXPORT_WORD(gfx.pg_PrintMaxWidth);
    EXPORT_WORD(gfx.pg_PrintMaxHeight);

    D(bug("SavePrefsFH: fh: %lx\n", fh));

    if ((iff = AllocIFF()))
    {
        iff->iff_Stream = (IPTR) fh;
        D(bug("SavePrefsFH: stream opened.\n"));

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
#if !DEBUG && defined(__mc68000__)
                        /* jmcmullan: AGH! Terrible workaround here.
                         * It appears that, with arch/m68k-all/include/gencall.c,
                         * revision 45499, that gcc clobbers the return value of
                         * WriteChunkBytes() with the call to PopChunk()
                         * before comparing it.
                         *
                         * Obviously, gencall.c (or gcc) needs to be fixed,
                         * but this evil workaround will have to suffice
                         * for now.
                         *
                         * FIXME FIXME FIXME
                         */
                        kprintf("");
#endif

                        PopChunk(iff);

                        if (!PushChunk(iff, ID_PREF, ID_PTXT, sizeof(struct PrinterTxtPrefs)) &&
                            WriteChunkBytes(iff, &txt, sizeof(txt)) == sizeof(txt) &&
                            !PopChunk(iff) &&
                            !PushChunk(iff, ID_PREF, ID_PUNT, sizeof(struct PrinterUnitPrefs)) &&
                            WriteChunkBytes(iff, &unit, sizeof(unit)) == sizeof(unit) &&
                            !PopChunk(iff) &&
                            !PushChunk(iff, ID_PREF, ID_PDEV, sizeof(struct PrinterDeviceUnitPrefs)) &&
                            WriteChunkBytes(iff, &devunit, sizeof(devunit)) == sizeof(devunit) &&
                            !PopChunk(iff) &&
                            !PushChunk(iff, ID_PREF, ID_PGFX, sizeof(struct PrinterGfxPrefs)) &&
                            WriteChunkBytes(iff, &gfx, sizeof(gfx)) == sizeof(gfx) &&
                            !PopChunk(iff))
                        {
                                retval = TRUE;
                        }
                    }
                    else
                    {
                        PopChunk(iff);
                    }
                }
                PopChunk(iff);
            }
            CloseIFF(iff);
        }
        FreeIFF(iff);
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
                Prefs_Default(0);
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

BOOL Prefs_Default(int PrinterUnit)
{
    struct PrinterTxtPrefs *txt = &printerprefs.pp_Txt;
    struct PrinterUnitPrefs *unit = &printerprefs.pp_Unit;
    struct PrinterDeviceUnitPrefs *devunit = &printerprefs.pp_DeviceUnit;
    struct PrinterGfxPrefs *gfx = &printerprefs.pp_Gfx;

    memset(&printerprefs, 0, sizeof(printerprefs));

    D(bug("Prefs_Default: Unit %d\n", PrinterUnit));

    strcpy(txt->pt_Driver, "PostScript");
    txt->pt_Port = PP_PARALLEL;   /* Actually, unused */
    txt->pt_PaperType = PT_SINGLE;
    txt->pt_PaperSize = PS_US_LETTER;
    txt->pt_PaperLength = 65;
    txt->pt_Pitch = PP_PICA;
    txt->pt_Spacing = PS_SIX_LPI;
    txt->pt_LeftMargin = 5;
    txt->pt_RightMargin = 75;
    txt->pt_Quality = PQ_LETTER;

    unit->pu_UnitNum = PrinterUnit; /* printtofile.device unit == printer.device unit */
    unit->pu_OpenDeviceFlags = 0;
    strcpy(unit->pu_DeviceName, "printtofile");

    devunit->pd_UnitNum = PrinterUnit;
    snprintf(devunit->pd_UnitName, sizeof(devunit->pd_UnitName), "Unit %d", PrinterUnit);

    gfx->pg_Aspect = PA_VERTICAL;
    gfx->pg_Shade = PS_COLOR;
    gfx->pg_Image = PI_POSITIVE;
    gfx->pg_Threshold = 256;
    gfx->pg_ColorCorrect = 0;   /* No color correction enabled */
    gfx->pg_Dimensions = PD_IGNORE;
    gfx->pg_GraphicFlags = PGFB_CENTER_IMAGE;
    gfx->pg_PrintDensity = PD_FLOYD;
    gfx->pg_PrintMaxWidth = 0;       /* Will be set by driver */
    gfx->pg_PrintMaxHeight = 0;      /* Will be set by driver */
    gfx->pg_PrintXOffset = 0;
    gfx->pg_PrintYOffset = 0;

    return TRUE;
}

