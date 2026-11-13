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
#include <proto/intuition.h>

static void GetWindowSize (Object *window, LONG *w, LONG *h) {
	struct Window *winptr = NULL;
	GetAttr(WINDOW_Window, window, (Tag *)&winptr);
	if (winptr) {
		*w = winptr->Width;
		*h = winptr->Height;
	}
}

void RestoreWindowSize (Object *window, CONST_STRPTR name) {
	PrefsObject *window_prefs;
	LONG w, h;
	if (!window) {
		return;
	}
	window_prefs = DictGetObjectForKey(ApplicationPrefs, name);
	w = DictGetIntegerForKey(window_prefs, "width", 320);
	h = DictGetIntegerForKey(window_prefs, "height", 200);
	if (w && h) {
		SetAttrs(window, WA_Width, w, WA_Height, h, TAG_END);
	}
}

void SaveWindowSize (Object *window, CONST_STRPTR name, BOOL envarc) {
	LONG w, h;
	if (!window) {
		return;
	}
	w = h = 0;
	GetWindowSize(window, &w, &h);
	if (w && h) {
		PrefsObject *env_prefs = ApplicationPrefs;
		PrefsObject *window_prefs;
		window_prefs = AllocPrefsDictionary();
		if (!DictSetObjectForKey(window_prefs, AllocPrefsInteger(w), "width") ||
			!DictSetObjectForKey(window_prefs, AllocPrefsInteger(h), "height"))
		{
			FreePrefsObject(window_prefs);
			return;
		}
		if (!DictSetObjectForKey(env_prefs, window_prefs, name)) {
			return;
		}
		WritePrefs(env_prefs, "ENV:"PROGNAME".a500.org.xml");
		if (envarc) {
			PrefsObject *envarc_prefs;
			envarc_prefs = AllocPrefsDictionary();
			if (!envarc_prefs) {
				return;
			}
			ReadPrefs(envarc_prefs, "ENVARC:"PROGNAME".a500.org.xml");
			window_prefs = AllocPrefsDictionary();
			if (!DictSetObjectForKey(window_prefs, AllocPrefsInteger(w), "width") ||
				!DictSetObjectForKey(window_prefs, AllocPrefsInteger(h), "height"))
			{
				FreePrefsObject(window_prefs);
				return;
			}
			if (!DictSetObjectForKey(envarc_prefs, window_prefs, name)) {
				return;
			}
			WritePrefs(envarc_prefs, "ENVARC:"PROGNAME".a500.org.xml");
			FreePrefsObject(envarc_prefs);
		}
	}
}
