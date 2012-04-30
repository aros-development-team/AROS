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
#include <xpk/xpk.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/xpkmaster.h>
#include <string.h>
#include "device_locale.h"
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("XPK")

extern struct DiskImagePlugin xpk_plugin;

PLUGIN_TABLE(&xpk_plugin)

BOOL XPK_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
void XPK_Exit (struct DiskImagePlugin *Self);
BOOL XPK_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR XPK_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
static ULONG ProgressHookFunc (REG(a0, struct Hook *hook), REG(a2, void *unused),
	REG(a1, struct XpkProgress *xp));
static LONG XPKError (LONG error, LONG *error_string);

struct DiskImagePlugin xpk_plugin = {
	PLUGIN_NODE(-1, "XPK"),
	PLUGIN_FLAG_M68K,
	0,
	ZERO,
	NULL,
	XPK_Init,
	XPK_Exit,
	XPK_CheckImage,
	XPK_OpenImage,
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
static struct Library *XpkBase;

BOOL XPK_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

void XPK_Exit (struct DiskImagePlugin *Self) {
	if (XpkBase) CloseLibrary(XpkBase);
}

BOOL XPK_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	struct XpkFib *xf;
	BOOL is_xpk = FALSE;

	if (!XpkBase) {
		XpkBase = OpenLibrary("xpkmaster.library", 0);
		if (!XpkBase) return FALSE;
	}

	xf = XpkAllocObject(XPKOBJ_FIB, NULL);
	if (xf) {
		if (XpkExamineTags(xf, XPK_InFH, file, TAG_DONE) == XPKERR_OK) {
			if (xf->xf_Type == XPKTYPE_PACKED) is_xpk = TRUE;
		}
		XpkFreeObject(XPKOBJ_FIB, xf);
		ChangeFilePosition(file, 0, OFFSET_BEGINNING);
	}
	return is_xpk;
}

APTR XPK_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file,
	CONST_STRPTR name)
{
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	APTR image = NULL;
	BPTR outfile;
	CONST_STRPTR ext;
	BPTR tmpdir;
	CONST_STRPTR tmpname;

	if (!XpkBase) {
		XpkBase = OpenLibrary("xpkmaster.library", 0);
		if (!XpkBase) {
			error = ERROR_OBJECT_NOT_FOUND;
			error_string = MSG_REQ;
			error_args[0] = (IPTR)"xpkmaster.library";
			goto error;
		}
	}

	ext = strrchr(FilePart(name), '.');
	if (ext) ext++;

	error = IPlugin_CreateTempFile(unit, ext, &tmpdir, &tmpname);
	if (error) goto error;

	outfile = IPlugin_OpenTempFile(unit, MODE_NEWFILE);
	if (!outfile) {
		error = IoErr();
		goto error;
	} else {
		struct Hook progresshook = {0};
		LONG xpk_err;

		progresshook.h_Entry = ProgressHookFunc;
		progresshook.h_Data = IPlugin_CreateProgressBar(unit, TRUE);

		xpk_err = XpkUnpackTags(
			XPK_InFH,		file,
			XPK_OutFH,		outfile,
			XPK_ChunkHook,	&progresshook,
			TAG_END);
		if (xpk_err == XPKERR_NEEDPASSWD) {
			CONST_STRPTR passwd;
			passwd = IPlugin_RequestPassword(unit);
			if (passwd) {
				xpk_err = XpkUnpackTags(
					XPK_Password,	passwd,
					XPK_InFH,		file,
					XPK_OutFH,		outfile,
					XPK_ChunkHook,	&progresshook,
					TAG_END);
				FreeVec(passwd);
			} else {
				error = ERROR_NO_FREE_STORE;
			}
		}
		IPlugin_DeleteProgressBar(progresshook.h_Data);
		Close(outfile);
		Close(file);
		file = ZERO;
		if (error != NO_ERROR) goto error;

		error = XPKError(xpk_err, &error_string);
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
	Close(file);
	if (!done) {
		IPlugin_SetDiskImageErrorA(unit, error, error_string, error_args);
	}
	return image;
}

static ULONG ProgressHookFunc (REG(a0, struct Hook *hook), REG(a2, void *unused),
	REG(a1, struct XpkProgress *xp))
{
	APTR pb = hook->h_Data;
	if (pb) {
		IPlugin_SetProgressBarAttrs(pb,
			FUELGAUGE_Percent,	TRUE,
			FUELGAUGE_Max,		100,
			FUELGAUGE_Level,	xp->xp_Done,
			TAG_END);
	}
	return IPlugin_ProgressBarInput(pb) ? 1 : 0;
}

static LONG XPKError (LONG error, LONG *error_string) {
	switch (error) {
		case XPKERR_OK:
			return NO_ERROR;
		case XPKERR_NOMEM:
			return ERROR_NO_FREE_STORE;
		case XPKERR_ABORTED:
			*error_string = MSG_CANCELED;
			return ERROR_BREAK;
		case XPKERR_NEEDPASSWD:
			*error_string = MSG_NOPASSWD;
			return ERROR_REQUIRED_ARG_MISSING;
		case XPKERR_WRONGPW:
			*error_string = MSG_WRONGPASSWD;
			return ERROR_REQUIRED_ARG_MISSING;
		default:
			*error_string = MSG_XPKERR;
			return ERROR_OBJECT_WRONG_TYPE;
	}
}
