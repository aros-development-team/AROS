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

#ifdef __AROS__
#  include <expat.h>
#  include <string.h>
#else
#  include <libraries/expat.h>
#  include <proto/expat.h>
#endif

#include <proto/exec.h>
#include <proto/dos.h>
#include <stdlib.h>

#define PREFS_BUFFER_SIZE 512

typedef struct {
	LONG state;
	PrefsObject *dict;
	BOOL key_tag, value_tag;
	BOOL has_key;
	TEXT key[64];
	TEXT value[512];
	struct Library *expatbase;
	XML_Parser parser;
	BOOL error;
} parser_data;

static void start_element_handler (void *user_data, const char *name, const char **attrs);
static void end_element_handler (void *user_data, const char *name);
static void character_data_handler (void *user_data, const char *s, int len);

BOOL ReadPrefs (PrefsObject *dict, CONST_STRPTR filename) {
	struct Library *ExpatBase;
	BOOL res = FALSE;
#ifdef __AROS__
	ExpatBase = OpenLibrary("expat_au.library", 0);
#else
	ExpatBase = OpenLibrary("expat.library", 4);
#endif
	if (!ExpatBase) {
		return res;
	}
	if (dict && dict->type == PREFS_DICTIONARY) {
		BPTR file;
		ClearPrefsDictionary(dict);
		file = Open(filename, MODE_OLDFILE);
		if (file) {
			XML_Parser parser;
			parser = XML_ParserCreate(NULL);
			if (parser) {
				STRPTR buffer;
				parser_data *data;
				buffer = AllocVec(PREFS_BUFFER_SIZE, MEMF_ANY);
				data = AllocVec(sizeof(parser_data), MEMF_ANY);
				if (buffer && data) {
					LONG len;
					BOOL eof;
					data->state = 0;
					data->dict = dict;
					data->key_tag = FALSE;
					data->value_tag = FALSE;
					data->has_key = FALSE;
					data->expatbase = ExpatBase;
					data->parser = parser;
					data->error = FALSE;
					XML_SetUserData(parser, data);
					XML_SetElementHandler(parser, start_element_handler, end_element_handler);
					XML_SetCharacterDataHandler(parser, character_data_handler);
					while (TRUE) {
						len = Read(file, buffer, PREFS_BUFFER_SIZE);
						if (len == -1) {
							break;
						}
						eof = (len == 0);
						if (!XML_Parse(parser, buffer, len, eof)) {
							break;
						}
						if (data->error) {
							break;
						}
						if (eof) {
							res = TRUE;
							break;
						}
					}
				}
				FreeVec(buffer);
				FreeVec(data);
				XML_ParserFree(parser);
			}
			Close(file);
		}
	}
	CloseLibrary(ExpatBase);
	return res;
}

static void stop_parser (parser_data *data) {
	struct Library *ExpatBase = data->expatbase;
	data->error = TRUE;
	XML_StopParser(data->parser, XML_TRUE);
}

static void start_element_handler (void *user_data, const char *name, const char **attrs) {
	parser_data *data = user_data;
	if (data->error) {
		return;
	}
	switch (data->state) {
		case 0:
			if (!strcmp(name, "pobjects")) {
				data->state++;
			} else {
				stop_parser(data);
				return;
			}
			break;
		case 1:
			if (!strcmp(name, "dict")) {
				data->state++;
			} else {
				stop_parser(data);
				return;
			}
			break;
		case 2:
			if (data->key_tag || data->value_tag) {
				stop_parser(data);
				return;
			}
			if (!data->has_key && !strcmp(name, "key")) {
				data->key_tag = TRUE;
				data->key[0] = 0;
			} else if (data->has_key &&
				(!strcmp(name, "dict") ||
				!strcmp(name, "bool") ||
				!strcmp(name, "integer") ||
				!strcmp(name, "string")))
			{
				if (!strcmp(name, "dict")) {
					PrefsObject *obj;
					obj = AllocPrefsDictionary();
					if (DictSetObjectForKey(data->dict, obj, data->key)) {
						data->dict = obj;
						data->has_key = FALSE;
					} else {
						stop_parser(data);
						return;
					}
				} else {
					data->value_tag = TRUE;
					data->value[0] = 0;
				}
			} else {
				stop_parser(data);
				return;
			}
			break;
		default:
			stop_parser(data);
			return;
	}
}

static void end_element_handler (void *user_data, const char *name) {
	parser_data *data = user_data;
	if (data->error) {
		return;
	}
	switch (data->state) {
		case 2:
			if (!data->has_key && !strcmp(name, "dict")) {
				data->dict = data->dict->parent;
				if (!data->dict) {
					data->state++;
				}
			} else if (data->key_tag && !strcmp(name, "key")) {
				data->has_key = (data->key[0] != 0);
				data->key_tag = FALSE;
			} else if (data->has_key && data->value_tag &&
				(!strcmp(name, "bool") ||
				!strcmp(name, "integer") ||
				!strcmp(name, "string")))
			{
				PrefsObject *obj;
				if (!strcmp(name, "bool")) {
					obj = AllocPrefsBoolean(!strcmp(data->value, "TRUE"));
				} else if (!strcmp(name, "integer")) {
					obj = AllocPrefsInteger(atol(data->value));
				} else if (!strcmp(name, "string")) {
					obj = AllocPrefsString(data->value);
				} else {
					obj = NULL;
				}
				if (DictSetObjectForKey(data->dict, obj, data->key)) {
					data->has_key = FALSE;
					data->value_tag = FALSE;
				} else {
					stop_parser(data);
					return;
				}
			} else {
				stop_parser(data);
				return;
			}
			break;
		case 3:
			if (!strcmp(name, "pobjects")) {
				data->state++;
			} else {
				stop_parser(data);
				return;
			}
			break;
		default:
			stop_parser(data);
			return;
	}
}

static void append_text (char *dest, int dest_len, const char *src, int src_len) {
	int offs = strlen(dest);
	int space = dest_len - offs;
	int len;
	if (space > src_len) {
		len = src_len;
	} else {
		len = space - 1;
	}
	if (len > 0) {
		memcpy(&dest[offs], src, len);
		dest[offs + len] = 0;
	}
}

static void character_data_handler (void *user_data, const char *s, int len) {
	parser_data *data = user_data;
	if (data->error) {
		return;
	}
	if (data->key_tag) {
		append_text(data->key, sizeof(data->key), s, len);
	} else if (data->value_tag) {
		append_text(data->value, sizeof(data->value), s, len);
	}
}
