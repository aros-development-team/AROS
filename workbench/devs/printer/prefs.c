/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
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

#include "printer_intern.h"

#ifdef BIGENDIAN_PREFS
#define GET_WORD AROS_BE2WORD
#define GET_LONG AROS_BE2LONG
#else
#define GET_WORD(x) x
#define GET_LONG(x) x
#endif

/*********************************************************************************************/

#define PREFS_PATH_ENVARC "ENVARC:SYS/printer"
#define PREFS_PATH_ENV    "ENV:SYS/printer"

/*********************************************************************************************/

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

/*********************************************************************************************/

#define IMPORT_WORD(x)  do { x = AROS_BE2WORD(x); } while (0)
#define IMPORT_LONG(x)  do { x = AROS_BE2LONG(x); } while (0)

#define PATHMAX 63

BOOL Printer_LoadPrefs(struct PrinterBase *PrinterBase, LONG unitnum, struct PrinterPrefs *prefs)
{
    struct PrinterTxtPrefs txt = prefs->pp_Txt;
    struct PrinterUnitPrefs unit = prefs->pp_Unit;
    struct PrinterDeviceUnitPrefs devunit = prefs->pp_DeviceUnit;
    struct PrinterGfxPrefs gfx = prefs->pp_Gfx;
    BPTR fh;
    TEXT envpath[PATHMAX + 1];
    TEXT envarcpath[PATHMAX + 1];

    struct IFFHandle   *iff;
    LONG chunk_map = 0;
    LONG stop_chunks[] = {
        ID_PREF, ID_PTXT,
        ID_PREF, ID_PUNT,
        ID_PREF, ID_PDEV,
        ID_PREF, ID_PGFX,
    };

    AddPart(envpath, PREFS_PATH_ENV, PATHMAX);
    AddPart(envarcpath, PREFS_PATH_ENVARC, PATHMAX);
    if (unitnum) {
        TEXT c[2] = { '0' + unitnum, 0 };
        strncat(envpath, c, PATHMAX);
        strncat(envarcpath, c, PATHMAX);
    }
    strncat(envpath, ".prefs", PATHMAX);
    strncat(envarcpath, ".prefs", PATHMAX);
    envpath[PATHMAX] = '\0';
    envarcpath[PATHMAX] = '\0';

    D(bug("%s: envpath \"%s\"\n", __func__, envpath));
    D(bug("%s: envarcpath \"%s\"\n", __func__, envpath));

    if (((fh = Open(envpath, MODE_OLDFILE)) == BNULL) &&
        ((fh = Open(envarcpath, MODE_OLDFILE)) == BNULL))
        return FALSE;

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
    Close(fh);

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
        prefs->pp_Txt = txt;
    }

    if (chunk_map & (1 << 1)) {
        D(bug("LoadPrefs: PUNT\n"));

        IMPORT_LONG(unit.pu_UnitNum);
        IMPORT_LONG(unit.pu_OpenDeviceFlags);
        prefs->pp_Unit = unit;
    }

    if (chunk_map & (1 << 2)) {
        D(bug("LoadPrefs: PDEV\n"));

        IMPORT_LONG(devunit.pd_UnitNum);
        prefs->pp_DeviceUnit = devunit;
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

        prefs->pp_Gfx = gfx;
    }

    D(bug("LoadPrefs: Done\n"));
    return (chunk_map & (1 << 0)) ? TRUE : FALSE;
}
