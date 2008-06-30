/*
 *  Format -- disk formatting and file-system creation program
 *  Copyright (C) 1999 Ben Hutchings <womble@zzumbouk.demon.co.uk>
 *  Copyright (C) 2008 Pavel Fedin <sonic_amiga@rambler.ru>

 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *  Layout of the main window of version 40 in English is:
 *
 *  +-------------------------------------------------------------+
 *  | Format - <device>                                           |
 *  +-------------------------------------------------------------+
 *  |    Current Information: Device '<device>'                   |
 *  |                         Volume '<volume>'                   |
 *  |                         <size> capacity <usage>% used       |
 *  |                          _________________________________  |
 *  |        New Volume Name: |Empty____________________________| |
 *  |                          __                                 |
 *  |           Put Trashcan: |./|                                |
 *  |                          __                                 |
 *  |       Fast File System: |__|                                |
 *  |                          __                                 |
 *  |     International Mode: |__|                                |
 *  |                          __                                 |
 *  |        Directory Cache: |__|                                |
 *  |                                                             |
 *  | +-----------------+ +-----------------+ +-----------------+ |
 *  | |      Format     | |   Quick Format  | |      Cancel     | |
 *  | +-----------------+ +-----------------+ +-----------------+ |
 *  +-------------------------------------------------------------+
 *
 *  For ID_NOT_REALLY_DOS and ID_UNREADABLE_DISK volumes, the information
 *  takes the format:
 *      Device '<device>'
 *      <size> capacity
 *
 *  For non-native file-systems, the lower three checkboxes are not shown.
 *
 *  <size> is the whole number of kilobytes (units of 2^10 bytes)
 *  followed by `K', if this number is less than or equal to 9999;
 *  otherwise the whole number of megabytes (units of 2^20 bytes).
 *
 *  The progress window has this layout:
 *
 *  +-----------------------------------------------------------------------+
 *  | Format - <device>                                                     |
 *  +-----------------------------------------------------------------------+
 *  |                                                                       |
 *  |                           Formatting disk...                          |
 *  | +-------------------------------------------------------------------+ |
 *  | |                                                                   | |
 *  | +----------------+----------------+----------------+----------------+ |
 *  | 0%                               50%                             100% |
 *  |                                                                       |
 *  |                         +--------------------+                        |
 *  |                         |        Stop        |                        |
 *  |                         +--------------------+                        |
 *  +-----------------------------------------------------------------------+
 *
 *  The contents of this window are replaced by the text `Initializing
 *  disk...' for the file-system creation and trashcan creation stages.
 */


#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <stdarg.h>
#include <workbench/startup.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <string.h>

#include "format.h"


static struct Gadget * pgadCreate(
    struct Gadget * pgadPrevious,
    ULONG gid, ULONG kind, WORD xLeft, WORD yTop, WORD cxWidth, WORD cyHeight,
    const char * psz, ULONG flags, ... );


enum {
    gidCancel = 1,
    gidFormat,
    gidQuickFormat
};


static struct Window * pwin;
static APTR vi;
static UWORD cxScale, cyScale, xOrigin, yOrigin;


int rcGuiMain(void)
{
    static char szTitle[6+3+30+1];
    struct Screen * pscr = 0;
    struct DrawInfo * pdri = 0;
    struct TextFont * ptf = 0;
    struct FileLock * pflVolume = 0;
    struct DosList * pdlVolume = 0;
    struct Gadget * pgadList = 0;
    BOOL bDoFormat, bMakeTrashcan, bFFS, bIntl;
    BOOL bDirCache = FALSE;
    static struct InfoData dinf __attribute__((aligned (4)));
    char szCapacityInfo[5+1+8+2+2+2+4+1];
    UBYTE ipenText;
    LONG rc = RETURN_FAIL;
    struct IntuiText init_it = {
	0, 0, JAM1,
	0, 0,
	NULL, "Initializing disk...", 0
    };

#ifdef DEBUG
    BPTR bpfhStdErr =
	Open("CON:0/50/640/100/Format Debug Output/CLOSE/WAIT",MODE_READWRITE);
    SelectInput(bpfhStdErr);
    SelectOutput(bpfhStdErr);
#endif

    if( _WBenchMsg->sm_NumArgs > 1 )
    {
	struct DosList *pdlList;

	if( _WBenchMsg->sm_ArgList[1].wa_Lock == 0 )
	{
	    /* it's a device */
	    if( !bSetSzDosDeviceFromSz(_WBenchMsg->sm_ArgList[1].wa_Name) ) {
		D(bug("Bad device name wrom Workbench: %s\n", _WBenchMsg->sm_ArgList[1].wa_Name));
		/* Workbench is playing silly buggers */
		goto cleanup;
	    }
	}
	else if( _WBenchMsg->sm_ArgList[1].wa_Name[0] == 0 )
	{
	    struct DosList *pdlDevice;
	    /* it's a volume */

	    D(bug("Object specified by lock\n"));
	    /* make sure it's mounted before looking for its device */
	    if( !Info( _WBenchMsg->sm_ArgList[1].wa_Lock, &dinf ) )
	    {
		ReportErrSz( ertFailure, 0, 0 );
		goto cleanup;
	    }

	    pflVolume =
		(struct FileLock *)BADDR(_WBenchMsg->sm_ArgList[1].wa_Lock);
	    pdlVolume = (struct DosList *)BADDR(pflVolume->fl_Volume);
	    pdlList = LockDosList( LDF_DEVICES | LDF_READ );
	    pdlDevice = pdlList;
	    do
	    {
		if( (pdlDevice = NextDosEntry( pdlDevice,
					       LDF_DEVICES | LDF_READ )) == 0 )
		{
		    ReportErrSz( ertFailure, ERROR_DEVICE_NOT_MOUNTED, 0 );
		    goto cleanup;
		}
	    }
	    while( pdlDevice->dol_Task != pflVolume->fl_Task );

	    RawDoFmtSz( szDosDevice, "%b", pdlDevice->dol_Name );
	    pchDosDeviceColon = szDosDevice + strlen(szDosDevice);
	    *(pchDosDeviceColon+1) = 0;
	}
	else
	{
	    ReportErrSz( ertFailure, ERROR_OBJECT_WRONG_TYPE, 0 );
	    goto cleanup;
	}

	if(!bGetDosDevice(pdlList))
	    goto cleanup;
    }

    {
	/* The units of the size are initially bytes, but we shift the
	   number until the units are kilobytes, megabytes or
	   something larger and the number of units has at most 4
	   digits. */
	ULLONG cUnits;
	/* The unit symbols go up to exabytes, since the maximum
           possible disk size is 2^64 bytes = 16 exabytes. */
	const char * pchUnitSymbol = "KMGTPE";

	cUnits = ibyEnd - ibyStart;
	while( (cUnits >>= 10) > 9999 )
	    ++pchUnitSymbol;

	if(pflVolume)
	    RawDoFmtSz( szCapacityInfo, "%lu%lc capacity, %lu%% used",
			(ULONG)cUnits, (ULONG)*pchUnitSymbol,
			/* Calculate percentage used, to nearest point. */
			(ULONG)(((ULLONG)dinf.id_NumBlocksUsed*100ULL
				 + dinf.id_NumBlocks/2) / dinf.id_NumBlocks) );
	else
	    RawDoFmtSz( szCapacityInfo, "%lu%lc capacity",
			(ULONG)cUnits, (ULONG)*pchUnitSymbol );
    }

    if( (pscr = LockPubScreen(0)) == 0
	|| (vi = GetVisualInfo( pscr, TAG_END )) == 0
	|| (pdri = GetScreenDrawInfo(pscr)) == 0
	|| (ptf = OpenFont(pscr->Font)) == 0 )
    {
	/* TODO: report error */
	goto cleanup;
    }

    cxScale = ptf->tf_XSize;
    cyScale = ptf->tf_YSize;
    ipenText = pdri->dri_Pens[TEXTPEN];
    init_it.FrontPen = ipenText;
    init_it.ITextFont = pscr->Font;

    if( (pwin = OpenWindowTags(
	0,
	WA_InnerWidth, 424UL * cxScale >> 3,
	WA_InnerHeight, 141UL * cyScale >> 3,
	WA_Flags,
	WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_NOCAREREFRESH
	|WFLG_NEWLOOKMENUS|WFLG_SMART_REFRESH,
	WA_IDCMP,
	BUTTONIDCMP|CHECKBOXIDCMP|STRINGIDCMP,
	WA_Title, (ULONG)"Format",
	TAG_END )) == 0 )
    {
	/* TODO: report error */
	goto cleanup;
    }

    xOrigin = pwin->BorderLeft;
    yOrigin = pwin->BorderTop;

    if( _WBenchMsg->sm_NumArgs == 1 )
    {
	/* TODO: display list of devices and have user select one */
	goto cleanup;
    }

    RawDoFmtSz( szTitle, "Format - %s", szDosDevice );
    SetWindowTitles( pwin, szTitle, (UBYTE *)(-1) );

    {
	BOOL bEnd = FALSE;
	struct Gadget * pgad, * pgadName, * pgadTrashcan,
	    * pgadFFS, * pgadIntl;
#ifndef __AROS__
	struct Gadget * pgadDirCache;
#endif

	char szDeviceInfo[6+2+MAX_FS_NAME_LEN+2];
	char szVolumeInfo[6+2+MAX_FS_NAME_LEN+2];
	RawDoFmtSz( szDeviceInfo, "Device '%s'", szDosDevice );
	pgad = CreateContext(&pgadList);
	pgad = pgadCreate(
	    pgad, 0, TEXT_KIND, 192, 7, 224, 8,
	    "Current Information:", PLACETEXT_LEFT,
	    GTTX_Text, (ULONG)szDeviceInfo, TAG_END );
	if(pdlVolume)
	{
	    RawDoFmtSz( szVolumeInfo, "Volume '%b'", pdlVolume->dol_Name );
	    pgad = pgadCreate(
		pgad, 0, TEXT_KIND, 192, 16, 224, 8,
		"", PLACETEXT_LEFT,
		GTTX_Text, (ULONG)szVolumeInfo,	TAG_END );
	}
	pgad = pgadCreate(
	    pgad, 0, TEXT_KIND, 192, pdlVolume ? 25 : 16, 224, 8,
	    "", PLACETEXT_LEFT,
	    GTTX_Text, (ULONG)szCapacityInfo, TAG_END );
	pgadName = pgadCreate(
	    pgad, 0, STRING_KIND, 192, 44, 224, 14,
	    "New Volume Name:", PLACETEXT_LEFT,
	    GTST_String, (ULONG)"Empty",
	    GTST_MaxChars, (ULONG)MAX_FS_NAME_LEN,
	    TAG_END );
	pgad = pgadTrashcan = pgadCreate(
	    pgadName, 0, CHECKBOX_KIND, 192, 60, 26, 11,
	    "Put Trashcan:", PLACETEXT_LEFT,
	    GTCB_Checked, (ULONG)TRUE, TAG_END );
	if( DosType >= 0x444F5300
	    && DosType <= 0x444F5305 )
	{
	    pgadFFS = pgadCreate(
		pgadTrashcan, 0, CHECKBOX_KIND, 192, 74, 26, 11,
		"Fast File System:", PLACETEXT_LEFT,
		GTCB_Checked, DosType & 1UL, TAG_END );
	    pgadIntl = pgadCreate(
		pgadFFS, 0, CHECKBOX_KIND, 192, 88, 26, 11,
		"International Mode:", PLACETEXT_LEFT,
		GTCB_Checked, DosType & 6UL, TAG_END );
#ifndef __AROS__
	    pgad = pgadDirCache = pgadCreate(
		pgadIntl, 0, CHECKBOX_KIND, 192, 102, 26, 11,
		"Directory Cache:", PLACETEXT_LEFT,
		GTCB_Checked, DosType & 4UL, TAG_END );
#endif
	}
	pgad = pgadCreate(
	    pgad, gidFormat, BUTTON_KIND, 8, 124, 134, 14,
	    "Format", PLACETEXT_IN,
	    TAG_END );
	pgad = pgadCreate(
	    pgad, gidQuickFormat, BUTTON_KIND, 145, 124, 134, 14,
	    "Quick Format", PLACETEXT_IN,
	    TAG_END );
	pgad = pgadCreate(
	    pgad, gidCancel, BUTTON_KIND, 282, 124, 134, 14,
	    "Cancel", PLACETEXT_IN,
	    TAG_END );

	if( pgad == 0 )
	    goto cleanup;

	AddGList( pwin, pgadList, -1, -1, 0 );
	RefreshGList( pgadList, pwin, 0, -1 );
	GT_RefreshWindow( pwin, 0 );

	bDoFormat = TRUE;

	do
	{
	    struct IntuiMessage * pim;
	    BOOL bCancel = FALSE;

	    while( (pim = GT_GetIMsg(pwin->UserPort)) == 0 )
		WaitPort(pwin->UserPort);

	    if( pim->Class == IDCMP_GADGETUP )
	    {
		switch( ((struct Gadget *)(pim->IAddress))->GadgetID )
		{
		case gidQuickFormat:
		    bDoFormat = FALSE;
		    /* fall through */
		case gidFormat:
		    bMakeTrashcan = pgadTrashcan->Flags & GFLG_SELECTED;
		    bFFS = pgadFFS->Flags & GFLG_SELECTED;
		    bIntl = pgadIntl->Flags & GFLG_SELECTED;
#ifndef __AROS__
		    bDirCache = pgadDirCache->Flags & GFLG_SELECTED;
#endif
		    bEnd = TRUE;
		    break;
		case gidCancel:
		    bCancel = TRUE;
		    bEnd = TRUE;
		    break;
		}
	    }

	    GT_ReplyIMsg(pim);

	    if(bCancel)
		goto cleanup;
	}
	while(!bEnd);

	/* TODO: set volume name same as old one if name is null string */
	if( !bSetSzVolumeFromSz( (char *)((struct StringInfo *)
					  pgadName->SpecialInfo)->Buffer ) )
	{
	    rc = RETURN_ERROR;
	    goto cleanup;
	}

	RemoveGList( pwin, pgadList, -1 );
	FreeGadgets(pgadList);
	pgadList = 0;

	EraseRect( pwin->RPort, xOrigin, yOrigin,
		   pwin->Width - pwin->BorderRight - 1,
		   pwin->Height - pwin->BorderTop -1 );
    }

    ChangeWindowBox( pwin,
		     pwin->LeftEdge, pwin->TopEdge,
		     pwin->BorderLeft + pwin->BorderRight
		     + (424 * cxScale >> 3),
		     pwin->BorderTop + pwin->BorderBottom
		     + (88 * cyScale >> 3) );

    if(bDoFormat)
    {
	struct Gadget * pgad = CreateContext(&pgadList);
	pgad = pgadCreate(
	    pgad, 0, TEXT_KIND, 8, 16, 409, 8,
	    "Formatting Disk...", PLACETEXT_IN,
	    TAG_END );
	pgad = pgadCreate(
	    pgad, gidCancel, BUTTON_KIND, 145, 71, 134, 14,
	    "Stop", PLACETEXT_IN,
	    TAG_END );
	if( pgad == 0 )
	    goto cleanup;

	AddGList( pwin, pgadList, -1, -1, 0 );
	RefreshGList( pgadList, pwin, 0, -1 );
	GT_RefreshWindow( pwin, 0 );

	DrawBevelBox(
	    pwin->RPort,
	    xOrigin + (8 * cxScale >> 3), yOrigin + (28 * cyScale >> 3),
	    409 * cxScale >> 3, 12 * cyScale >> 3,
	    GTBB_Recessed, TRUE,
	    GTBB_FrameType, BBFT_BUTTON,
	    GT_VisualInfo, (ULONG)vi,
	    TAG_END );
	{
	    struct IntuiText ait[3] = { {
		ipenText, 0, JAM1,
		8 * cxScale >> 3, 0,
		pscr->Font, "0%", &ait[1]
	    }, {
		ipenText, 0, JAM1,
		0, 0,
		pscr->Font, "50%", &ait[2]
	    }, {
		ipenText, 0, JAM1,
		0, 0,
		pscr->Font, "100%", 0
	    } };
	    ait[1].LeftEdge = (212 * cxScale >> 3)
		- IntuiTextLength(&ait[1]) / 2;
	    ait[2].LeftEdge = (416 * cxScale >> 3)
		- IntuiTextLength(&ait[2]);
	    PrintIText( pwin->RPort, &ait[0],
			xOrigin,
			yOrigin + (46 * cyScale >> 3) );
	}
	{
	    WORD y = yOrigin + (40 * cyScale >> 3);
	    WORD cxTicksWidth = (409 * cxScale >> 3) - 2;
	    WORD xTicksLeft = xOrigin + (8 * cxScale >> 3);
	    int i;

	    for( i = 0; i <= 4; ++i )
	    {
		WORD x = xTicksLeft + cxTicksWidth * i / 4;
		SetAPen( pwin->RPort, pdri->dri_Pens[TEXTPEN] );
		WritePixel( pwin->RPort, x, y );
		WritePixel( pwin->RPort, x, y+1 );
		WritePixel( pwin->RPort, x+1, y );
		WritePixel( pwin->RPort, x+1, y+1 );
	    }
	}
    } else
	PrintIText( pwin->RPort, &init_it,
		    xOrigin + (212 * cxScale >> 3) - IntuiTextLength(&init_it)/2,
		    yOrigin + (40 * cyScale >> 3) );
    {
	struct EasyStruct es;
	char szVolumeId[11 + MAX_FS_NAME_LEN];
	if(pdlVolume)
	    RawDoFmtSz( szVolumeId, "%b", pdlVolume->dol_Name );
	else
	    RawDoFmtSz( szVolumeId, "in device %s", szDosDevice );
	es.es_StructSize = sizeof(es);
	es.es_Flags = 0;
	es.es_Title = "Format Request";
	es.es_TextFormat =
	    "OK to format volume\n"
	    "%s?\n\n"
	    "WARNING!\n\n"
	    "All data will be lost!\n"
	    "(%s)";
	es.es_GadgetFormat = "Format|Cancel";
	if( EasyRequest( 0, &es, 0,
			 (ULONG)szVolumeId, (ULONG)szCapacityInfo ) != 1 )
	    goto cleanup;
    }

    if(bDoFormat)
    {
	ULONG icyl;
	UWORD xBarLeft, yBarTop, yBarBottom, xFillFrom, xFillTo, cxBarWidth;

	if(!bGetExecDevice(TRUE))
	    goto cleanup;

	/* TODO: use the real bevel edge thicknesses here here */
	xBarLeft = xOrigin + (8 * cxScale >> 3) + 2;
	cxBarWidth = (409 * cxScale >> 3) - 4;
	yBarTop = yOrigin + (28 * cyScale >> 3) + 1,
	yBarBottom = yBarTop + (12 * cyScale >> 3) - 3;
	xFillFrom = xBarLeft;

	SetAPen( pwin->RPort, pdri->dri_Pens[FILLPEN] );

	for( icyl = LowCyl;
	     icyl <= HighCyl;
	     ++icyl )
	{
	    struct IntuiMessage * pim;
	    while( (pim = GT_GetIMsg(pwin->UserPort)) != 0 )
	    {
		BOOL bCancel =
		    pim->Class == IDCMP_GADGETUP
		    && ((struct Gadget *)(pim->IAddress))->GadgetID
		    == gidCancel;
		GT_ReplyIMsg(pim);
		if(bCancel)
		    goto cleanup;
	    }

	    if( !bFormatCylinder(icyl) || !bVerifyCylinder(icyl) )
		goto cleanup;

	    xFillTo = xBarLeft
		+ ((icyl - LowCyl) * cxBarWidth) / (HighCyl + 1 - LowCyl);
	    if( xFillTo >= xFillFrom )
		RectFill( pwin->RPort,
			  xFillFrom, yBarTop, xFillTo - 1, yBarBottom );
	}

	FreeExecDevice();

	RemoveGList( pwin, pgadList, -1 );
	FreeGadgets(pgadList);
	pgadList = 0;

	EraseRect( pwin->RPort, xOrigin, yOrigin,
		   pwin->Width - pwin->BorderRight - 1,
		   pwin->Height - pwin->BorderBottom - 1 );
	PrintIText( pwin->RPort, &init_it,
		    xOrigin + (212 * cxScale >> 3) - IntuiTextLength(&init_it)/2,
		    yOrigin + (40 * cyScale >> 3) );
    }

    if( bMakeFileSys( bFFS, !bFFS, bIntl, !bIntl, bDirCache, !bDirCache )
	&& (!bMakeTrashcan || bMakeFiles(FALSE)) )
	rc = RETURN_OK;

cleanup:
    if(pwin)
	CloseWindow(pwin);
    if(pdri)
	FreeScreenDrawInfo( pscr, pdri );
    if(pscr)
	UnlockPubScreen( 0, pscr );
    FreeGadgets(pgadList);
    FreeVisualInfo(vi);
    CloseFont(ptf);

#ifdef DEBUG
    FreeAll();
    Close(bpfhStdErr);
#endif

    return rc;
}


struct Gadget * pgadCreate(
    struct Gadget * pgadPrevious,
    ULONG gid, ULONG kind, WORD xLeft, WORD yTop, WORD cxWidth, WORD cyHeight,
    const char * psz, ULONG flags, ... )
{
    struct NewGadget ng;
    ng.ng_TextAttr = pwin->WScreen->Font;
    ng.ng_VisualInfo = vi;
    ng.ng_LeftEdge = xOrigin + (xLeft * cxScale >> 3);
    ng.ng_TopEdge = yOrigin + (yTop * cyScale >> 3);
    ng.ng_Width = cxWidth * cxScale >> 3;
    ng.ng_Height = cyHeight * cyScale >> 3;
    ng.ng_GadgetText = (UBYTE *)psz;
    ng.ng_Flags = flags;
    ng.ng_GadgetID = gid;

    return CreateGadgetA( kind, pgadPrevious, &ng,
			  (struct TagItem *)(&flags+1) );
}
