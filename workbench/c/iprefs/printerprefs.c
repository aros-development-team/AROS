/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

/*********************************************************************************************/

#include "global.h"

#include <aros/debug.h>

#include <aros/macros.h>
#include <datatypes/pictureclass.h>
#include <prefs/prefhdr.h>
#include <prefs/printertxt.h>
#include <prefs/printergfx.h>

#include <intuition/iprefs.h>
#include <proto/datatypes.h>

#include <string.h>

/*********************************************************************************************/

static LONG stopchunks[] =
{
    ID_PREF, ID_PTXT,
    ID_PREF, ID_PUNT,
    ID_PREF, ID_PGFX
};

void PrinterPrefs_Handler(STRPTR filename)
{
    struct IFFHandle *iff;
    struct PrinterTxtPrefs *ptp;
    struct PrinterUnitPrefs *pup;
    struct PrinterGfxPrefs *pgp;
    UBYTE devname[DEVICENAMESIZE] = "";
    UBYTE port = 0xff;
    CONST_STRPTR name;
    ULONG len;

    D(bug("[PrinterPrefs] filename=%s\n", filename));

    iff = CreateIFF(filename, stopchunks, 3);
    if (iff)
    {
        struct Preferences prefs;
        GetPrefs(&prefs, sizeof(prefs));

        while(ParseIFF(iff, IFFPARSE_SCAN) == 0)
        {
            struct ContextNode *cn = CurrentChunk(iff);
            switch(cn->cn_ID)
            {
                case ID_PTXT:
                    ptp = LoadChunk(iff, sizeof(struct PrinterTxtPrefs), MEMF_ANY);
                    if (ptp)
                    {
                        CopyMem(ptp->pt_Driver, prefs.PrinterFilename, sizeof(prefs.PrinterFilename) - 1);
                        prefs.PrinterFilename[sizeof(prefs.PrinterFilename) - 1] = 0;
                        port = ptp->pt_Port;
                        prefs.PrinterPort = ptp->pt_Port;
                        prefs.PaperType = AROS_BE2WORD(ptp->pt_PaperType);
                        prefs.PaperSize = AROS_BE2WORD(ptp->pt_PaperSize);
                        prefs.PaperLength = AROS_BE2WORD(ptp->pt_PaperLength);
                        prefs.PrintPitch = AROS_BE2WORD(ptp->pt_Pitch);
                        prefs.PrintSpacing = AROS_BE2WORD(ptp->pt_Spacing);
                        prefs.PrintLeftMargin = AROS_BE2WORD(ptp->pt_LeftMargin);
                        prefs.PrintRightMargin = AROS_BE2WORD(ptp->pt_RightMargin);
                        prefs.PrintQuality = AROS_BE2WORD(ptp->pt_Quality);

                        FreeVec(ptp);
                    }
                    break;
                case ID_PUNT:
                    pup = LoadChunk(iff, sizeof(struct PrinterUnitPrefs), MEMF_ANY);
                    if (pup)
                    {
                        CopyMem(pup->pu_DeviceName, devname, sizeof(devname) - 1);
                        prefs.DefaultPrtUnit = AROS_BE2LONG(pup->pu_UnitNum);
                        FreeVec(pup);
                    }
                    break;
                case ID_PGFX:
                    pgp = LoadChunk(iff, sizeof(struct PrinterGfxPrefs), MEMF_ANY);
                    if (pgp)
                    {
                        prefs.PrintAspect = AROS_BE2WORD(pgp->pg_Aspect);
                        prefs.PrintShade = AROS_BE2WORD(pgp->pg_Shade);
                        prefs.PrintImage = AROS_BE2WORD(pgp->pg_Image);
                        prefs.PrintThreshold = AROS_BE2WORD(pgp->pg_Threshold);
                        //prefs.ColorCorrect = pgp->pg_ColorCorrect;
                        //prefs.Dimensions = pgp->pg_Dimensions;
                        //prefs.Dithering = pgp->pg_Dithering;
                        //prefs.GraphicFlags = AROS_BE2WORD(pgp->pg_GraphicFlags);
                        prefs.PrintDensity = pgp->pg_PrintDensity;
                        prefs.PrintMaxWidth = AROS_BE2WORD(pgp->pg_PrintMaxWidth);
                        prefs.PrintMaxHeight = AROS_BE2WORD(pgp->pg_PrintMaxHeight);
                        prefs.PrintXOffset = pgp->pg_PrintXOffset;
                        //prefs.PrintYOffset = pgp->pg_PrintYOffset;

                        FreeVec(pgp);
                    }
                    break;
            }
        }
        KillIFF(iff);

        /* PrtDevName is the device printer.device opens, without ".device".
           The preferences give its full name, or none for the port's own;
           older AROS preferences left out ".device". */
        if (devname[0] != 0)
            name = devname;
        else if (port == PP_PARALLEL)
            name = "parallel";
        else if (port == PP_SERIAL)
            name = "serial";
        else if (port != 0xff)
            name = "printtofile";
        else
            name = NULL;
        if (name != NULL)
        {
            len = strlen(name);
            if (len >= 7 && strcmp(&name[len - 7], ".device") == 0)
                len -= 7;
            if (len < sizeof(prefs.PrtDevName))
            {
                CopyMem(name, prefs.PrtDevName, len);
                prefs.PrtDevName[len] = 0;
            }
            else
                D(bug("[IPREFS] printer device name (%s) does not fit PrtDevName\n", name));
        }

        SetPrefs(&prefs, sizeof(prefs), TRUE);
    }
}
