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

#include "support.h"
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

#define VAR_FLAGS LV_VAR|GVF_GLOBAL_ONLY

STRPTR GetEnvVar (CONST_STRPTR name) {
	TEXT buf[4];
	if (GetVar(name, buf, sizeof(buf), VAR_FLAGS) != -1) {
		LONG size;
		STRPTR contents;
		size = IoErr();
		contents = AllocVec(size+2, MEMF_ANY);
		if (contents) {
			contents[size+1] = '\0';
			if (GetVar(name, contents, size+1, VAR_FLAGS) != -1) {
				return contents;
			}
			FreeVec(contents);
		}
	}
	return NULL;
}

BOOL SetEnvVar (CONST_STRPTR name, CONST_STRPTR contents, BOOL envarc) {
	if (contents && contents[0]) {
		ULONG flags = VAR_FLAGS;
		if (envarc) flags |= GVF_SAVE_VAR;
		return SetVar(name, contents, strlen(contents), flags);
	} else {
		return DeleteVar(name, VAR_FLAGS);
	}
}
