/* Main code for LCD test */

#include <Pilot.h>
#include <System/SerialMgr.h>
#include <System/StringMgr.h>
#include <string.h>
#include "callback.h"
#define __PALM_CODE__
#include "../registers.h"

#include "loaderRsc.h"

ULong get_ssp(void);

struct PalmMemory 
{
	ULong	reg_SSP;	/* current position of the supervisor stack pointer */
	ULong	reg_LSSA;	/* current start address of the LCD display memory */
	Word	reg_CSGBA;
	Word	reg_CSGBB;
	Word	reg_CSGBC;
	Word	reg_CSGBD;
	Word	reg_CSUGBA;
	Word	reg_CSCR;
	Word	reg_CSA;
	Word	reg_CSB;
	Word	reg_CSC;
	Word	reg_CSD;
	Word	reg_DRAMMC;
	Word	reg_DRAMC;
	Word	reg_SDCTRL;
	Word	reg_SDPWDN;
};

int _strncmp(const char * s1, const char * s2, int len)
{
	int c = 0;
	int d;
	while (c < len) {
		d = s1[c] - s2[c];
		if (d != 0)
			return d;
		c++;
	} 
	return 0;
}

int do_handshake(UInt refNum)
{
	int rc = -1;
	ULong numBytes;
	UChar rcvBuf[6];
	int count = 0;
	int done = 0;
	Err err;

	while (count < 5 && 0 == done) {
		SerSend(refNum,"AROS?",5, &err);
		SerSendWait(refNum, -1);
		numBytes = SerReceive(refNum,rcvBuf,5,1000,&err);
		if (5 == numBytes) {
			if (0 == _strncmp("AROS!",rcvBuf,5)) {
				SerSend(refNum,"greets!",7,&err);
				SerSendWait(refNum,-1);
				done = 1;
				rc = 0;
			} else {
				SerSend(refNum,rcvBuf,numBytes,&err);
			}
		} else {
			SerReceiveFlush(refNum, 100);
		}
		count ++;
	}
	return rc;
}

int download_AROS(void)
{
	int rc = 0;
	Err err;
	UInt refNum;
	err = SysLibFind("Serial Library",&refNum);
	if (0 == err) {
		err = SerOpen(refNum, 0, 57600);
		if (0 == err) {
			/*
			 * Serial Port is opened. So let's try a handshake
			 */
			if (0 == do_handshake(refNum)) {
				/*
				 * Let me transmit memory layout now.
				 */
				struct PalmMemory pm;
				pm.reg_SSP  = get_ssp();
				pm.reg_LSSA = RREG_L(LSSA);
				pm.reg_CSGBA = RREG_W(CSGBA);
				pm.reg_CSGBB = RREG_W(CSGBB);
				pm.reg_CSGBC = RREG_W(CSGBC);
				pm.reg_CSGBD = RREG_W(CSGBD);
				pm.reg_CSUGBA= RREG_W(CSUGBA);
				pm.reg_CSCR  = RREG_W(CSCR);
				pm.reg_CSA   = RREG_W(CSA);
				pm.reg_CSB   = RREG_W(CSB);
				pm.reg_CSC   = RREG_W(CSC);
				pm.reg_CSD   = RREG_W(CSD);
				pm.reg_DRAMMC= RREG_W(DRAMMC);
				pm.reg_DRAMC = RREG_W(DRAMC);
				pm.reg_SDCTRL= RREG_W(SDCTRL);
				pm.reg_SDPWDN= RREG_W(SDPWDN);
			}
			
			SerClose(refNum);
		}
		else
			return -1;
	} else
		rc = -1;		
	return rc;
}

static Boolean MainFormHandleEvent (EventPtr e)
{
    Boolean handled = false;
    FormPtr frm;
    int rc;
    CALLBACK_PROLOGUE

    switch (e->eType) {
    case frmOpenEvent:
	frm = FrmGetActiveForm();
	FrmDrawForm(frm);

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
		case Button1:
			rc = download_AROS();
		break;
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
