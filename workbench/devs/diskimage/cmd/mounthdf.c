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

#include <exec/exec.h>
#include <dos/dos.h>
#include <workbench/startup.h>
#include <libraries/expansion.h>
#include <devices/diskimage.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/expansion.h>
#include <proto/diskimage.h>
#ifdef __AROS__
#include <libraries/mui.h>
#include <proto/muimaster.h>
#else
#include <reaction/reaction_macros.h>
#include <classes/requester.h>
#include <proto/intuition.h>
#include <proto/requester.h>
#include <clib/alib_protos.h>
#endif
#include "support.h"
#include "endian.h"
#include "rev/MountHDF_rev.h"

CONST TEXT USED verstag[] = VERSTAG;

#define PROGNAME "MountHDF"
#define TEMPLATE "U=UNIT/N/A,HDF=HARDFILE/A,WP=WRITEPROTECT/S," \
	"TRKSIZ=BLOCKSPERTRACK/N,HEADS=SIDES=SURFACES/N,BLKSIZ=BLOCKSIZE/N," \
	"RESERVED=BOOTBLOCKS/N"

enum {
	ARG_UNIT,
	ARG_HDF,
	ARG_WRITEPROTECT,
	ARG_BLOCKSPERTRACK,
	ARG_SURFACES,
	ARG_BLOCKSIZE,
	ARG_RESERVED,
	MAX_ARGS
};

struct WBStartup *WBMsg;
#ifndef __AROS__
struct Library *IconBase;
struct IntuitionBase *IntuitionBase;
struct Library *RequesterBase;
#endif
extern struct IOStdReq *DiskImageIO;

LONG DoReq (Object *req);
void IoErrRequester (LONG error, CONST_STRPTR header);
void ErrorStringRequester (CONST_STRPTR error_string);

int main (int argc, char **argv) {
	int rc = RETURN_FAIL;
	BPTR stderr = ZERO;
	struct DiskObject *icon = NULL;
	struct RDArgs *rdargs = NULL;
	BPTR filedir = ZERO;
	CONST_STRPTR filename = NULL;
	LONG unit = -1;
	BOOL writeprotect = FALSE;
	ULONG blockspertrack = 32;
	ULONG surfaces = 1;
	ULONG blocksize = 512;
	ULONG reserved = 2;
	LONG error = NO_ERROR;
	LONG error2 = NO_ERROR;
	TEXT error_buffer[256];
	UBYTE *block_buffer = NULL;
	struct DriveGeometry dg;
	UQUAD filesize;
	ULONG cylindersize;
	TEXT drivename[20];
	IPTR parampkt[21];
	struct DeviceNode *device;
	
	stderr = Output();
	
	if (argc == 0) {
		BPTR currdir;
		WBMsg = (struct WBStartup *)argv;
#ifndef __AROS__
		if (!(IconBase = OpenLibrary("icon.library", MIN_OS_VERSION)) ||
			!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", MIN_OS_VERSION)) ||
			!(RequesterBase = OpenLibrary("requester.class", MIN_OS_VERSION)))
		{
			goto error;
		}
#endif
		if (WBMsg->sm_NumArgs < 2) {
			IoErrRequester(ERROR_REQUIRED_ARG_MISSING, NULL);
			goto error;
		}
		filedir = WBMsg->sm_ArgList[1].wa_Lock;
		filename = WBMsg->sm_ArgList[1].wa_Name;
		currdir = CurrentDir(filedir);
		icon = GetDiskObjectNew((STRPTR)filename);
		CurrentDir(currdir);
		if (!icon) {
			IoErrRequester(IoErr(), NULL);
			goto error;
		}
		unit = TTInteger(icon, "UNIT", -1);
		writeprotect = TTBoolean(icon, "WP") || TTBoolean(icon, "WRITEPROTECT");
		blockspertrack = TTInteger(icon, "BLOCKSPERTRACK", blockspertrack);
		surfaces = TTInteger(icon, "SURFACES", surfaces);
		blocksize = TTInteger(icon, "BLOCKSIZE", blocksize);
		reserved = TTInteger(icon, "RESERVED", TTInteger(icon, "BOOTBLOCKS", reserved));
	} else {
		IPTR args[MAX_ARGS];
		ClearMem(args, sizeof(args));
		rdargs = ReadArgs(TEMPLATE, args, NULL);
		if (!rdargs) {
			PrintFault(IoErr(), PROGNAME);
			goto error;
		}
		filedir = GetCurrentDir();
		filename = (CONST_STRPTR)args[ARG_HDF];
		if (args[ARG_UNIT]) unit = *(LONG *)args[ARG_UNIT];
		writeprotect = args[ARG_WRITEPROTECT] ? TRUE : FALSE;
		if (args[ARG_BLOCKSPERTRACK]) blockspertrack = *(ULONG *)args[ARG_BLOCKSPERTRACK];
		if (args[ARG_SURFACES]) surfaces = *(ULONG *)args[ARG_SURFACES];
		if (args[ARG_BLOCKSIZE]) blocksize = *(ULONG *)args[ARG_BLOCKSIZE];
		if (args[ARG_RESERVED]) reserved = *(ULONG *)args[ARG_RESERVED];
	}
	
	if (unit == -1) {
		if (WBMsg)
			IoErrRequester(ERROR_REQUIRED_ARG_MISSING, NULL);
		else
			PrintFault(ERROR_REQUIRED_ARG_MISSING, PROGNAME);
		goto error;
	}
	
	if (!OpenDiskImageDevice(unit)) {
		SNPrintf(error_buffer, sizeof(error_buffer),
			"failed to open diskimage.device");
		if (WBMsg)
			ErrorStringRequester(error_buffer);
		else
			FPrintf(stderr, "%s: %s\n", PROGNAME, error_buffer);
		goto error;
	}
	
	error_buffer[0] = 0;
	error = UnitControl(unit,
		DITAG_Error,				(IPTR)&error2,
		DITAG_ErrorString,			(IPTR)error_buffer,
		DITAG_ErrorStringLength,	sizeof(error_buffer),
		DITAG_CurrentDir,			(IPTR)filedir,
		DITAG_Filename,				(IPTR)filename,
		DITAG_WriteProtect,			writeprotect,
		TAG_END);
	if (error == NO_ERROR) error = error2;
	if (error_buffer[0]) {
		if (WBMsg)
			ErrorStringRequester(error_buffer);
		else
			FPrintf(stderr, "%s: %s\n", filename, error_buffer);
		goto error;
	} else if (error) {
		if (WBMsg)
			IoErrRequester(error, NULL);
		else
			PrintFault(error, filename);
		goto error;
	}
	
	block_buffer = AllocVec(blocksize, MEMF_ANY);
	if (!block_buffer) {
		if (WBMsg)
			IoErrRequester(ERROR_NO_FREE_STORE, NULL);
		else
			PrintFault(ERROR_NO_FREE_STORE, PROGNAME);
		goto error;
	}

	DiskImageIO->io_Command = TD_GETGEOMETRY;
	DiskImageIO->io_Data = &dg;
	DiskImageIO->io_Length = sizeof(dg);
	if (DoIO((struct IORequest *)DiskImageIO) != IOERR_SUCCESS) {
		SNPrintf(error_buffer, sizeof(error_buffer), "disk geometry error");
		if (WBMsg)
			ErrorStringRequester(error_buffer);
		else
			FPrintf(stderr, "%s: %s\n", PROGNAME, error_buffer);
		goto error;
	}
	filesize = (UQUAD)dg.dg_TotalSectors * dg.dg_SectorSize;
	cylindersize = blockspertrack * surfaces * blocksize;
	
	DiskImageIO->io_Command = CMD_READ;
	DiskImageIO->io_Data = block_buffer;
	DiskImageIO->io_Offset = 0;
	DiskImageIO->io_Length = blocksize;
	if (DoIO((struct IORequest *)DiskImageIO) != IOERR_SUCCESS) {
		SNPrintf(error_buffer, sizeof(error_buffer), "read error on block 0");
		if (WBMsg)
			ErrorStringRequester(error_buffer);
		else
			FPrintf(stderr, "%s: %s\n", PROGNAME, error_buffer);
		goto error;
	}
		
	SNPrintf(drivename, sizeof(drivename), "IHD%ld", unit);
	parampkt[0] = (IPTR)drivename;
	parampkt[1] = (IPTR)"diskimage.device";
	parampkt[2] = unit;
	parampkt[3] = 0;
	parampkt[4] = 16;
	parampkt[5] = blocksize >> 2;
	parampkt[6] = 0;
	parampkt[7] = surfaces;
	parampkt[8] = 1;
	parampkt[9] = blockspertrack;
	parampkt[10] = reserved;
	parampkt[11] = 0;
	parampkt[12] = 0;
	parampkt[13] = 0;
	parampkt[14] = (filesize / cylindersize) - 1;
	parampkt[15] = 5;
	parampkt[16] = MEMF_ANY;
	parampkt[17] = 0x7fffffffUL;
	parampkt[18] = 0xfffffffcUL;
	parampkt[19] = 0;
	parampkt[20] = rbe32(block_buffer);
	device = MakeDosNode(parampkt);
	if (!device) {
		if (WBMsg)
			IoErrRequester(ERROR_NO_FREE_STORE, NULL);
		else
			PrintFault(ERROR_NO_FREE_STORE, PROGNAME);
		goto error;
	}
	if (!AddBootNode(0, ADNF_STARTPROC, device, NULL)) {
		if (WBMsg)
			IoErrRequester(ERROR_DEVICE_NOT_MOUNTED, NULL);
		else
			PrintFault(ERROR_DEVICE_NOT_MOUNTED, PROGNAME);
		goto error;
	}
	
	rc = RETURN_OK;
	
error:
	FreeVec(block_buffer);
	CloseDiskImageDevice();
	if (WBMsg) {
		if (icon) FreeDiskObject(icon);
#ifndef __AROS__
		if (RequesterBase) CloseLibrary(RequesterBase);
		if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
		if (IconBase) CloseLibrary(IconBase);
#endif
	} else {
		FreeArgs(rdargs);
	}
	
	return rc;
}

#ifdef __AROS__

void IoErrRequester (LONG error, CONST_STRPTR header) {
	TEXT bodytext[80];
	if (error == NO_ERROR) {
		return;
	}
	Fault(error, (STRPTR)header, bodytext, sizeof(bodytext));
	MUI_Request(NULL, NULL, 0, "Error - "PROGNAME, "Ok", "%s", bodytext);
}

void ErrorStringRequester (CONST_STRPTR error_string) {
	MUI_Request(NULL, NULL, 0, "Error - "PROGNAME, "Ok", "%s", (STRPTR)error_string);
}

#else

LONG DoReq (Object *req) {
	LONG res = 0;
	if (req) {
		res = DoMethod(req, RM_OPENREQ, NULL, NULL, NULL);
		DisposeObject(req);
	}
	return res;
}

void IoErrRequester (LONG error, CONST_STRPTR header) {
	TEXT bodytext[80];
	Object *req;
	if (error == NO_ERROR) {
		return;
	}
	Fault(error, (STRPTR)header, bodytext, sizeof(bodytext));
	req = RequesterObject,
		REQ_TitleText,	"Error - "PROGNAME,
		REQ_BodyText,	bodytext,
		REQ_GadgetText,	"_Ok",
	End;
	DoReq(req);
}

void ErrorStringRequester (CONST_STRPTR error_string) {
	Object *req;
	req = RequesterObject,
		REQ_TitleText,	"Error - "PROGNAME,
		REQ_BodyText,	error_string,
		REQ_GadgetText,	"_Ok",
	End;
	DoReq(req);
}

#endif
