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

#include "diskimage_device.h"
#include "device_locale.h"

#ifndef __AROS__
#include <classes/requester.h>
#include <proto/requester.h>

STRPTR RequestPassword (APTR Self, struct DiskImageUnit *unit) {
	struct DiskImageBase *libBase = unit->LibBase;
	struct Screen *scr = unit->Screen;
	struct Library *SysBase = libBase->SysBase;
	struct Library *IntuitionBase = libBase->IntuitionBase;
	struct LocaleInfo *li = &libBase->LocaleInfo;
	struct Library *RequesterBase;
	STRPTR passwd = NULL;

	passwd = AllocVec(128, MEMF_CLEAR);
	if (!passwd) {
		return NULL;
	}
	if (unit->Password) {
		Strlcpy(passwd, unit->Password, 128);
		unit->Password = NULL;
		return passwd;
	}

	RequesterBase = OpenLibrary("requester.class", 52);
	if (RequesterBase) {
		Object *req;
		req = NewObject(REQUESTER_GetClass(), NULL,
			REQ_Type,			REQTYPE_STRING,
			/*REQ_Image,			REQIMAGE_QUESTION,*/
			REQ_TitleText,		unit->Node.ln_Name,
			REQ_BodyText,		GetString(li, MSG_PASSWORD_REQ),
			REQ_GadgetText,		GetString(li, MSG_OK_GAD),
			REQS_AllowEmpty,	TRUE,
			REQS_Invisible,		TRUE,
			REQS_Buffer,		passwd,
			REQS_MaxChars,		127,
			TAG_END);
		if (req) {
			DoMethod(req, RM_OPENREQ, NULL, NULL, scr);
			DisposeObject(req);
		}
		CloseLibrary(RequesterBase);
	}

	return passwd;
}

#else

STRPTR RequestPassword (APTR Self, struct DiskImageUnit *unit) {
	STRPTR passwd = NULL;

	passwd = AllocVec(128, MEMF_CLEAR);
	if (!passwd) {
		return NULL;
	}
	if (unit->Password) {
		Strlcpy(passwd, unit->Password, 128);
		unit->Password = NULL;
		return passwd;
	}

	return passwd;
}

#endif
