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

LONG CreateTempFile (APTR Self, struct DiskImageUnit *unit, CONST_STRPTR ext,
	BPTR *tmpdir, CONST_STRPTR *tmpname)
{
	struct DiskImageBase *libBase = unit->LibBase;
	struct Library *SysBase = libBase->SysBase;
	struct Library *DOSBase = libBase->DOSBase;
	TEXT buf[4];
	STRPTR path;
	LONG error;

	*tmpdir = ZERO;
	*tmpname = "";

	if (unit->TempDir) return ERROR_OBJECT_IN_USE;

	path = NULL;
	if (GetVar(TEMPDIR_VAR, buf, 4, LV_VAR|GVF_GLOBAL_ONLY) != -1) {
		LONG size;
		size = IoErr();
		path = AllocVec(size+2, 0);
		if (path) {
			if (GetVar(TEMPDIR_VAR, path, size+1, LV_VAR|GVF_GLOBAL_ONLY) == -1) {
				path[0] = '\0';
			}
		}
	}

	if (path && path[0])
		unit->TempDir = Lock(path, ACCESS_READ);
	else
		unit->TempDir = 0;
	FreeVec(path);

	if (!unit->TempDir)
		unit->TempDir = Lock("T:", ACCESS_READ);

	if (unit->TempDir) {
		if (!ext) ext = "img";
		unit->TempName = ASPrintf("unit_%ld.%s", unit->UnitNum, ext);
		if (unit->TempName) {
			*tmpdir = unit->TempDir;
			*tmpname = unit->TempName;
			error = NO_ERROR;
		} else
			error = ERROR_NO_FREE_STORE;
	} else
		error = IoErr();

	return error;
}

BPTR OpenTempFile (APTR Self, struct DiskImageUnit *unit, ULONG mode) {
	struct Library *DOSBase = unit->LibBase->DOSBase;
	BPTR old, file;
	old = CurrentDir(unit->TempDir);
	file = Open(unit->TempName, mode);
	CurrentDir(old);
	return file;
}

void RemoveTempFile (APTR Self, struct DiskImageUnit *unit) {
	struct DiskImageBase *libBase = unit->LibBase;
	struct Library *SysBase = libBase->SysBase;
	struct Library *DOSBase = libBase->DOSBase;
	if (unit->TempDir && unit->TempName) {
		BPTR old;
		old = CurrentDir(unit->TempDir);
		DeleteFile(unit->TempName);
		CurrentDir(old);
	}
	UnLock(unit->TempDir);
	FreeVec(unit->TempName);
	unit->TempDir = 0;
	unit->TempName = NULL;
}
