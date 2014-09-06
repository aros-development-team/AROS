#define DEBUG 0

#include <clib/alib_protos.h>
#include <devices/trackdisk.h>
#include <dos/dosextens.h>
#include <intuition/imageclass.h>
#include <libraries/iffparse.h>
#include <libraries/mui.h>
#include <utility/tagitem.h>
#include <workbench/startup.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <stdio.h>

#if defined(__AROS__)
#include <aros/debug.h>
#include <prefs/trackdisk.h>
#else
#include "prefs_common.h"
#define D(x)
#endif
#include "trackdisk_prefs.h"

#include "locale.h"

Object                *App = NULL,
                      *MainWin = NULL,
                      *SaveButton = NULL,
                      *UseButton = NULL,
                      *CancelButton = NULL;
struct DriveControls  Drives[TD_NUMUNITS];
struct WindowGroup    MainGrp;
struct TrackdiskPrefs TDPrefs;
struct IORequest      TDIO;
int res = RETURN_OK;

int __nocommandline;

int main(void)
{
    ULONG signals;
    int i;
    ULONG retval = 0;

    D(bug("[Trackdisk.Prefs] Main()\n"));

    Locale_Initialize();

    for (i = 0; i < TD_NUMUNITS; i++)
	    InitUnitPrefs(&TDPrefs.UnitPrefs[i], i);
    LoadPrefs();
    for (i = 0; i < TD_NUMUNITS; i++) {
	    MainGrp.DriveGroup[i].ti_Tag = MUIA_Group_Child;
	    MainGrp.DriveGroup[i].ti_Data = (IPTR)CreateDriveControls(&Drives[i], i);
    }
    MainGrp.TagChild = MUIA_Group_Child;
    MainGrp.ButtonsGroup = MUI_NewObject("Group.mui", MUIA_Group_Horiz, TRUE,
	    MUIA_Group_Child,
        SaveButton = ImageButton(_(MSG_SAVE), "THEME:Images/Gadgets/Save"),
	    MUIA_Group_Child,
        UseButton = ImageButton(_(MSG_USE), "THEME:Images/Gadgets/Use"),
	    MUIA_Group_Child,
	    CancelButton = ImageButton(_(MSG_CANCEL), "THEME:Images/Gadgets/Cancel"),
    TAG_DONE);
    MainGrp.TagDone = TAG_DONE;
    App = MUI_NewObject("Application.mui", MUIA_Application_Author, "Pavel Fedin and Team AROS",
	    MUIA_Application_Base, (IPTR)"TRACKDISKPREFS",
	    MUIA_Application_Copyright, (IPTR)"(c) 2006-2007 Pavel Fedin and Team AROS",
        MUIA_Application_Description, __(MSG_DESCRIPTION),
	    MUIA_Application_SingleTask, TRUE,
        MUIA_Application_Title, __(MSG_NAME),
	MUIA_Application_Version, (IPTR)"$VER: Trackdisk 41.5 (30.11.2007) ©2007 Pavel Fedin and Team AROS",
	    MUIA_Application_Window,
	    MainWin = MUI_NewObject("Window.mui", MUIA_Window_ID, MAKE_ID('M', 'A', 'I', 'N'),
            MUIA_Window_Title,__(MSG_WINDOW_TITLE),
		    MUIA_Window_RootObject, MUI_NewObjectA("Group.mui", (struct TagItem *)&MainGrp),
	    TAG_DONE),
    TAG_DONE);
    if (App) {
	    DoMethod(SaveButton, MUIM_Notify, MUIA_Pressed, FALSE, App, 2, MUIM_Application_ReturnID, 1);
	    DoMethod(UseButton, MUIM_Notify, MUIA_Pressed, FALSE, App, 2, MUIM_Application_ReturnID, 2);
	    DoMethod(CancelButton, MUIM_Notify, MUIA_Pressed, FALSE, App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	    DoMethod(MainWin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	    SetAttrs(MainWin, MUIA_Window_Open, TRUE, TAG_DONE);
	    while (retval != MUIV_Application_ReturnID_Quit) {
		    retval = DoMethod(App, MUIM_Application_NewInput, &signals);
		    if (retval && (retval != MUIV_Application_ReturnID_Quit)) {
			    for (i = 0; i < TD_NUMUNITS; i++)
				    ControlsToPrefs(&Drives[i], &TDPrefs.UnitPrefs[i]);
			    if (retval == 1)
				    SavePrefs();
			    UsePrefs();
			    retval = MUIV_Application_ReturnID_Quit;
		    }
		    if (signals)
			    signals = Wait(signals);
	    }
	    MUI_DisposeObject(App);
    } else res = RETURN_ERROR;
    Locale_Deinitialize();
    return res;
}

void InitUnitPrefs(struct TDU_Prefs *UnitPrefs, int nunit)
{
	D(bug("[Trackdisk.Prefs] InitUnitPrefs()\n"));
	UnitPrefs->TagUnitNum	= TDPR_UnitNum;
	UnitPrefs->Unit		= nunit;
	UnitPrefs->TagPubFlags	= TDPR_PubFlags;
	UnitPrefs->PubFlags	= 0;
	UnitPrefs->TagRetryCnt	= TDPR_RetryCnt;
	UnitPrefs->RetryCnt	= 3;
}

Object *CreateDriveControls(struct DriveControls *dc, int ndrive)
{
	D(bug("[Trackdisk.Prefs] CreateDriveControls()\n"));
    sprintf(dc->DriveLabel, _(MSG_DRIVE), ndrive);
	return MUI_NewObject("Group.mui", MUIA_Group_Horiz, TRUE,
		MUIA_Disabled, dc->Disabled,
		MUIA_FrameTitle, dc->DriveLabel,
		MUIA_Background, MUII_GroupBack,
		MUIA_Frame, MUIV_Frame_Group,
        MUIA_Group_Child, MUI_MakeObject(MUIO_Label, _(MSG_NOCLICK), 0),
		MUIA_Group_Child,
		dc->NoClickSwitch = MUI_NewObject("Image.mui", MUIA_Image_Spec, MUII_CheckMark,
			MUIA_InputMode, MUIV_InputMode_Toggle,
			MUIA_Selected, TDPrefs.UnitPrefs[ndrive].PubFlags & TDPF_NOCLICK,
			MUIA_CycleChain, TRUE,
			MUIA_Frame, MUIV_Frame_ImageButton,
			MUIA_ShowSelState, FALSE,
		TAG_DONE),
        MUIA_Group_Child, MUI_MakeObject(MUIO_Label, _(MSG_RETRIES), 0),
		MUIA_Group_Child,
		dc->RetriesSlider = MUI_NewObject("Slider.mui", MUIA_CycleChain, TRUE,
			MUIA_Numeric_Min, 1, MUIA_Numeric_Max, 10,
			MUIA_Numeric_Value, TDPrefs.UnitPrefs[ndrive].RetryCnt,
		TAG_DONE),
	TAG_DONE);
}

void LoadPrefs(void)
{
	ULONG i;
	struct TDU_PublicUnit *tdu = NULL;

	D(bug("[Trackdisk.Prefs] LoadPrefs()\n"));

	for (i = 0; i < TD_NUMUNITS; i++) {
		if (OpenDevice("trackdisk.device", i, &TDIO, 0))
			Drives[i].Disabled = TRUE;
		else {
			tdu = (struct TDU_PublicUnit *)TDIO.io_Unit;
			TDPrefs.UnitPrefs[i].PubFlags = tdu->tdu_PubFlags;
			TDPrefs.UnitPrefs[i].RetryCnt = tdu->tdu_RetryCnt;
			CloseDevice(&TDIO);
			Drives[i].Disabled = FALSE;
		}
	}
}

void ControlsToPrefs(struct DriveControls *dc, struct TDU_Prefs *pr)
{
	IPTR data;

	D(bug("[Trackdisk.Prefs] ControlsToPrefs()\n"));

	GetAttr(MUIA_Selected, dc->NoClickSwitch, &data);
	pr->PubFlags = data ? TDPF_NOCLICK : 0 ;
	GetAttr(MUIA_Numeric_Value, dc->RetriesSlider, &data);
	pr->RetryCnt = data;
}

void SavePrefs(void)
{
	BPTR cf;

	D(bug("[Trackdisk.Prefs] SavePrefs()\n"));

	cf = Open(TRACKDISK_PREFS_NAME, MODE_NEWFILE);
	if (cf) {
		TDPrefs.TagDone = TAG_DONE;
		Write(cf, &TDPrefs, sizeof(TDPrefs));
		Close(cf);
	}
}

void UsePrefs(void)
{
	ULONG i;
	struct TDU_PublicUnit *tdu = NULL;

	D(bug("[Trackdisk.Prefs] UsePrefs()\n"));

	for (i = 0; i < TD_NUMUNITS; i++) {
		if ((!Drives[i].Disabled) && (!OpenDevice("trackdisk.device", i, &TDIO, 0))) {
			tdu = (struct TDU_PublicUnit *)TDIO.io_Unit;
			tdu->tdu_PubFlags = TDPrefs.UnitPrefs[i].PubFlags;
			tdu->tdu_RetryCnt = TDPrefs.UnitPrefs[i].RetryCnt;
			CloseDevice(&TDIO);
		}
	}
}

