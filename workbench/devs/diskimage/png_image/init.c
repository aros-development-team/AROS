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

#include "class.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/z.h>
#include "support.h"
#include "png.image_rev.h"

#define LIBNAME "png.image"
const char USED verstag[] = VERSTAG;

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct UtilityBase *UtilityBase;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *LayersBase;
struct Library *P96Base;
struct Library *CyberGfxBase;
struct Library *ZBase;

struct ClassBase *libInit (REG(d0, struct ClassBase *libBase), REG(a0, BPTR seglist),
	REG(a6, struct ExecBase *exec_base));
struct ClassBase *libOpen (REG(a6, struct ClassBase *libBase), REG(d0, ULONG version));
BPTR libClose (REG(a6, struct ClassBase *libBase));
BPTR libExpunge (REG(a6, struct ClassBase *libBase));
APTR libReserved (REG(a6, struct ClassBase *libBase));
BOOL OpenLibs (void);
void CloseLibs (void);
Class *GetClass (REG(a6, struct ClassBase *libBase));

struct InitTable {
	ULONG it_Size;
	APTR it_FunctionTable;
	APTR it_DataTable;
	APTR it_InitRoutine;
};

CONST_APTR function_table[] = {
	(APTR)libOpen,
	(APTR)libClose,
	(APTR)libExpunge,
	(APTR)libReserved,
	(APTR)GetClass,
	(APTR)-1
};

CONST struct InitTable init_table = {
	sizeof(struct ClassBase),
	(APTR)function_table,
	NULL,
	(APTR)libInit
};

CONST struct Resident USED lib_res = {
	RTC_MATCHWORD,
	(struct Resident *)&lib_res,
	(APTR)(&lib_res + 1),
	RTF_AUTOINIT,
	VERSION,
	NT_LIBRARY,
	0,
	LIBNAME,
	VSTRING,
	(APTR)&init_table
};

struct ClassBase *libInit (REG(d0, struct ClassBase *libBase), REG(a0, BPTR seglist),
	REG(a6, struct ExecBase *exec_base))
{
	libBase->libNode.lib_Node.ln_Type = NT_LIBRARY;
	libBase->libNode.lib_Node.ln_Pri  = 0;
	libBase->libNode.lib_Node.ln_Name = LIBNAME;
	libBase->libNode.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
	libBase->libNode.lib_Version      = VERSION;
	libBase->libNode.lib_Revision     = REVISION;
	libBase->libNode.lib_IdString     = VSTRING;

	SysBase = exec_base;
	libBase->seglist = seglist;

	if (OpenLibs()) {
		libBase->class = MakeClass(LIBNAME, "imageclass", NULL, sizeof(struct ClassData), 0); 
		if (libBase->class) {
			libBase->class->cl_Dispatcher.h_Entry = (HOOKFUNC)ClassDispatch;
			AddClass(libBase->class);
			return libBase;
		}
	}
	CloseLibs();
	DeleteLibrary((struct Library *)libBase);

	return NULL;
}

struct ClassBase *libOpen (REG(a6, struct ClassBase *libBase), REG(d0, ULONG version)) {
	libBase->libNode.lib_OpenCnt++;
	libBase->libNode.lib_Flags &= ~LIBF_DELEXP;
	return libBase;
}

BPTR libClose (REG(a6, struct ClassBase *libBase)) {
	libBase->libNode.lib_OpenCnt--;
	return 0;
}

BPTR libExpunge (REG(a6, struct ClassBase *libBase)) {
	BPTR result = 0;

	if (libBase->libNode.lib_OpenCnt > 0) {
		libBase->libNode.lib_Flags |= LIBF_DELEXP;
		return 0;
	}

	Remove(&libBase->libNode.lib_Node);

	FreeClass(libBase->class);
	CloseLibs();
	
	result = libBase->seglist;

	DeleteLibrary((struct Library *)libBase);

	return result;
}

APTR libReserved (REG(a6, struct ClassBase *libBase)) {
	return NULL;
}

BOOL OpenLibs (void) {
	return (DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 39)) &&
		(UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 39)) &&
		(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)) &&
		(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)) &&
		(LayersBase = OpenLibrary("layers.library", 39)) &&
		((P96Base = OpenLibrary("Picasso96API.library", 0)) ||
		(CyberGfxBase = OpenLibrary("cybergraphics.library", 0)) || 1) &&
		(ZBase = OpenLibrary("z.library", 1));
}

void CloseLibs (void) {
	if (ZBase) CloseLibrary(ZBase);
	if (CyberGfxBase) CloseLibrary(CyberGfxBase);
	if (P96Base) CloseLibrary(P96Base);
	if (LayersBase) CloseLibrary(LayersBase);
	if (GfxBase) CloseLibrary((struct Library *)GfxBase);
	if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
	if (UtilityBase) CloseLibrary((struct Library *)UtilityBase);
	if (DOSBase) CloseLibrary((struct Library *)DOSBase);
}

Class *GetClass (REG(a6, struct ClassBase *libBase)) {
	return libBase->class;
}
