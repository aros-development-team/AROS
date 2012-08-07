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

#ifndef PREFS_H
#define PREFS_H

#ifndef EXEC_EXEC_H
#include <exec/exec.h>
#endif

typedef struct {
	struct MinNode node;
	APTR parent;
	STRPTR key;
	ULONG type;
	union {
		struct List *list;
		BOOL bool;
		LONG integer;
		STRPTR string;
	} value;
} PrefsObject;

enum {
	PREFS_UNKNOWN,
	PREFS_DICTIONARY,
	PREFS_BOOL,
	PREFS_INTEGER,
	PREFS_STRING
};

/* prefs.c */
PrefsObject *AllocPrefsDictionary (void);
PrefsObject *AllocPrefsBoolean (BOOL value);
PrefsObject *AllocPrefsInteger (LONG value);
PrefsObject *AllocPrefsString (CONST_STRPTR value);
void FreePrefsObject (PrefsObject *obj);
PrefsObject *DictGetObjectForKey (PrefsObject *dict, CONST_STRPTR key);
BOOL DictGetBooleanForKey (PrefsObject *dict, CONST_STRPTR key, BOOL def_val);
LONG DictGetIntegerForKey (PrefsObject *dict, CONST_STRPTR key, LONG def_val);
CONST_STRPTR DictGetStringForKey (PrefsObject *dict, CONST_STRPTR key, CONST_STRPTR def_val);
BOOL DictSetObjectForKey (PrefsObject *dict, PrefsObject *obj, CONST_STRPTR key);
BOOL DictRemoveObjForKey (PrefsObject *dict, CONST_STRPTR key);
void ClearPrefsDictionary (PrefsObject *dict);

/* readprefs.c */
BOOL ReadPrefs (PrefsObject *dict, CONST_STRPTR filename);

/* writeprefs.c */
BOOL WritePrefs (PrefsObject *dict, CONST_STRPTR filename);

#endif
