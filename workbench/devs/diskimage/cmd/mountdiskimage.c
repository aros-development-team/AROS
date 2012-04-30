/* Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/filehandler.h>
#include <workbench/startup.h>
#include <devices/diskimage.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/diskimage.h>
#ifdef __AROS__
#include <libraries/mui.h>
#include <proto/muimaster.h>
#else
#include <reaction/reaction_macros.h>
#include <classes/requester.h>
#include <proto/intuition.h>
#include <proto/requester.h>
#include <clib/alib_protos.h>
#endif
#include <string.h>
#include "support.h"
#include "rev/MountDiskImage_rev.h"

CONST TEXT USED verstag[] = VERSTAG;

#ifndef ACTION_GET_DISK_FSSM
#define ACTION_GET_DISK_FSSM  4201
#define ACTION_FREE_DISK_FSSM 4202
#endif

#define PROGNAME "MountDiskImage"
#define TEMPLATE "D=DEVICE=DRIVE/K,U=UNIT/K/N,INSERT=FILE,EJECT/S," \
	"P=PLUGIN/K,WP=WRITEPROTECT/S,PWD=PASSWORD/K,RELOADPLUGINS/S"

enum {
	ARG_DRIVE,
	ARG_UNIT,
	ARG_INSERT,
	ARG_EJECT,
	ARG_PLUGIN,
	ARG_WRITEPROTECT,
	ARG_PASSWORD,
	ARG_RELOADPLUGINS,
	MAX_ARGS
};

struct WBStartup *WBMsg;
#ifndef __AROS__
struct Library *IconBase;
struct IntuitionBase *IntuitionBase;
struct Library *RequesterBase;
#endif

LONG DoReq (Object *req);
void IoErrRequester (LONG error, CONST_STRPTR header);
void ErrorStringRequester (CONST_STRPTR error_string);

int main (int argc, char **argv) {
	int rc = RETURN_FAIL;
	BPTR stderr = ZERO;
	struct DiskObject *icon = NULL;
	struct RDArgs *rdargs = NULL;
	BPTR filedir = ZERO;
	CONST_STRPTR filename = NULL;
	CONST_STRPTR drivestr = NULL;
	LONG unit = -1;
	BOOL writeprotect = FALSE;
	CONST_STRPTR password = NULL;
	CONST_STRPTR plugin = NULL;
	BOOL eject = FALSE;
	BOOL reloadplugins = FALSE;
	LONG error = NO_ERROR;
	LONG error2 = NO_ERROR;
	TEXT error_buffer[256];

	stderr = Output();

	if (argc == 0) {
		BPTR currdir;
		WBMsg = (struct WBStartup *)argv;
#ifndef __AROS__
		if (!(IconBase = OpenLibrary("icon.library", MIN_OS_VERSION)) ||
			!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", MIN_OS_VERSION)) ||
			!(RequesterBase = OpenLibrary("requester.class", MIN_OS_VERSION)))
		{
			goto error;
		}
#endif
		if (WBMsg->sm_NumArgs < 2) {
			IoErrRequester(ERROR_REQUIRED_ARG_MISSING, NULL);
			goto error;
		}
		filedir = WBMsg->sm_ArgList[1].wa_Lock;
		filename = WBMsg->sm_ArgList[1].wa_Name;
		currdir = CurrentDir(filedir);
		icon = GetDiskObjectNew((STRPTR)filename);
		CurrentDir(currdir);
		if (!icon) {
			IoErrRequester(IoErr(), NULL);
			goto error;
		}
		if (!(drivestr = TTString(icon, "DRIVE", NULL)) &&
			!(drivestr = TTString(icon, "DEVICE", NULL)))
		{
			drivestr = NULL;
		}
		unit = TTInteger(icon, "UNIT", unit);
		writeprotect = TTBoolean(icon, "WP") || TTBoolean(icon, "WRITEPROTECT");
		if (!(password = TTString(icon, "PWD", NULL)) &&
			!(password = TTString(icon, "PASSWORD", NULL)))
		{
			password = NULL;
		}
		plugin = TTString(icon, "PLUGIN", NULL);
	} else {
		IPTR args[MAX_ARGS];
		ClearMem(args, sizeof(args));
		rdargs = ReadArgs(TEMPLATE, args, NULL);
		if (!rdargs) {
			PrintFault(IoErr(), PROGNAME);
			goto error;
		}
		filedir = GetCurrentDir();
		filename = (CONST_STRPTR)args[ARG_INSERT];
		eject = args[ARG_EJECT] ? TRUE : FALSE;
		reloadplugins = args[ARG_RELOADPLUGINS] ? TRUE : FALSE;
		if (reloadplugins) {
			unit = -1;
		} else if (filename || eject) {
			if (args[ARG_DRIVE]) drivestr = (CONST_STRPTR)args[ARG_DRIVE];
			if (args[ARG_UNIT]) unit = *(LONG *)args[ARG_UNIT];
			if (filename) {
				if (args[ARG_PASSWORD]) password = (CONST_STRPTR)args[ARG_PASSWORD];
				if (args[ARG_PLUGIN]) plugin = (CONST_STRPTR)args[ARG_PLUGIN];
				writeprotect = args[ARG_WRITEPROTECT] ? TRUE : FALSE;
			}
		} else {
			PrintFault(ERROR_REQUIRED_ARG_MISSING, PROGNAME);
			goto error;
		}
	}

	if (drivestr && drivestr[0]) {
		TEXT drivename[20];
		LONG len;
		const ULONG flags = LDF_DEVICES|LDF_READ;
		struct DosList *dl;
		BOOL devicefound;
		TEXT devicename[64];
		Strlcpy(drivename, drivestr, sizeof(drivename));
		len = strlen(drivename)-1;
		if (drivename[len] == ':') {
			drivename[len] = 0;
		}
		dl = LockDosList(flags);
		devicefound = FALSE;
		if ((dl = FindDosEntry(dl, drivename, flags))) {
			struct FileSysStartupMsg *fssm;
#ifndef __AROS__
			if (dl->dol_Task) {
				fssm = (struct FileSysStartupMsg *)DoPkt0(dl->dol_Task, ACTION_GET_DISK_FSSM);
				if (fssm) {
					CopyStringBSTRToC(fssm->fssm_Device, devicename, sizeof(devicename));
					unit = fssm->fssm_Unit;
					devicefound = TRUE;
					DoPkt1(dl->dol_Task, ACTION_FREE_DISK_FSSM, (IPTR)fssm);
				} else if (IoErr() == ERROR_ACTION_NOT_KNOWN) {
#endif
					fssm = CheckBPTR(dl->dol_misc.dol_handler.dol_Startup);
					if (fssm && CheckBPTR(fssm->fssm_Device)) {
						CopyStringBSTRToC(fssm->fssm_Device, devicename, sizeof(devicename));
						unit = fssm->fssm_Unit;
						devicefound = TRUE;
					}
#ifndef __AROS__
				}
			}
#endif
		}
		UnLockDosList(flags);
		if (!devicefound) {
			SNPrintf(error_buffer, sizeof(error_buffer), "%s: device not found", drivename);
			if (WBMsg)
				ErrorStringRequester(error_buffer);
			else
				FPrintf(stderr, "%s\n", error_buffer);
			goto error;
		}
		if (strcmp(FilePart(devicename), "diskimage.device")) {
			SNPrintf(error_buffer, sizeof(error_buffer), "%s: not a diskimage device", drivename);
			if (WBMsg)
				ErrorStringRequester(error_buffer);
			else
				FPrintf(stderr, "%s\n", error_buffer);
			goto error;
		}
	}

	if ((filename || eject) && unit == -1) {
		if (WBMsg)
			IoErrRequester(ERROR_REQUIRED_ARG_MISSING, NULL);
		else
			PrintFault(ERROR_REQUIRED_ARG_MISSING, PROGNAME);
		goto error;
	}

	if (!OpenDiskImageDevice(unit)) {
		SNPrintf(error_buffer, sizeof(error_buffer),
			"failed to open diskimage.device");
		if (WBMsg)
			ErrorStringRequester(error_buffer);
		else
			FPrintf(stderr, "%s: %s\n", PROGNAME, error_buffer);
		goto error;
	}

	if (reloadplugins) {
		error = ReloadPlugins();
		if (error) {
			PrintFault(error, PROGNAME);
			goto error;
		}
	} else if (filename || eject) {
		error_buffer[0] = 0;
		if (filename) {
			LONG type = DITYPE_NONE;
			UnitControl(unit,
				DITAG_DiskImageType,		(IPTR)&type,
				DITAG_Filename,				(IPTR)NULL,
				TAG_END);
			if (type != DITYPE_NONE) {
				Delay(5);
			}
			error = UnitControl(unit,
				DITAG_Error,				(IPTR)&error2,
				DITAG_ErrorString,			(IPTR)error_buffer,
				DITAG_ErrorStringLength,	sizeof(error_buffer),
				DITAG_Password,				(IPTR)password,
				DITAG_Plugin,				(IPTR)plugin,
				DITAG_CurrentDir,			(IPTR)filedir,
				DITAG_Filename,				(IPTR)filename,
				DITAG_WriteProtect,			writeprotect,
				TAG_END);
		} else {
			error = UnitControl(unit,
				DITAG_Filename,				(IPTR)NULL,
				DITAG_WriteProtect,			writeprotect,
				TAG_END);
		}
		if (error == NO_ERROR) error = error2;
		if (error_buffer[0]) {
			if (WBMsg)
				ErrorStringRequester(error_buffer);
			else
				FPrintf(stderr, "%s: %s\n", filename, error_buffer);
			goto error;
		} else if (error) {
			if (WBMsg)
				IoErrRequester(error, NULL);
			else
				PrintFault(error, filename);
			goto error;
		}
	}

	rc = RETURN_OK;

error:
	CloseDiskImageDevice();
	if (WBMsg) {
		if (icon) FreeDiskObject(icon);
#ifndef __AROS__
		if (RequesterBase) CloseLibrary(RequesterBase);
		if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
		if (IconBase) CloseLibrary(IconBase);
#endif
	} else {
		FreeArgs(rdargs);
	}

	return rc;
}

#ifdef __AROS__

void IoErrRequester (LONG error, CONST_STRPTR header) {
	TEXT bodytext[80];
	if (error == NO_ERROR) {
		return;
	}
	Fault(error, (STRPTR)header, bodytext, sizeof(bodytext));
	MUI_Request(NULL, NULL, 0, "Error - "PROGNAME, "Ok", "%s", bodytext);
}

void ErrorStringRequester (CONST_STRPTR error_string) {
	MUI_Request(NULL, NULL, 0, "Error - "PROGNAME, "Ok", "%s", (STRPTR)error_string);
}

#else

LONG DoReq (Object *req) {
	LONG res = 0;
	if (req) {
		res = DoMethod(req, RM_OPENREQ, NULL, NULL, NULL);
		DisposeObject(req);
	}
	return res;
}

void IoErrRequester (LONG error, CONST_STRPTR header) {
	TEXT bodytext[80];
	Object *req;
	if (error == NO_ERROR) {
		return;
	}
	Fault(error, (STRPTR)header, bodytext, sizeof(bodytext));
	req = RequesterObject,
		REQ_TitleText,	"Error - "PROGNAME,
		REQ_BodyText,	bodytext,
		REQ_GadgetText,	"_Ok",
	End;
	DoReq(req);
}

void ErrorStringRequester (CONST_STRPTR error_string) {
	Object *req;
	req = RequesterObject,
		REQ_TitleText,	"Error - "PROGNAME,
		REQ_BodyText,	error_string,
		REQ_GadgetText,	"_Ok",
	End;
	DoReq(req);
}

#endif
