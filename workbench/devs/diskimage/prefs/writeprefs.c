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

#include "prefs.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include "support.h"

static void Indent (BPTR file, LONG level);
static void WritePrefsObject (PrefsObject *obj, BPTR file, LONG level);

BOOL WritePrefs (PrefsObject *dict, CONST_STRPTR filename) {
	BOOL res = FALSE;
	if (dict && dict->type == PREFS_DICTIONARY) {
		BPTR file;
		file = Open(filename, MODE_NEWFILE);
		if (file) {
			FPrintf(file, "<pobjects version=\"1.0\">\n");
			WritePrefsObject(dict, file, 0);
			FPrintf(file, "</pobjects>\n");
			res = TRUE;
			Close(file);
		}
	}
	return res;
}

static void Indent (BPTR file, LONG level) {
	while (level--) FPutC(file, '\t');
}

static void WritePrefsObject (PrefsObject *obj, BPTR file, LONG level) {
	PrefsObject *child;
	if (obj) {
		switch (obj->type) {
			case PREFS_DICTIONARY:
				Indent(file, level);
				FPrintf(file, "<dict>\n");
				child = (PrefsObject *)GetHead(obj->value.list);
				while (child) {
					Indent(file, level + 1);
					FPrintf(file, "<key>%s</key>\n", child->key);
					WritePrefsObject(child, file, level + 1);
					child = (PrefsObject *)GetSucc((struct Node *)child);
				}
				Indent(file, level);
				FPrintf(file, "</dict>\n");
				break;
			case PREFS_BOOL:
				Indent(file, level);
				FPrintf(file, "<bool>%s</bool>\n", obj->value.bool ? "TRUE" : "FALSE");
				break;
			case PREFS_INTEGER:
				Indent(file, level);
				FPrintf(file, "<integer>%ld</integer>\n", obj->value.integer);
				break;
			case PREFS_STRING:
				Indent(file, level);
				FPrintf(file, "<string>%s</string>\n", obj->value.string);
				break;
		}
	}
}
