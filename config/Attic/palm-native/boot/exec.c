/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
    
    Main code for LCD test 
*/

#include <Pilot.h>
#include "callback.h"

#include "execRsc.h"

#define __PALM_CODE__
#include "palmgfx.h"

extern void main_init(void * memory, Long size);


static Boolean MainFormHandleEvent (EventPtr e)
{
    Boolean handled = false;
    FormPtr frm;
    void * mem;
    Long MEMSIZE = 0x9500;
    CALLBACK_PROLOGUE

    switch (e->eType) {
    case frmOpenEvent:
	frm = FrmGetActiveForm();
	FrmDrawForm(frm);

	mem = MemPtrNew(MEMSIZE);
	/*
	 * Memory must be properly aligned!
	 */
	(Long)mem = ((Long)mem + 7) & ~7;
	if (0 != mem && 0 == ((Long)mem & 7)) {
		clearscreen(0);
		main_init(mem,MEMSIZE);
	}

	handled = true;
	break;

    case menuEvent:
	MenuEraseStatus(NULL);

	switch(e->data.menu.itemID) {
	}

    	handled = true;
	break;

    case ctlSelectEvent:
	switch(e->data.ctlSelect.controlID) {
	}
	break;

    default:
        break;
    }

    CALLBACK_EPILOGUE

    return handled;
}

static Boolean ApplicationHandleEvent(EventPtr e)
{
    FormPtr frm;
    Word    formId;
    Boolean handled = false;

    if (e->eType == frmLoadEvent) {
	formId = e->data.frmLoad.formID;
	frm = FrmInitForm(formId);
	FrmSetActiveForm(frm);

	switch(formId) {
	case MainForm:
	    FrmSetEventHandler(frm, MainFormHandleEvent);
	    break;
	}
	handled = true;
    }

    return handled;
}

/* Get preferences, open (or create) app database */
static Word StartApplication(void)
{
    FrmGotoForm(MainForm);
    return 0;
}

/* Save preferences, close forms, close app database */
static void StopApplication(void)
{
    FrmSaveAllForms();
    FrmCloseAllForms();
}

/* The main event loop */
static void EventLoop(void)
{
    Word err;
    EventType e;

    do {
	EvtGetEvent(&e, evtWaitForever);
	if (! SysHandleEvent (&e))
	    if (! MenuHandleEvent (NULL, &e, &err))
		if (! ApplicationHandleEvent (&e))
		    FrmDispatchEvent (&e);
    } while (e.eType != appStopEvent);
}



/* Main entry point; it is unlikely you will need to change this except to
   handle other launch command codes */
DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
    Word err;


    if (cmd == sysAppLaunchCmdNormalLaunch) {

	err = StartApplication();
	if (err) return err;

	EventLoop();
	StopApplication();
	
    } else {
	return sysErrParamErr;
    }

    return 0;
}
