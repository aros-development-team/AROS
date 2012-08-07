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

static BPTR FileExists (CONST_STRPTR dir, CONST_STRPTR file, STRPTR path, int32 path_size) {
	BPTR fh;
	Strlcpy(path, dir, path_size);
	AddPart(path, file, path_size);
	fh = Open(path, MODE_OLDFILE);
	if (fh) Close(fh);
	return fh;
}

BOOL FindImage (CONST_STRPTR image, STRPTR path, int32 path_size) {
	return FileExists("PROGDIR:", image, path, path_size)
		|| FileExists("PROGDIR:Images/digfx", image, path, path_size)
            || FileExists("IMAGES:digfx", image, path, path_size);
}

Object *LoadImage (CONST_STRPTR image, const struct TagItem *tags) {
	TEXT path[64];
	if (!FindImage(image, path, sizeof(path))) {
		return NULL;
	}
	return DtpicObject,
		MUIA_Dtpic_Name,			path,
		tags ? TAG_MORE : TAG_END,	tags,
	End;
}

Object *MakeImageButton (CONST_STRPTR image, CONST_STRPTR help, BOOL disabled) {
	Object *button;
	struct TagItem tags[] = {
		{ MUIA_Frame,     MUIV_Frame_ImageButton   },
		{ MUIA_InputMode, MUIV_InputMode_RelVerify },
		{ MUIA_ShortHelp, (IPTR)help               },
		{ MUIA_Disabled,  disabled                 },
		{ TAG_END,        0                        }
	};
	button = LoadImage(image, tags);
	if (!button) {
		ImageNotFoundRequester(image);
	}
	return button;
}
