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

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <string.h>

#include "format.h"
#include "locale.h"

static char szCapacityInfo[5+1+8+2+2+2+4+1];
static Object *app, *mainwin, *formatwin, *chk_trash, *chk_intl, *chk_ffs, *chk_cache;
static Object *txt_action, *str_volume, *gauge;
static struct DosList * pdlVolume = 0;
static struct Hook btn_format_hook;
static struct Hook list_consfunc_hook;
static struct Hook list_desfunc_hook;
static struct Hook list_dispfunc_hook;

static void message(CONST_STRPTR s);

struct SFormatEntry
{
    TEXT deviceName[32];
    TEXT volumeName[32];
    TEXT capacityInfo[64];
};

AROS_UFH3S(IPTR, consfunc,
    AROS_UFHA(struct Hook *, h, A0),
	AROS_UFHA(IPTR*, pool, A2),
	AROS_UFHA(struct SFormatEntry *, entry,A1))
{
    AROS_USERFUNC_INIT
    
    int size = sizeof(struct SFormatEntry);
    
    struct SFormatEntry *new = AllocPooled(pool, size);
    
    if (new) memcpy(new, entry, size);
    
    return (IPTR)new;
    
    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, desfunc,
    AROS_UFHA(struct Hook *, h, A0),
	AROS_UFHA(IPTR*, pool, A2),
	AROS_UFHA(struct SFormatEntry *, entry,A1))
{
    AROS_USERFUNC_INIT
    
    FreePooled(pool, entry, sizeof(struct SFormatEntry));
    
    AROS_USERFUNC_EXIT
}

AROS_UFH3S(LONG, dispfunc,
    AROS_UFHA(struct Hook *, h, A0),
	AROS_UFHA(char **, array, A2),
	AROS_UFHA(struct SFormatEntry *, entry,A1))
{
    AROS_USERFUNC_INIT
    
    static char s[128];
    
    strcpy(s, "(");
    if (strlen(entry->volumeName))
    {
    	strcat(s, entry->volumeName);
    	strcat(s, ", ");
    }
    strcat(s, entry->capacityInfo);
    strcat(s, ")");
    
    *array++ = entry->deviceName;
    *array = s;
    
    return 0;
    
    AROS_USERFUNC_EXIT
}

BOOL ObjectTypeOk(struct DosList* device)
{
    struct FileSysStartupMsg *pfssm;
    struct DosEnvec *pdenDevice;
    
    if( (pfssm = (struct FileSysStartupMsg *)
	 BADDR(device->dol_misc.dol_handler.dol_Startup)) == 0
	|| (IPTR)device->dol_misc.dol_handler.dol_Startup == 1  // RAW: device
	|| TypeOfMem(pfssm) == 0
	|| pfssm->fssm_Device == 0
	|| (pdenDevice = (struct DosEnvec *)BADDR(pfssm->fssm_Environ)) == 0
	|| TypeOfMem(pdenDevice) == 0
	|| pdenDevice->de_TableSize < DE_DOSTYPE
	/* Check that parameters that should always be 0, are */
	|| pdenDevice->de_SecOrg != 0
	|| pdenDevice->de_Interleave != 0 )
    {
    	/* error object wrong type */
		return FALSE;
    }
    
    ULLONG cbyTrack = (ULLONG)pdenDevice->de_BlocksPerTrack * (ULLONG)(pdenDevice->de_SizeBlock * sizeof(LONG));
    ULLONG cbyCylinder = cbyTrack * pdenDevice->de_Surfaces;

    ibyStart = pdenDevice->de_LowCyl * cbyCylinder;
    ibyEnd = (pdenDevice->de_HighCyl + 1) * cbyCylinder;    
    
    return TRUE;
}

void ComputeCapacity(struct DosList *pdlVolume, struct InfoData *pInfoData)
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

	if(pdlVolume && pInfoData)
	    RawDoFmtSz( szCapacityInfo, _(MSG_CAPACITY_USED),
			(ULONG)cUnits, (ULONG)*pchUnitSymbol,
			/* Calculate percentage used, to nearest point. */
			(ULONG)(((ULLONG)pInfoData->id_NumBlocksUsed*100ULL
				 + pInfoData->id_NumBlocks/2) / pInfoData->id_NumBlocks) );
	else
	    RawDoFmtSz( szCapacityInfo, "%lu%lc capacity",
			(ULONG)cUnits, (ULONG)*pchUnitSymbol );
	D(Printf("Done: %s\n", szCapacityInfo));
}

void VolumesToList(Object* listObject)
{
    ULONG flags = LDF_READ | LDF_DEVICES | LDF_VOLUMES;
    struct DosList *adl, *device, *volume;
	
    adl = device = AttemptLockDosList(flags);
	
    if (adl == NULL) return;

    while((device = NextDosEntry(device, flags - LDF_VOLUMES)) != NULL)
    {
	TEXT name[128];
	strcpy(name, AROS_BSTR_ADDR(device->dol_Name));
	strcat(name, ":");
		
	/* exclude RAM, NIL, RAW, CON, ... */
	if (!ObjectTypeOk(device)) continue;
    
	/* floppy, cdrom and unformatted disks can't be locked */
	BPTR lock = Lock(name, SHARED_LOCK);
	BOOL addDevice = TRUE;
	struct InfoData *pInfoData = NULL;
			
	if (lock)
	{
	    struct InfoData infoData;
			
	    /* XPIPE and PIPEFS fails here */
	    if (Info(lock, &infoData))
	    {
		pInfoData = &infoData;
	    }
	    else addDevice = FALSE;

	    UnLock(lock);
	}
			
	if (addDevice)
	{
	    struct SFormatEntry entry;
	    strcpy(entry.deviceName, AROS_BSTR_ADDR(device->dol_Name));
	    volume = adl;            
    	    
    	    do
    	    {
		if ((volume = NextDosEntry(volume, LDF_VOLUMES)) == 0) break;
	    } while (strcmp(AROS_BSTR_ADDR(volume->dol_Name), AROS_BSTR_ADDR(device->dol_Name)) != 0);
	
	    if (volume) strcpy(entry.volumeName, AROS_BSTR_ADDR(volume->dol_Name));
	    else strcpy(entry.volumeName, "");
	
	    ComputeCapacity(volume, pInfoData);
	    strcpy(entry.capacityInfo, szCapacityInfo);
	
	    DoMethod(listObject, MUIM_List_InsertSingle, &entry,
		MUIV_List_Insert_Top);				
	}
    }
		
    UnLockDosList(flags);
}

struct SFormatEntry* SelectDevice(void)
{
    Object *app, *wnd, *list, *ok, *cancel;
    
    list_consfunc_hook.h_Entry = (HOOKFUNC)consfunc;
    list_desfunc_hook.h_Entry = (HOOKFUNC)desfunc;
    list_dispfunc_hook.h_Entry = (HOOKFUNC)dispfunc;

    app = (Object *)ApplicationObject,
        SubWindow, (IPTR)(wnd = (Object *)WindowObject,
            MUIA_Window_CloseGadget, (IPTR)FALSE,
            MUIA_Window_Title, (IPTR)_(MSG_APPLICATION_TITLE),
            WindowContents, (IPTR)(VGroup,
		Child, (IPTR)(TextObject,
		    MUIA_Text_Contents, (IPTR)_(MSG_SELECTDEVICE),
		End),
		Child, (IPTR)(list = (Object *)ListviewObject,
		    MUIA_Listview_List, (IPTR)ListObject,
			InputListFrame,
			MUIA_List_ConstructHook, (IPTR)&list_consfunc_hook,
			MUIA_List_DestructHook, (IPTR)&list_desfunc_hook,
			MUIA_List_DisplayHook, (IPTR)&list_dispfunc_hook,
			MUIA_List_Format, (IPTR)",",
			MUIA_List_AdjustWidth, (IPTR)TRUE,
		    End,
            	End),
                Child, (IPTR)(HGroup,
		    Child, (IPTR)(ok = SimpleButton(_(MSG_OK))),
		    Child, (IPTR)RectangleObject, End,
                    Child, (IPTR)(cancel = SimpleButton(_(MSG_CANCEL))),
               	End),
            End),
        End),
    End;

    if(app == NULL) return NULL;
	
    VolumesToList(list);

    DoMethod(list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
        ok , 3, MUIM_Set, MUIA_Disabled, FALSE);
    DoMethod(ok, MUIM_Notify, MUIA_Pressed, FALSE,
	app, 2, MUIM_Application_ReturnID, (IPTR)ok);
    DoMethod(cancel, MUIM_Notify, MUIA_Pressed, FALSE,
	app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    
    set(ok, MUIA_Disabled, TRUE);
    set(wnd, MUIA_Window_Open, TRUE);
	
    ULONG sigs = 0;
 
    // Main loop
    struct SFormatEntry* selectedEntry = NULL;
    IPTR returnId;
    while((returnId = (IPTR)DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs))
	!= MUIV_Application_ReturnID_Quit)
    {
	if (sigs)
        {
       	    if ((Object*) returnId == ok)
            {
	        IPTR active = XGET(list, MUIA_List_Active);
		struct SFormatEntry *entry;
		DoMethod(list, MUIM_List_GetEntry, active, &entry);
		selectedEntry = AllocMem(sizeof(struct SFormatEntry), 0L);
		if (selectedEntry)
		    memcpy(selectedEntry, entry, sizeof(struct SFormatEntry));
           	break;
	    }
                	
            sigs = Wait(sigs | SIGBREAKF_CTRL_C);
            
            if (sigs & SIGBREAKF_CTRL_C)
                break;
	}
    }	

    MUI_DisposeObject(app);
    
    return selectedEntry;	
}

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
    
    if(!bSetSzDosDeviceFromSz(szDosDevice))
        goto cleanup;
    
    /* TODO: set volume name same as old one if name is null string */
    if( !bSetSzVolumeFromSz( (char *)XGET(str_volume, MUIA_String_Contents) ) )
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
    
    set(mainwin, MUIA_Window_Open, FALSE);
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
 
    BOOL formatOk = TRUE;
    if(bDoFormat)
    {
	ULONG icyl, sigs;
	if(!bGetExecDevice(TRUE))
	    goto cleanup;
	D(PutStr("GetExecDevice done\n"));
	set(gauge, MUIA_Gauge_Max, HighCyl-LowCyl);
	for( icyl = LowCyl; icyl <= HighCyl; ++icyl )
	{
	    DoMethod(app, MUIM_Application_Input, (IPTR)&sigs);
	    /* interrupted by user? */
	    if(XGET(formatwin, MUIA_UserData) == 1)
	    {
		formatOk = FALSE;
		break;
	    }
	    if(!bFormatCylinder(icyl) || !bVerifyCylinder(icyl))
	    {
		formatOk = FALSE;
		break;
	    }
	    set(gauge, MUIA_Gauge_Current, icyl-LowCyl);    
	}
	FreeExecDevice();
	D(PutStr("FreeExecDevice done\n"));
    }

    if(formatOk)
	if( bMakeFileSys( bFFS, !bFFS, bIntl, !bIntl, bDirCache, !bDirCache )
            && (!bMakeTrashcan || bMakeFiles(FALSE)) )
            rc = RETURN_OK;
    
cleanup:
    DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    if (rc != RETURN_OK) {
        /* Shouldn't we do something if rc != RETURN_OK? */
    }

    AROS_USERFUNC_EXIT
}

int rcGuiMain(void)
{
    static char szTitle[6+3+30+1];
    char szVolumeInfo[6+2+MAX_FS_NAME_LEN+2];
    char szDeviceInfo[6+2+MAX_FS_NAME_LEN+2];
    char szVolumeName[108];
    struct DosList *pdlDevice = NULL;
    szVolumeInfo[0] = '\0';
    struct FileLock * pflVolume = 0;
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

	ComputeCapacity(pdlVolume, &dinf );
	
	if (pdlVolume)
	    strcpy(szVolumeName, AROS_BSTR_ADDR(pdlVolume->dol_Name)); 
    }
    else if ( _WBenchMsg->sm_NumArgs == 1 )
    {
	/* message(_(MSG_ERROR_WANDERER) ); */
	struct SFormatEntry* entry = SelectDevice();
	if (!entry)
	    goto cleanup;
	strcpy(szDosDevice, entry->deviceName);
	strcat(szDosDevice, ":");
	strcpy(szVolumeName, entry->volumeName);
	strcpy(szCapacityInfo, entry->capacityInfo);
	FreeMem(entry, sizeof(struct SFormatEntry));

        D(Printf("Object name selected via SelectDevice: %s\n", szDosDevice));
        /* it's a device */
        if( !bSetSzDosDeviceFromSz(szDosDevice) ) {
            D(Printf("Bad device name from SelectDevice: %s\n", szDosDevice));
            goto cleanup;
        }
    }
    
    if (!bGetDosDevice(pdlDevice, LDF_DEVICES|LDF_READ))
	goto cleanup;
	    
    RawDoFmtSz( szTitle, _(MSG_WINDOW_TITLE), szDosDevice );
    D(Printf("Setting window title to '%s'\n", szTitle));

    {
	Object *btn_format, *btn_qformat, *btn_cancel, *btn_stop;

	btn_format_hook.h_Entry = (HOOKFUNC)btn_format_function;

	RawDoFmtSz( szDeviceInfo, _(MSG_DEVICE), szDosDevice );
	RawDoFmtSz( szVolumeInfo, _(MSG_VOLUME), szVolumeName );

	D(Printf("Creating GUI...\n"));
	
	app = (Object *)ApplicationObject,
	    MUIA_Application_Title, __(MSG_APPLICATION_TITLE),
	    MUIA_Application_Version, (IPTR)szVersion,
	    MUIA_Application_Description, __(MSG_DESCRIPTION),
	    MUIA_Application_Copyright, __(MSG_COPYRIGHT),
	    MUIA_Application_Author, __(MSG_AUTHOR),
	    MUIA_Application_Base, (IPTR)"FORMAT",
	    MUIA_Application_SingleTask, FALSE,
	    SubWindow, (IPTR)(mainwin = (Object *)WindowObject,
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
			Child, (IPTR)(str_volume = (Object *)StringObject,
			    StringFrame,
			    MUIA_String_Contents, __(MSG_DEFAULT_VOLUME_NAME),
			    MUIA_String_MaxLen, MAX_FS_NAME_LEN,
			End),
			Child, (IPTR)Label2( _(MSG_LABEL_PUT_TRASHCAN) ),
			Child, (IPTR)(chk_trash = MUI_MakeObject(MUIO_Checkmark, NULL)),
			Child, (IPTR)Label2( _(MSG_LABEL_FFS) ),
			Child, (IPTR)(chk_ffs = MUI_MakeObject(MUIO_Checkmark, NULL)),
			Child, (IPTR)Label2( _(MSG_LABEL_INTL)),
			Child, (IPTR)(chk_intl = MUI_MakeObject(MUIO_Checkmark, NULL)),
			Child, (IPTR)Label2( _(MSG_LABEL_CACHE) ),
			Child, (IPTR)(chk_cache = MUI_MakeObject(MUIO_Checkmark, NULL)),
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
	    SubWindow, (IPTR)(formatwin = (Object *)WindowObject,
		MUIA_Window_ID, MAKE_ID('F','R','M','2'),
		MUIA_Window_Title, (IPTR)szTitle,
		MUIA_Window_CloseGadget, (IPTR)FALSE,
		WindowContents, (IPTR)(VGroup,
		    Child, (IPTR)(txt_action = (Object *)TextObject,
			TextFrame,
		    End),
		    Child, (IPTR)(gauge = (Object *)GaugeObject,
			GaugeFrame,
			MUIA_Gauge_Horiz, (IPTR)TRUE,
			MUIA_FixHeightTxt, (IPTR)"|",
		    End),
		    Child, (IPTR)(HGroup,
		    	Child, (IPTR)RectangleObject, End,
		    	Child, (IPTR)(btn_stop = SimpleButton( _(MSG_BTN_STOP) )),
		    	Child, (IPTR)RectangleObject, End,
		    End),
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
	DoMethod(btn_stop, MUIM_Notify, MUIA_Pressed, FALSE,
	    formatwin, 3, MUIM_Set, MUIA_UserData, 1);
	    
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
