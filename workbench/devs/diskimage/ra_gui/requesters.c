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

#include "diskimagegui.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/diskimage.h>
#include <clib/alib_protos.h>
#include "rev/DiskImageGUI_rev.h"

LONG DoReq (Object *req) {
	LONG res = 0;
	if (req) {
		struct Window *window;

		StopScreenNotify();
		SetWindowBusy(~0, TRUE);
		window = NULL;
		if (Gui.windows[WID_MAIN]) {
			GetAttr(WINDOW_Window, Gui.windows[WID_MAIN], (Tag *)&window);
		}

		if (Gui.screen) {
			res = DoMethod(req, RM_OPENREQ, NULL, window, Gui.screen);
		} else {
			struct Screen *screen;
			screen = LockPubScreen(NULL);
			if (screen) {
				res = DoMethod(req, RM_OPENREQ, NULL, NULL, screen);
				UnlockPubScreen(NULL, screen);
			}
		}

		SetWindowBusy(~0, FALSE);
		BeginScreenNotify();

		DisposeObject(req);
	}
	return res;
}

#ifndef FALLBACK_IMAGES
void ImageNotFoundRequester (CONST_STRPTR image) {
	TEXT titletext[40];
	TEXT bodytext[80];
	CONST_STRPTR gadgettext;
	Object *req;
	
	SNPrintf(titletext, sizeof(titletext), GetString(&LocaleInfo, MSG_ERROR_WND), PROGNAME);
	SNPrintf(bodytext, sizeof(bodytext), GetString(&LocaleInfo, MSG_IMAGENOTFOUND_REQ), image);
	gadgettext = GetString(&LocaleInfo, MSG_OK_GAD);

	req = RequesterObject,
		REQ_TitleText,	titletext,
		REQ_BodyText,	bodytext,
		REQ_GadgetText,	gadgettext,
	End;
	
	DoReq(req);
}
#endif

void AboutRequester (void) {
	TEXT titletext[40];
	STRPTR bodytext;
	CONST_STRPTR gadgettext;
	Object *req;

	SNPrintf(titletext, sizeof(titletext), GetString(&LocaleInfo, MSG_ABOUT_WND), PROGNAME);
	bodytext = ASPrintf(GetString(&LocaleInfo, MSG_ABOUT_REQ),
		DiskImageBase->lib_Node.ln_Name, DiskImageBase->lib_Version, DiskImageBase->lib_Revision,
		PROGNAME, VERSION, REVISION);
	gadgettext = GetString(&LocaleInfo, MSG_OK_GAD);
	if (!bodytext) {
		IoErrRequester(ERROR_NO_FREE_STORE);
		return;
	}

	req = RequesterObject,
		REQ_TitleText,	titletext,
		REQ_BodyText,	bodytext,
		REQ_GadgetText,	gadgettext,
	End;

	DoReq(req);
	
	FreeVec(bodytext);
}

void IoErrRequester (LONG error) {
	TEXT titletext[40];
	TEXT bodytext[80];
	CONST_STRPTR gadgettext;
	Object *req;

	if (error == NO_ERROR) {
		return;
	}

	SNPrintf(titletext, sizeof(titletext), GetString(&LocaleInfo, MSG_ERROR_WND), PROGNAME);
	Fault(error, NULL, bodytext, sizeof(bodytext));
	gadgettext = GetString(&LocaleInfo, MSG_OK_GAD);

	req = RequesterObject,
		REQ_TitleText,	titletext,
		REQ_BodyText,	bodytext,
		REQ_GadgetText,	gadgettext,
	End;

	DoReq(req);
}

void ErrorStringRequester (CONST_STRPTR error_string) {
	TEXT titletext[40];
	CONST_STRPTR bodytext;
	CONST_STRPTR gadgettext;
	Object *req;

	SNPrintf(titletext, sizeof(titletext), GetString(&LocaleInfo, MSG_ERROR_WND), PROGNAME);
	bodytext = error_string;
	gadgettext = GetString(&LocaleInfo, MSG_OK_GAD);

	req = RequesterObject,
		REQ_TitleText,	titletext,
		REQ_BodyText,	bodytext,
		REQ_GadgetText,	gadgettext,
	End;

	DoReq(req);
}

void RequiredVersionRequester (CONST_STRPTR res_name, LONG version, LONG revision) {
	TEXT titletext[40];
	TEXT bodytext[80];
	CONST_STRPTR gadgettext;
	Object *req;
	
	SNPrintf(titletext, sizeof(titletext), GetString(&LocaleInfo, MSG_ERROR_WND), PROGNAME);
	SNPrintf(bodytext, sizeof(bodytext), GetString(&LocaleInfo, MSG_REQUIREDVERSION_REQ),
		res_name, version, revision);
	gadgettext = GetString(&LocaleInfo, MSG_OK_GAD);

	req = RequesterObject,
		REQ_TitleText,	titletext,
		REQ_BodyText,	bodytext,
		REQ_GadgetText,	gadgettext,
	End;

	DoReq(req);
}
