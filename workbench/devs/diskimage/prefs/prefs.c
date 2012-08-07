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
#include <string.h>
#include "support.h"

PrefsObject *AllocPrefsDictionary (void) {
	PrefsObject *obj;
	obj = AllocVec(sizeof(PrefsObject), MEMF_CLEAR);
	if (obj) {
		obj->type = PREFS_DICTIONARY;
		obj->value.list = CreateList(TRUE);
		if (obj->value.list) {
			return obj;
		}
		FreePrefsObject(obj);
	}
	return NULL;
}

PrefsObject *AllocPrefsBoolean (BOOL value) {
	PrefsObject *obj;
	obj = AllocVec(sizeof(PrefsObject), MEMF_CLEAR);
	if (obj) {
		obj->type = PREFS_BOOL;
		obj->value.bool = value;
		return obj;
	}
	return NULL;
}

PrefsObject *AllocPrefsInteger (LONG value) {
	PrefsObject *obj;
	obj = AllocVec(sizeof(PrefsObject), MEMF_CLEAR);
	if (obj) {
		obj->type = PREFS_INTEGER;
		obj->value.integer = value;
		return obj;
	}
	return NULL;
}

PrefsObject *AllocPrefsString (CONST_STRPTR value) {
	PrefsObject *obj;
	obj = AllocVec(sizeof(PrefsObject), MEMF_CLEAR);
	if (obj) {
		obj->type = PREFS_STRING;
		obj->value.string = ASPrintf("%s", value);
		if (obj->value.string) {
			return obj;
		}
		FreePrefsObject(obj);
	}
	return NULL;
}

void FreePrefsObject (PrefsObject *obj) {
	if (obj) {
		FreeVec(obj->key);
		switch (obj->type) {
			case PREFS_DICTIONARY:
				if (obj->value.list) {
					PrefsObject *child;
					while ((child = (PrefsObject *)RemHead(obj->value.list))) {
						FreePrefsObject(child);
					}
					DeleteList(obj->value.list);
				}
				break;
			case PREFS_STRING:
				FreeVec(obj->value.string);
				break;
		}
		FreeVec(obj);
	}
}

PrefsObject *DictGetObjectForKey (PrefsObject *dict, CONST_STRPTR key) {
	if (dict && dict->type == PREFS_DICTIONARY && key) {
		PrefsObject *child;
		child = (PrefsObject *)GetHead(dict->value.list);
		while (child) {
			if (!strcmp(child->key, key)) {
				return child;
			}
			child = (PrefsObject *)GetSucc((struct Node *)child);
		}
	}
	return NULL;
}

BOOL DictGetBooleanForKey (PrefsObject *dict, CONST_STRPTR key, BOOL def_val) {
	PrefsObject *child;
	child = DictGetObjectForKey(dict, key);
	if (child && child->type == PREFS_BOOL) {
		return child->value.bool;
	} else {
		return def_val;
	}
}

LONG DictGetIntegerForKey (PrefsObject *dict, CONST_STRPTR key, LONG def_val) {
	PrefsObject *child;
	child = DictGetObjectForKey(dict, key);
	if (child && child->type == PREFS_INTEGER) {
		return child->value.integer;
	} else {
		return def_val;
	}
}

CONST_STRPTR DictGetStringForKey (PrefsObject *dict, CONST_STRPTR key, CONST_STRPTR def_val) {
	PrefsObject *child;
	child = DictGetObjectForKey(dict, key);
	if (child && child->type == PREFS_STRING) {
		return child->value.string;
	} else {
		return def_val;
	}
}

BOOL DictSetObjectForKey (PrefsObject *dict, PrefsObject *obj, CONST_STRPTR key) {
	if (dict && dict->type == PREFS_DICTIONARY && obj && key) {
		obj->parent = dict;
		obj->key = ASPrintf("%s", key);
		if (obj->key) {
			DictRemoveObjForKey(dict, key);
			AddTail(dict->value.list, (struct Node *)obj);
			return TRUE;
		} else {
			FreePrefsObject(obj);
			return FALSE;
		}
	} else {
		FreePrefsObject(obj);
		return FALSE;
	}
}

BOOL DictRemoveObjForKey (PrefsObject *dict, CONST_STRPTR key) {
	PrefsObject *child;
	child = DictGetObjectForKey(dict, key);
	if (child) {
		Remove((struct Node *)child);
		FreePrefsObject(child);
		return TRUE;
	} else {
		return FALSE;
	}
}

void ClearPrefsDictionary (PrefsObject *dict) {
	if (dict && dict->type == PREFS_DICTIONARY) {
		PrefsObject *child;
		while ((child = (PrefsObject *)RemHead(dict->value.list))) {
			FreePrefsObject(child);
		}
	}
}
