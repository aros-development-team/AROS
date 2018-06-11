/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#define DEBUG 0
#include <aros/debug.h>
#include <aros/macros.h>
#include <datatypes/pictureclass.h>
#include <prefs/prefhdr.h>
#include <prefs/printertxt.h>
#include <prefs/printergfx.h>

#include <intuition/iprefs.h>
#include <proto/datatypes.h>

#define DEV_EXT_LEN 7

/*********************************************************************************************/

static LONG stopchunks[] =
{
    ID_PREF, ID_PTXT,
    ID_PREF, ID_PGFX
};

void PrinterPrefs_Handler(STRPTR filename)
{
    struct IFFHandle *iff;
    struct PrinterTxtPrefs *ptp;
    struct PrinterGfxPrefs *pgp;
    char *devname;
    ULONG devnamelen;
    BOOL name_too_long = FALSE;

    D(bug("[PrinterPrefs] filename=%s\n", filename));

    iff = CreateIFF(filename, stopchunks, 2);
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
                        devname = FilePart(ptp->pt_Driver);
                        devnamelen = strlen(devname);
                        if (devnamelen < DEVNAME_SIZE + DEV_EXT_LEN)
                        {
                            if (devnamelen >= DEV_EXT_LEN && !strcmp(
                                &devname[devnamelen - DEV_EXT_LEN], ".device"))
                            {
                                strncpy(prefs.PrtDevName, devname,
                                    devnamelen - DEV_EXT_LEN);
                            }
                            else if (devnamelen < DEVNAME_SIZE)
                                strncpy(prefs.PrtDevName, devname, devnamelen);
                            else
                                name_too_long = TRUE;
                        }
                        else
                            name_too_long = TRUE;

                        if (name_too_long)
                            bug("[IPREFS] ERROR: printer device name (%s) "
                                "is too long!", devname);

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
        SetPrefs(&prefs, sizeof(prefs), TRUE);
    }
}
