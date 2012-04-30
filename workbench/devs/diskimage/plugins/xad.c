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

#define USED_PLUGIN_API_VERSION 8
#include <devices/diskimage.h>
#include <gadgets/fuelgauge.h>
#include <libraries/xadmaster.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/xadmaster.h>
#include <string.h>
#include "device_locale.h"
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("XAD")

extern struct DiskImagePlugin xad_plugin;

PLUGIN_TABLE(&xad_plugin)

BOOL XAD_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
void XAD_Exit (struct DiskImagePlugin *Self);
BOOL XAD_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR XAD_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
static ULONG ProgressHookFunc (REG(a0, struct Hook *hook), REG(a2, void *unused),
	REG(a1, struct xadProgressInfo *pi));
static LONG XADError (LONG error, LONG *error_string);

struct DiskImagePlugin xad_plugin = {
	PLUGIN_NODE(-1, "XAD"),
	PLUGIN_FLAG_M68K,
	0,
	ZERO,
	NULL,
	XAD_Init,
	XAD_Exit,
	XAD_CheckImage,
	XAD_OpenImage,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

struct Library *SysBase;
struct Library *DOSBase;
static struct DIPluginIFace *IPlugin;
static struct xadMasterBase *xadMasterBase;

BOOL XAD_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

void XAD_Exit (struct DiskImagePlugin *Self) {
	if (xadMasterBase) CloseLibrary((struct Library *)xadMasterBase);
}

BOOL XAD_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	struct xadArchiveInfo *xai;
	BOOL is_xad = FALSE;

	if (!xadMasterBase) {
		xadMasterBase = (struct xadMasterBase *)OpenLibrary("xadmaster.library", 0);
		if (!xadMasterBase) return FALSE;
	}

	xai = xadAllocObjectA(XADOBJ_ARCHIVEINFO, NULL);
	if (xai) {
		if (xadGetInfo(xai, XAD_INFILEHANDLE, file, TAG_END) == XADERR_OK) {
			if (xai->xai_FileInfo) is_xad = TRUE;
			xadFreeInfo(xai);
		}
		xadFreeObjectA(xai, NULL);
		ChangeFilePosition(file, 0, OFFSET_BEGINNING);
	}
	return is_xad;
}

static CONST TEXT match_str1[] = "#?.(adf|b5i|bin|cdi|cso|d64|daa|dax|"
	"dmg|img|iso|mdf|nrg|pdi|raw|sad|toast|uif)";
static CONST TEXT match_str2[] = "~(#?(displayme|readme|liesmich)#?|"
	"#?.(diz|info|doc|dok|txt|text|exe|lst|cue|mds|nfo|pdf))";

static CONST CONST_STRPTR match_strs[] = {
	match_str1, match_str2, NULL
};

APTR XAD_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file,
	CONST_STRPTR name)
{
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	APTR image = NULL;
	struct xadArchiveInfo *xai;
	UBYTE *patbuf;
	ULONG patbuf_size;
	struct xadFileInfo *xfi, *chosen = NULL;
	CONST CONST_STRPTR *match_str;
	BPTR outfile = ZERO;
	CONST_STRPTR ext;
	BPTR tmpdir;
	CONST_STRPTR tmpname;

	if (!xadMasterBase) {
		xadMasterBase = (struct xadMasterBase *)OpenLibrary("xadmaster.library", 0);
		if (!xadMasterBase) {
			error = ERROR_OBJECT_NOT_FOUND;
			error_string = MSG_REQ;
			error_args[0] = (IPTR)"xadmaster.library";
			goto error;
		}
	}

	xai = xadAllocObjectA(XADOBJ_ARCHIVEINFO, NULL);
	patbuf_size = 2*max(sizeof(match_str1), sizeof(match_str2))+2;
	patbuf = AllocVec(patbuf_size, MEMF_ANY);
	if (!xai || !patbuf) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	if (xadGetInfo(xai, XAD_INFILEHANDLE, file, TAG_END) != XADERR_OK) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}
		
	match_str = match_strs;
	while (!chosen && *match_str) {
		ParsePatternNoCase(*match_str++, patbuf, patbuf_size);
		for (xfi = xai->xai_FileInfo; xfi; xfi = xfi->xfi_Next) {
			if (MatchPatternNoCase(patbuf, FilePart(xfi->xfi_FileName))) {
				chosen = xfi;
				break;
			}
		}
	}
	FreeVec(patbuf);
	patbuf = NULL;
	if (!chosen) {
		chosen = xai->xai_FileInfo;
		if (!chosen) {
			error = ERROR_OBJECT_NOT_FOUND;
			goto error;
		}
	}

	ext = strrchr(FilePart(chosen->xfi_FileName), '.');
	if (ext) ext++;

	error = IPlugin_CreateTempFile(unit, ext, &tmpdir, &tmpname);
	if (error != NO_ERROR) goto error;

	outfile = IPlugin_OpenTempFile(unit, MODE_NEWFILE);
	if (!outfile) {
		error = IoErr();
		goto error;
	} else {
		struct Hook progresshook = {0};
		LONG xad_err;

		progresshook.h_Entry = ProgressHookFunc;
		progresshook.h_Data = IPlugin_CreateProgressBar(unit, TRUE);

		xad_err = xadFileUnArc(xai,
			XAD_OUTFILEHANDLE,	outfile,
			XAD_OVERWRITE,		TRUE,
			XAD_ENTRYNUMBER,	chosen->xfi_EntryNumber,
			XAD_PROGRESSHOOK,	&progresshook,
			TAG_END);
		if (xad_err == XADERR_PASSWORD) {
			CONST_STRPTR passwd;
			passwd = IPlugin_RequestPassword(unit);
			if (passwd) {
				xad_err = xadFileUnArc(xai,
					XAD_PASSWORD,		passwd,
					XAD_OUTFILEHANDLE,	outfile,
					XAD_OVERWRITE,		TRUE,
					XAD_ENTRYNUMBER,	chosen->xfi_EntryNumber,
					XAD_PROGRESSHOOK,	&progresshook,
					TAG_DONE);
				FreeVec(passwd);
			} else {
				error = ERROR_NO_FREE_STORE;
			}
		}
		IPlugin_DeleteProgressBar(progresshook.h_Data);
		Close(outfile);
		xadFreeObjectA(xai, NULL);
		xai = NULL;
		Close(file);
		file = ZERO;
		if (error != NO_ERROR) goto error;

		error = XADError(xad_err, &error_string);
		if (error != NO_ERROR) goto error;

		outfile = IPlugin_OpenTempFile(unit, MODE_OLDFILE);
		if (!outfile) {
			error = IoErr();
			goto error;
		}

		done = TRUE;
		tmpdir = CurrentDir(tmpdir);
		image = IPlugin_OpenImage(unit, outfile, tmpname);
		CurrentDir(tmpdir);
	}

error:
	if (xai) xadFreeObjectA(xai, NULL);
	FreeVec(patbuf);
	Close(file);
	if (!done) {
		IPlugin_SetDiskImageErrorA(unit, error, error_string, error_args);
	}
	return image;
}

static ULONG ProgressHookFunc (REG(a0, struct Hook *hook), REG(a2, void *unused),
	REG(a1, struct xadProgressInfo *pi))
{
	APTR pb = hook->h_Data;
	if (pb) {
		switch (pi->xpi_Mode) {
			case XADPMODE_PROGRESS:
				if (pi->xpi_FileInfo->xfi_Flags & XADFIF_NOUNCRUNCHSIZE) {
					hook->h_SubEntry = (void *)(pi->xpi_CurrentSize >> 10);
					IPlugin_SetProgressBarAttrs(pb,
						FUELGAUGE_Percent,	FALSE,
						FUELGAUGE_Level,	0,
						GA_Text,			"%ld KB",
						FUELGAUGE_VarArgs,	&hook->h_SubEntry,
						TAG_END);
				} else {
					IPlugin_SetProgressBarAttrs(pb,
						FUELGAUGE_Percent,	TRUE,
						FUELGAUGE_Max,		pi->xpi_FileInfo->xfi_Size,
						FUELGAUGE_Level,	pi->xpi_CurrentSize,
						TAG_END);
				}
				break;
		}
	}
	return IPlugin_ProgressBarInput(pb) ? 0 : XADPIF_OK;
}

static LONG XADError (LONG error, LONG *error_string) {
	switch (error) {
		case XADERR_OK:
			return NO_ERROR;
		case XADERR_INPUT:
		case XADERR_OUTPUT:
			return ERROR_BUFFER_OVERFLOW;
		case XADERR_NOMEMORY:
			return ERROR_NO_FREE_STORE;
		case XADERR_BREAK:
			*error_string = MSG_CANCELED;
			return ERROR_BREAK;
		case XADERR_PASSWORD:
			*error_string = MSG_WRONGPASSWD;
			return ERROR_REQUIRED_ARG_MISSING;
		default:
			*error_string = MSG_XADERR;
			return ERROR_OBJECT_WRONG_TYPE;
	}
}
