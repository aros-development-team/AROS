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
#include <proto/utility.h>
#include <proto/intuition.h>

extern struct ClassLibrary *PNGBase;

#ifndef BITMAP_DisabledSourceFile
#define BITMAP_DisabledSourceFile (BITMAP_Dummy + 19)
#endif

#define PNGObject NewObject(PNGBase->cl_Class, NULL

static BPTR FileExists (CONST_STRPTR dir, CONST_STRPTR file, STRPTR path, LONG path_size) {
	struct Process *proc = (struct Process *)FindTask(NULL);
	APTR window = proc->pr_WindowPtr;
	BPTR fh;
	Strlcpy(path, dir, path_size);
	AddPart(path, file, path_size);
	proc->pr_WindowPtr = (APTR)~0;
	fh = Open(path, MODE_OLDFILE);
	proc->pr_WindowPtr = window;
	if (fh) Close(fh);
	return fh;
}

BOOL FindImage (CONST_STRPTR image, STRPTR path, LONG path_size) {
	return FileExists("PROGDIR:", image, path, path_size)
		|| FileExists("PROGDIR:Images", image, path, path_size)
		|| FileExists("SYS:Prefs/Presets/Images", image, path, path_size)
		|| FileExists("TBImages:", image, path, path_size);
}

Object *LoadImage (struct Screen *screen, CONST_STRPTR image, BOOL selected, BOOL disabled) {
	TEXT normal_path[64];
	TEXT selected_path[64];
	TEXT disabled_path[64];
	if (!FindImage(image, normal_path, sizeof(normal_path))) {
		return NULL;
	}
	Strlcpy(selected_path, normal_path, sizeof(selected_path));
	Strlcat(selected_path, "_s", sizeof(selected_path));
	Strlcpy(disabled_path, normal_path, sizeof(disabled_path));
	Strlcat(disabled_path, "_g", sizeof(disabled_path));
	if (PNGBase) {
		Object *image = PNGObject,
			BITMAP_Screen,				screen,
			BITMAP_SourceFile,			normal_path,
			selected ? BITMAP_SelectSourceFile : TAG_IGNORE, selected_path,
			disabled ? BITMAP_DisabledSourceFile : TAG_IGNORE, disabled_path,
		End;
		if (image) return image;
	}
	return BitMapObject,
		BITMAP_Screen,				screen,
		BITMAP_SourceFile,			normal_path,
		selected ? BITMAP_SelectSourceFile : TAG_IGNORE, selected_path,
		disabled ? BITMAP_DisabledSourceFile : TAG_IGNORE, disabled_path,
		BITMAP_Masking,				TRUE,
	End;
}
