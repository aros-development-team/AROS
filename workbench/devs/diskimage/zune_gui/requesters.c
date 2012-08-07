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
#include <proto/dos.h>
#include <proto/muimaster.h>

void ImageNotFoundRequester (CONST_STRPTR image) {
	TEXT titletext[40];
	STRPTR bodytext;
	STRPTR gadgettext;

	SNPrintf(titletext, sizeof(titletext), GetString(&LocaleInfo, MSG_ERROR_WND), PROGNAME);
	bodytext = (STRPTR)GetString(&LocaleInfo, MSG_IMAGENOTFOUND_REQ);
	gadgettext = (STRPTR)GetString(&LocaleInfo, MSG_OK_GAD);

	SetWindowBusy(~0, TRUE);
	MUI_Request(Gui.app, Gui.wnd[WID_MAIN], 0, titletext, gadgettext, bodytext, (STRPTR)image);
	SetWindowBusy(~0, FALSE);
}

void IoErrRequester (LONG error) {
	TEXT titletext[40];
	TEXT bodytext[80];
	STRPTR gadgettext;

	if (error == NO_ERROR) {
		return;
	}
	
	SNPrintf(titletext, sizeof(titletext), GetString(&LocaleInfo, MSG_ERROR_WND), PROGNAME);
	Fault(error, NULL, bodytext, sizeof(bodytext));
	gadgettext = (STRPTR)GetString(&LocaleInfo, MSG_OK_GAD);
	
	SetWindowBusy(~0, TRUE);
	MUI_Request(Gui.app, Gui.wnd[WID_MAIN], 0, titletext, gadgettext, "%s", bodytext);
	SetWindowBusy(~0, FALSE);
}

void ErrorStringRequester (CONST_STRPTR error_string) {
	TEXT titletext[40];
	STRPTR bodytext;
	STRPTR gadgettext;
	
	SNPrintf(titletext, sizeof(titletext), GetString(&LocaleInfo, MSG_ERROR_WND), PROGNAME);
	bodytext = (STRPTR)error_string;
	gadgettext = (STRPTR)GetString(&LocaleInfo, MSG_OK_GAD);
	
	SetWindowBusy(~0, TRUE);
	MUI_Request(Gui.app, Gui.wnd[WID_MAIN], 0, titletext, gadgettext, "%s", bodytext);
	SetWindowBusy(~0, FALSE);
}
