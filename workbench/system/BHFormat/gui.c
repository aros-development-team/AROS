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

#define MUIMASTER_YES_INLINE_STDARG
#include <libraries/mui.h>
#include <proto/muimaster.h>

#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <intuition/intuition.h>
#include <stdarg.h>
#include <workbench/startup.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <string.h>

#include "format.h"
#include "locale.h"

static struct DosList * pdlVolume = 0;
static char szCapacityInfo[5+1+8+2+2+2+4+1];
static Object *app, *mainwin, *formatwin, *chk_trash, *chk_intl, *chk_ffs, *chk_cache;
static Object *txt_action, *str_volume, *gauge;
static struct Hook btn_format_hook;

static void message(CONST_STRPTR s);

AROS_UFH3S(void, btn_format_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(IPTR *, msg, A1))
{
    AROS_USERFUNC_INIT

    BOOL bDoFormat = *msg;
    BOOL bMakeTrashcan, bFFS, bIntl;
    BOOL bDirCache = FALSE;
    LONG rc = FALSE;

    D(Printf("Full format? %d\n", bDoFormat));

    /* TODO: set volume name same as old one if name is null string */
    if( !bSetSzVolumeFromSz( XGET(str_volume, MUIA_String_Contents) ) )
    {
	goto cleanup;
    }

    bMakeTrashcan = XGET(chk_trash, MUIA_Selected);
    bFFS = XGET(chk_ffs, MUIA_Selected);
    bIntl = XGET(chk_intl, MUIA_Selected);
    bDirCache = XGET(chk_cache, MUIA_Selected);
#ifdef __AROS__
    bDirCache = FALSE;
#endif

    if(bDoFormat)
    {
	set(txt_action, MUIA_Text_Contents, _(MSG_GUI_FORMATTING) );
    }
    else
    {
	set(txt_action, MUIA_Text_Contents, _(MSG_GUI_INITIALIZING) );
    }

    set(formatwin, MUIA_Window_Open, TRUE);
    
    {
	char szVolumeId[11 + MAX_FS_NAME_LEN];
	if(pdlVolume)
	    RawDoFmtSz( szVolumeId, "%b", pdlVolume->dol_Name );
	else
	    RawDoFmtSz( szVolumeId, _(MSG_IN_DEVICE), szDosDevice );
	if( MUI_Request ( app, formatwin, 0,
			    _(MSG_FORMAT_REQUEST_TITLE), _(MSG_FORMAT_REQUEST_GADGETS),
			    _(MSG_FORMAT_REQUEST_TEXT),
			    szVolumeId, szCapacityInfo) != 1)
	    goto cleanup;
    }

    if(bDoFormat)
    {
	ULONG icyl;

	if(!bGetExecDevice(TRUE))
	    goto cleanup;

	set(gauge, MUIA_Gauge_Max, HighCyl-LowCyl);
	for( icyl = LowCyl;
	     icyl <= HighCyl;
	     ++icyl )
	{
	    set(gauge, MUIA_Gauge_Max, icyl-LowCyl);
	    
	    if( !bFormatCylinder(icyl) || !bVerifyCylinder(icyl) )
		goto cleanup;

	}
	FreeExecDevice();
    }

    if( bMakeFileSys( bFFS, !bFFS, bIntl, !bIntl, bDirCache, !bDirCache )
	&& (!bMakeTrashcan || bMakeFiles(FALSE)) )
	rc = RETURN_OK;
    
cleanup:
    DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    AROS_USERFUNC_EXIT
}


int rcGuiMain(void)
{
    static char szTitle[6+3+30+1];
    struct DosList *pdlDevice = NULL;
#ifdef AROS_FAKE_LOCK
    char volName[108];
#else
    struct FileLock * pflVolume = 0;
#endif
    static struct InfoData dinf __attribute__((aligned (4)));
    LONG rc = RETURN_FAIL;

#if DEBUG
    BPTR bpfhStdErr =
	Open("CON:0/50/640/400/Format Debug Output/CLOSE/WAIT",MODE_READWRITE);
    BPTR OldInput = SelectInput(bpfhStdErr);
    BPTR OldOutput = SelectOutput(bpfhStdErr);
#endif

    if( _WBenchMsg->sm_NumArgs > 1 )
    {
	struct DosList *pdlList;

	if( _WBenchMsg->sm_ArgList[1].wa_Lock == 0 )
	{
	    D(Printf("Object specified by name: %s\n", _WBenchMsg->sm_ArgList[1].wa_Name);)
	    /* it's a device */
	    if( !bSetSzDosDeviceFromSz(_WBenchMsg->sm_ArgList[1].wa_Name) ) {
		D(Printf("Bad device name wrom Workbench: %s\n", _WBenchMsg->sm_ArgList[1].wa_Name));
		/* Workbench is playing silly buggers */
		goto cleanup;
	    }
	}
	else if( _WBenchMsg->sm_ArgList[1].wa_Name[0] == 0 )
	{
	    /* it's a volume */

	    D(Printf("Object specified by lock\n"));
	    /* make sure it's mounted before looking for its device */
	    if( !Info( _WBenchMsg->sm_ArgList[1].wa_Lock, &dinf ) )
	    {
		ReportErrSz( ertFailure, 0, 0 );
		goto cleanup;
	    }
#ifdef AROS_FAKE_LOCK
	    if (NameFromLock(_WBenchMsg->sm_ArgList[1].wa_Lock, volName, sizeof(volName))) {
		D(Printf("Volume name: %s\n", volName));
		volName[strlen(volName)-1] = '\0';
		pdlList = LockDosList( LDF_DEVICES | LDF_VOLUMES | LDF_READ );
		pdlVolume = FindDosEntry(pdlList, volName, LDF_VOLUMES);
		if (pdlVolume) {
		    D(Printf("Looking for device = 0x%08lX Unit = 0x%08lX\n",
			     pdlVolume->dol_Ext.dol_AROS.dol_Device,
			     pdlVolume->dol_Ext.dol_AROS.dol_Unit));
		    pdlDevice = pdlList;            
		    do
		    {
			if ((pdlDevice = NextDosEntry(pdlDevice, LDF_DEVICES)) == 0)
			    break;
			D(Printf("Checking device %s:\n", pdlDevice->dol_Ext.dol_AROS.dol_DevName);)
			D(Printf("Device = 0x%08lX Unit = 0x%08lX\n", pdlDevice->dol_Ext.dol_AROS.dol_Device,
				 pdlDevice->dol_Ext.dol_AROS.dol_Unit);)
		    }
		    while((pdlDevice->dol_Ext.dol_AROS.dol_Device != pdlVolume->dol_Ext.dol_AROS.dol_Device) ||
			  (pdlDevice->dol_Ext.dol_AROS.dol_Unit != pdlVolume->dol_Ext.dol_AROS.dol_Unit));
		}
	    }
#else	    
	    pflVolume =
		(struct FileLock *)BADDR(_WBenchMsg->sm_ArgList[1].wa_Lock);
	    pdlVolume = (struct DosList *)BADDR(pflVolume->fl_Volume);
	    pdlList = LockDosList( LDF_DEVICES | LDF_READ );
	    pdlDevice = pdlList;
	    do
	    {
		if ((pdlDevice = NextDosEntry(pdlDevice, LDF_DEVICES)) == 0)
		    break;
	    }
	    while( pdlDevice->dol_Task != pflVolume->fl_Task );
#endif
	    if (!pdlDevice)
	    {
		ReportErrSz( ertFailure, ERROR_DEVICE_NOT_MOUNTED, 0 );
		goto cleanup;
	    }
	    RawDoFmtSz( szDosDevice, "%b", pdlDevice->dol_Name );
	    pchDosDeviceColon = szDosDevice + strlen(szDosDevice);
	    *(pchDosDeviceColon+1) = 0;
	}
	else
	{
	    ReportErrSz( ertFailure, ERROR_OBJECT_WRONG_TYPE, 0 );
	    goto cleanup;
	}

#ifdef AROS_FAKE_LOCK
	if (!bGetDosDevice(pdlDevice, LDF_DEVICES|LDF_VOLUMES|LDF_READ))
#else
	if (!bGetDosDevice(pdlDevice, LDF_DEVICES|LDF_READ))
#endif
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
	const char * pchUnitSymbol = _(MSG_UNITS);

	D(Printf("Calculating capacity info...\n"));
	cUnits = ibyEnd - ibyStart;
	while( (cUnits >>= 10) > 9999 )
	    ++pchUnitSymbol;

	if(pdlVolume)
	    RawDoFmtSz( szCapacityInfo, _(MSG_CAPACITY_USED),
			(ULONG)cUnits, (ULONG)*pchUnitSymbol,
			/* Calculate percentage used, to nearest point. */
			(ULONG)(((ULLONG)dinf.id_NumBlocksUsed*100ULL
				 + dinf.id_NumBlocks/2) / dinf.id_NumBlocks) );
	else
	    RawDoFmtSz( szCapacityInfo, "%lu%lc capacity",
			(ULONG)cUnits, (ULONG)*pchUnitSymbol );
	D(Printf("Done: %s\n", szCapacityInfo));
    }

    if( _WBenchMsg->sm_NumArgs == 1 )
    {
	message(_(MSG_ERROR_WANDERER) );
	/* TODO: display list of devices and have user select one */
	goto cleanup;
    }

    RawDoFmtSz( szTitle, _(MSG_WINDOW_TITLE), szDosDevice );
    D(Printf("Setting window title to '%s'\n", szTitle));

    {
	Object *btn_format, *btn_qformat, *btn_cancel;

	btn_format_hook.h_Entry = (HOOKFUNC)btn_format_function;

	char szDeviceInfo[6+2+MAX_FS_NAME_LEN+2];
	char szVolumeInfo[6+2+MAX_FS_NAME_LEN+2];
	RawDoFmtSz( szDeviceInfo, _(MSG_DEVICE), szDosDevice );
	szVolumeInfo[0] = '\0';
	if(pdlVolume)
	{
	    RawDoFmtSz( szVolumeInfo, _(MSG_VOLUME), pdlVolume->dol_Name );
	}

	D(Printf("Creating GUI...\n"));
	
	app = ApplicationObject,
	    MUIA_Application_Title, __(MSG_APPLICATION_TITLE),
	    MUIA_Application_Version, (IPTR)szVersion,
	    MUIA_Application_Description, __(MSG_DESCRIPTION),
	    MUIA_Application_Copyright, __(MSG_COPYRIGHT),
	    MUIA_Application_Author, __(MSG_AUTHOR),
	    MUIA_Application_Base, (IPTR)"FORMAT",
	    MUIA_Application_SingleTask, FALSE,
	    SubWindow, (IPTR)(mainwin = WindowObject,
		MUIA_Window_ID, MAKE_ID('F','R','M','1'),
		MUIA_Window_Title, (IPTR)szTitle,
		WindowContents, (IPTR)(VGroup,
		    Child, (IPTR)(ColGroup(2),
			Child, (IPTR)Label2( _(MSG_LABEL_CURRENT_INFORMATION) ),
			Child, (IPTR)(TextObject,
			    (IPTR)TextFrame,
			    MUIA_Text_Contents, (IPTR)szDeviceInfo,
			End),
			Child, (IPTR)Label2(""),
			Child, (IPTR)(TextObject,
			    TextFrame,
			    MUIA_Text_Contents, (IPTR)szVolumeInfo,
			End),
			Child, (IPTR)Label2(""),
			Child, (IPTR)(TextObject,
			    TextFrame,
			    MUIA_Text_Contents, (IPTR)szCapacityInfo,
			End),
			Child, (IPTR)Label2( _(MSG_LABEL_NEW_VOLUME_NAME) ),
			Child, (IPTR)(str_volume = StringObject,
			    StringFrame,
			    MUIA_String_Contents, _(MSG_DEFAULT_VOLUME_NAME),
			    MUIA_String_MaxLen, MAX_FS_NAME_LEN,
			End),
			Child, (IPTR)Label2( _(MSG_LABEL_PUT_TRASHCAN) ),
			Child, (IPTR)MUI_MakeObject(MUIO_Checkmark, NULL),
			Child, (IPTR)Label2( _(MSG_LABEL_FFS) ),
			Child, (IPTR)MUI_MakeObject(MUIO_Checkmark, NULL),
			Child, (IPTR)Label2( _(MSG_LABEL_INTL)),
			Child, (IPTR)MUI_MakeObject(MUIO_Checkmark, NULL),
			Child, (IPTR)Label2( _(MSG_LABEL_CACHE) ),
			Child, (IPTR)MUI_MakeObject(MUIO_Checkmark, NULL),
		    End), /* ColGroup */
		    Child, (IPTR) (RectangleObject, 
			MUIA_Rectangle_HBar, TRUE,
			MUIA_FixHeight,      2,
		    End),
		    Child, (IPTR)(HGroup,
			Child, (IPTR)(btn_format  = ImageButton( _(MSG_BTN_FORMAT) , "THEME:Images/Gadgets/Prefs/Save")),
			Child, (IPTR)(btn_qformat = ImageButton( _(MSG_BTN_QFORMAT), "THEME:Images/Gadgets/Prefs/Save")),
			Child, (IPTR)(btn_cancel  = ImageButton( _(MSG_BTN_CANCEL) , "THEME:Images/Gadgets/Prefs/Cancel")),
		    End), /* HGroup */
		End), /* VGroup */
	    End), /* Window */
	    SubWindow, (IPTR)(formatwin = WindowObject,
		MUIA_Window_ID, MAKE_ID('F','R','M','2'),
		MUIA_Window_Title, (IPTR)szTitle,
		WindowContents, (IPTR)(VGroup,
		    Child, (IPTR)(txt_action = TextObject,
			TextFrame,
		    End),
		    Child, (IPTR)(gauge = GaugeObject,
			GaugeFrame,
		    End),
		    Child, (IPTR)SimpleButton( _(MSG_BTN_STOP) ),
		End), /* VGroup */
	    End), /* Window */
	End; /* Application */
	
	if ( ! app)
	{
	    message( _(MSG_ERROR_NO_APPLICATION) );
	    goto cleanup;
	}
	
	DoMethod(mainwin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
	    app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	DoMethod(btn_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
	    app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	DoMethod(btn_format, MUIM_Notify, MUIA_Pressed, FALSE,
	    app, 3, MUIM_CallHook, (IPTR)&btn_format_hook, TRUE);
	DoMethod(btn_qformat, MUIM_Notify, MUIA_Pressed, FALSE,
	    app, 3, MUIM_CallHook, (IPTR)&btn_format_hook, FALSE);

	set(chk_trash, MUIA_Selected, TRUE);
	if( DosType >= 0x444F5300 && DosType <= 0x444F5305 )
	{
	    set(chk_ffs, MUIA_Selected, DosType & 1UL);
	    set(chk_intl, MUIA_Selected, DosType & 6UL);
	    set(chk_cache, MUIA_Selected, DosType & 4UL);
	}
	else
	{
	    set(chk_ffs, MUIA_Disabled, TRUE);
	    set(chk_intl, MUIA_Disabled, TRUE);
	    set(chk_cache, MUIA_Disabled, TRUE);
	}
#ifdef __AROS__
	set(chk_cache, MUIA_Disabled, TRUE);
#endif	
	set(mainwin, MUIA_Window_Open, TRUE);

	if (! XGET(mainwin, MUIA_Window_Open))
	{
	    message( _(MSG_ERROR_NO_WINDOW) );
	    goto cleanup;
	}
	DoMethod(app, MUIM_Application_Execute);
	rc = RETURN_OK;
    }

cleanup:
    MUI_DisposeObject(app);

#if DEBUG
    SelectInput(OldInput);
    SelectOutput(OldOutput);
    Close(bpfhStdErr);
#endif    

    return rc;
}

static void message(CONST_STRPTR s)
{
    if (s)
    {
	D(Printf(s));
	MUI_Request(app, NULL, 0, _(MSG_REQ_ERROR_TITLE), _(MSG_OK), s);
    }
}
