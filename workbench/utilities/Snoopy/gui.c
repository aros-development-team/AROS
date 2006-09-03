/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <aros/debug.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <proto/intuition.h>

#include "main.h"
#include "gui.h"
#include "setup.h"
#include "patches.h"
#include "locale.h"

#define VERSION "$VER: Snoopy 0.4 (3.9.2006) © 2006 The AROS Dev Team"

static Object *app, *window, *saveBtn, *openBtn, *useBtn, *undoBtn, *resetBtn, *cancelBtn;
static Object *failCM, *cliCM, *pathCM, *devCM, *ignoreCM;
static Object *nameNum, *actionNum, *targetNum, *optionNum;
static Object *changeDirCM, *deleteCM, *executeCM, *getVarCM, *loadSegCM, *lockCM, *makeDirCM, *makeLinkCM;
static Object *openCM, *renameCM, *runCommandCM, *setVarCM, *systemCM, *findPortCM, *findResidentCM, *findSemaphoreCM;
static Object *findTaskCM, *lockScreenCM, *openDeviceCM, *openFontCM, *openLibraryCM, *openResourceCM, *readToolTypesCM;

static struct Hook save_hook, open_hook, use_hook, undo_hook, reset_hook, cancel_hook;

AROS_UFH3S(void, save_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    oldsetup = setup;
    gui_get();
    patches_set();
    if ( ! setup_save())
    {
	MUI_Request ( app, window, 0, MSG(MSG_TITLE), MSG(MSG_OK), MSG(MSG_ERROR_SAVE), PREFFILE);
    }
    
    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, open_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    if ( ! setup_open() )
    {
	MUI_Request ( app, window, 0, MSG(MSG_TITLE), MSG(MSG_OK), MSG(MSG_ERROR_LOAD), PREFFILE);
    }
    gui_set();
    patches_set();

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, reset_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    oldsetup = setup;
    setup_reset();
    gui_set();
    patches_set();

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, use_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    oldsetup = setup;
    gui_get();
    patches_set();

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, undo_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    setup = oldsetup;
    gui_set();
    patches_set();

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, cancel_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    // FIXME: original SnoopDos has a separate task for the patches. When cancelling SnoopDos the GUI
    // is removed and the patchtask stays in RAM. We can't do that because we have no separate task.
    // The cancelling function of Snoopy is more for debugging.
    if (MUI_Request ( app, window, 0, MSG(MSG_TITLE), MSG(MSG_RESET_PATCHES), MSG(MSG_ASK_RESET)))
    {
	set(saveBtn, MUIA_Disabled, TRUE);
	set(openBtn, MUIA_Disabled, TRUE);
	set(useBtn, MUIA_Disabled, TRUE);
	set(undoBtn, MUIA_Disabled, TRUE);
	set(resetBtn, MUIA_Disabled, TRUE);
	set(cancelBtn, MUIA_Disabled, TRUE);
	patches_reset();
	if (MUI_Request ( app, window, 0, MSG(MSG_TITLE), MSG(MSG_REMOVE_SNOOP), MSG(MSG_ASK_REMOVE)))
	{
	    DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	}
    }
    AROS_USERFUNC_EXIT
}

void gui_init(void)
{
    save_hook.h_Entry = (APTR)save_function;
    open_hook.h_Entry = (APTR)open_function;
    use_hook.h_Entry = (APTR)use_function;
    undo_hook.h_Entry = (APTR)undo_function;
    reset_hook.h_Entry = (APTR)reset_function;
    cancel_hook.h_Entry = (APTR)cancel_function;

    app = ApplicationObject,
	MUIA_Application_Title, (IPTR) MSG(MSG_TITLE),
	MUIA_Application_Version, (IPTR) VERSION,
	MUIA_Application_Copyright, (IPTR) MSG(MSG_COPYRIGHT),
	MUIA_Application_Author, (IPTR) MSG(MSG_AUTHOR),
	MUIA_Application_Description, (IPTR) MSG(MSG_DESCRIPTION),
	MUIA_Application_Base, (IPTR) MSG(MSG_PORTNAME),
	MUIA_Application_SingleTask, TRUE,

	SubWindow, (IPTR)(window = WindowObject,
	    MUIA_Window_Title, (IPTR)MSG(MSG_TITLE),
	    MUIA_Window_CloseGadget, FALSE,
	    WindowContents, (IPTR)(VGroup,
		Child, (IPTR)(HGroup,
		    Child, (IPTR)(VGroup,
			Child, (IPTR)(ColGroup(2),
			    GroupFrameT(MSG(MSG_SETTINGS)),
			    Child, (IPTR)Label(MSG(MSG_SHOW_FAILS)),
			    Child, (IPTR)(failCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label(MSG(MSG_CLI_NUMBER)),
			    Child, (IPTR)(cliCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label(MSG(MSG_FULL_PATH)),
			    Child, (IPTR)(pathCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label(MSG(MSG_DEVICE)),
			    Child, (IPTR)(devCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label(MSG(MSG_IGNORE_WB)),
			    Child, (IPTR)(ignoreCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)HVSpace,
			    Child, (IPTR)HVSpace,
			End),
			Child, (IPTR)(ColGroup(2),
			    GroupFrameT(MSG(MSG_OUTPUT_FIELD)),
			    Child, (IPTR)Label(MSG(MSG_NAME)),
			    Child, (IPTR)(nameNum = SliderObject,
				MUIA_Numeric_Min, 10,
				MUIA_Numeric_Max, 50,
			    End),
			    Child, (IPTR)Label(MSG(MSG_ACTION)),
			    Child, (IPTR)(actionNum = SliderObject,
				MUIA_Numeric_Min, 10,
				MUIA_Numeric_Max, 50,
			    End),
			    Child, (IPTR)Label(MSG(MSG_TARGET)),
			    Child, (IPTR)(targetNum = SliderObject,
				MUIA_Numeric_Min, 10,
				MUIA_Numeric_Max, 50,
			    End),
			    Child, (IPTR)Label(MSG(MSG_OPTION)),
			    Child, (IPTR)(optionNum = SliderObject,
				MUIA_Numeric_Min, 10,
				MUIA_Numeric_Max, 50,
			    End),
			End),
		    End),
		    Child, (IPTR)(VGroup,
			Child, (IPTR)(ColGroup(2),
			    GroupFrameT(MSG(MSG_SYSTEM_FUNC)),
			    Child, (IPTR)Label("FindPort"),
			    Child, (IPTR)(findPortCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("FindResident"),
			    Child, (IPTR)(findResidentCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("FindSemaphore"),
			    Child, (IPTR)(findSemaphoreCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("FindTask"),
			    Child, (IPTR)(findTaskCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("LockScreen"),
			    Child, (IPTR)(lockScreenCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("OpenDevice"),
			    Child, (IPTR)(openDeviceCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("OpenFont"),
			    Child, (IPTR)(openFontCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("OpenLibrary"),
			    Child, (IPTR)(openLibraryCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("OpenResource"),
			    Child, (IPTR)(openResourceCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("ReadToolTypes"),
			    Child, (IPTR)(readToolTypesCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)HVSpace,
			    Child, (IPTR)HVSpace,
			End),
		    End),
		    Child, (IPTR)(VGroup,
			Child, (IPTR)(ColGroup(2),
			    GroupFrameT(MSG(MSG_DOS_FUNC)),
			    Child, (IPTR)Label("ChangeDir"),
			    Child, (IPTR)(changeDirCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("Delete"),
			    Child, (IPTR)(deleteCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("Execute"),
			    Child, (IPTR)(executeCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("GetVar"),
			    Child, (IPTR)(getVarCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("LoadSeg"),
			    Child, (IPTR)(loadSegCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("Lock"),
			    Child, (IPTR)(lockCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("MakeDir"),
			    Child, (IPTR)(makeDirCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("MakeLink"),
			    Child, (IPTR)(makeLinkCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("Open"),
			    Child, (IPTR)(openCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("Rename"),
			    Child, (IPTR)(renameCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("RunCommand"),
			    Child, (IPTR)(runCommandCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("SetVar"),
			    Child, (IPTR)(setVarCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)Label("System"),
			    Child, (IPTR)(systemCM = MUI_MakeObject(MUIO_Checkmark, "")),
			    Child, (IPTR)HVSpace,
			    Child, (IPTR)HVSpace,
			End),
		    End),
		End),
		Child, (IPTR) (RectangleObject, 
		    MUIA_Rectangle_HBar, TRUE,
		    MUIA_FixHeight,      2,
		End),
		Child, (IPTR)(HGroup,
		    Child, (IPTR)(saveBtn = SimpleButton(MSG(MSG_SAVE))),
		    Child, (IPTR)(openBtn = SimpleButton(MSG(MSG_OPEN))),
		    Child, (IPTR)HVSpace,
		    Child, (IPTR)(useBtn = SimpleButton(MSG(MSG_USE))),
		    Child, (IPTR)(undoBtn = SimpleButton(MSG(MSG_UNDO))),
		    Child, (IPTR)(resetBtn = SimpleButton(MSG(MSG_RESET))),
		    Child, (IPTR)HVSpace,
		    Child, (IPTR)(cancelBtn = SimpleButton(MSG(MSG_CANCEL))),
		End),
	    End), // WindowContents
	End), // WindowObject
    End; // ApplicationObject

    if ( ! app)
    {
	D(bug("Cant create application\n"));
	return;
    }
    
    // disable unavailable functions
    set(pathCM,   MUIA_Disabled, TRUE);
    set(devCM,    MUIA_Disabled, TRUE);

    gui_set();
    set(window, MUIA_Window_Open, TRUE);

    DoMethod(saveBtn, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&save_hook);

    DoMethod(openBtn, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&open_hook);

    DoMethod(useBtn, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&use_hook);

    DoMethod(undoBtn, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&undo_hook);

    DoMethod(resetBtn, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&reset_hook);

    DoMethod(cancelBtn, MUIM_Notify, MUIA_Pressed, FALSE,
	(IPTR)app, 2, MUIM_CallHook, (IPTR)&cancel_hook);
}

void gui_handleevents(void)
{
    DoMethod(app, MUIM_Application_Execute);
}

void gui_cleanup(void)
{
    MUI_DisposeObject(app);
}

void gui_set(void)
{
    set(failCM,          MUIA_Selected, setup.onlyShowFails);
    set(cliCM,           MUIA_Selected, setup.showCliNr);
    set(pathCM,          MUIA_Selected, setup.showPaths);
    set(devCM,           MUIA_Selected, setup.useDevNames);
    set(ignoreCM,        MUIA_Selected, setup.ignoreWB);

    set(changeDirCM,     MUIA_Selected, setup.enableChangeDir);
    set(deleteCM,        MUIA_Selected, setup.enableDelete);
    set(executeCM,       MUIA_Selected, setup.enableExecute);
    set(getVarCM,        MUIA_Selected, setup.enableGetVar);
    set(loadSegCM,       MUIA_Selected, setup.enableLoadSeg);
    set(lockCM,          MUIA_Selected, setup.enableLock);
    set(makeDirCM,       MUIA_Selected, setup.enableMakeDir);
    set(makeLinkCM,      MUIA_Selected, setup.enableMakeLink);
    set(openCM,          MUIA_Selected, setup.enableOpen);
    set(renameCM,        MUIA_Selected, setup.enableRename);
    set(runCommandCM,    MUIA_Selected, setup.enableRunCommand);
    set(setVarCM,        MUIA_Selected, setup.enableSetVar);
    set(systemCM,        MUIA_Selected, setup.enableSystem);

    set(findPortCM,      MUIA_Selected, setup.enableFindPort);
    set(findResidentCM,  MUIA_Selected, setup.enableFindResident);
    set(findSemaphoreCM, MUIA_Selected, setup.enableFindSemaphore);
    set(findTaskCM,      MUIA_Selected, setup.enableFindTask);
    set(lockScreenCM,    MUIA_Selected, setup.enableLockScreen);
    set(openDeviceCM,    MUIA_Selected, setup.enableOpenDevice);
    set(openFontCM,      MUIA_Selected, setup.enableOpenFont);
    set(openLibraryCM,   MUIA_Selected, setup.enableOpenLibrary);
    set(openResourceCM,  MUIA_Selected, setup.enableOpenResource);
    set(readToolTypesCM, MUIA_Selected, setup.enableReadToolTypes);

    set(nameNum,         MUIA_Numeric_Value, setup.nameLen);
    set(actionNum,       MUIA_Numeric_Value, setup.actionLen);
    set(targetNum,       MUIA_Numeric_Value, setup.targetLen);
    set(optionNum,       MUIA_Numeric_Value, setup.optionLen);
}

void gui_get(void)
{
    setup.onlyShowFails       = XGET(failCM,          MUIA_Selected);
    setup.showCliNr           = XGET(cliCM,           MUIA_Selected);
    setup.showPaths           = XGET(pathCM,          MUIA_Selected);
    setup.useDevNames         = XGET(devCM,           MUIA_Selected);
    setup.ignoreWB            = XGET(ignoreCM,        MUIA_Selected);

    setup.enableChangeDir     = XGET(changeDirCM,     MUIA_Selected);
    setup.enableDelete        = XGET(deleteCM,        MUIA_Selected);
    setup.enableExecute       = XGET(executeCM,       MUIA_Selected);
    setup.enableGetVar        = XGET(getVarCM,        MUIA_Selected);
    setup.enableLoadSeg       = XGET(loadSegCM,       MUIA_Selected);
    setup.enableLock          = XGET(lockCM,          MUIA_Selected);
    setup.enableMakeDir       = XGET(makeDirCM,       MUIA_Selected);
    setup.enableMakeLink      = XGET(makeLinkCM,      MUIA_Selected);
    setup.enableOpen          = XGET(openCM,          MUIA_Selected);
    setup.enableRename        = XGET(renameCM,        MUIA_Selected);
    setup.enableRunCommand    = XGET(runCommandCM,    MUIA_Selected);
    setup.enableSetVar        = XGET(setVarCM,        MUIA_Selected);
    setup.enableSystem        = XGET(systemCM,        MUIA_Selected);

    setup.enableFindPort      = XGET(findPortCM,      MUIA_Selected);
    setup.enableFindResident  = XGET(findResidentCM,  MUIA_Selected);
    setup.enableFindSemaphore = XGET(findSemaphoreCM, MUIA_Selected);
    setup.enableFindTask      = XGET(findTaskCM,      MUIA_Selected);
    setup.enableLockScreen    = XGET(lockScreenCM,    MUIA_Selected);
    setup.enableOpenDevice    = XGET(openDeviceCM,    MUIA_Selected);
    setup.enableOpenFont      = XGET(openFontCM,      MUIA_Selected);
    setup.enableOpenLibrary   = XGET(openLibraryCM,   MUIA_Selected);
    setup.enableOpenResource  = XGET(openResourceCM,  MUIA_Selected);
    setup.enableReadToolTypes = XGET(readToolTypesCM, MUIA_Selected);

    setup.nameLen             = XGET(nameNum,         MUIA_Numeric_Value);
    setup.actionLen           = XGET(actionNum,       MUIA_Numeric_Value);
    setup.targetLen           = XGET(targetNum,       MUIA_Numeric_Value);
    setup.optionLen           = XGET(optionNum,       MUIA_Numeric_Value);
}

