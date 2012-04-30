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
#include <proto/exec.h>
#include <proto/dos.h>
#include <xdms.h>
#include <string.h>
#include "device_locale.h"
#include "endian.h"
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("DMS")

extern struct DiskImagePlugin dms_plugin;

PLUGIN_TABLE(&dms_plugin)

BOOL DMS_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL DMS_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR DMS_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);

struct DiskImagePlugin dms_plugin = {
	PLUGIN_NODE(0, "DMS"),
	PLUGIN_FLAG_M68K,
	4,
	ZERO,
	NULL,
	DMS_Init,
	NULL,
	DMS_CheckImage,
	DMS_OpenImage,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static struct Library *SysBase;
static struct Library *DOSBase;
static struct DIPluginIFace *IPlugin;

BOOL DMS_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

#define DMS_MAGIC MAKE_ID('D','M','S','!')

BOOL DMS_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return testsize >= sizeof(ULONG) && rbe32(test) == DMS_MAGIC;
}

#define TRACK_BUFFER_SIZE (32*1024)
#define TEMP_BUFFER_SIZE (32*1024)

#define HEADLEN 56
#define THLEN 20

#define ERR_BADDECR 1
#define ERR_UNKNMODE 2

static LONG process_track (struct xdms_data *xdms, BPTR in, BPTR out, UBYTE *b1, UBYTE *b2, UWORD pwd,
	UBYTE *eof, LONG *error_string);
static UWORD unpack_track (struct xdms_data *xdms, UBYTE *b1, UBYTE *b2, UWORD pklen2, UWORD unpklen,
	UWORD cmode, UBYTE flags);
static void dms_decrypt (struct xdms_data *xdms, UBYTE *p, UWORD len);

APTR DMS_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file,
	CONST_STRPTR name)
{
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	APTR image = NULL;
	UBYTE *b1, *b2;
	UWORD geninfo, hcrc, disktype;
	BPTR outfile = ZERO;
	UBYTE eof;
	UWORD pwd = 0;
	BPTR tmpdir;
	CONST_STRPTR tmpname;
	struct xdms_data *xdms;
	UBYTE *text;

	b1 = AllocVec(TRACK_BUFFER_SIZE, MEMF_ANY);
	b2 = AllocVec(TRACK_BUFFER_SIZE, MEMF_ANY);
	xdms = AllocVec(sizeof(struct xdms_data), MEMF_CLEAR);
	text = AllocVec(TEMP_BUFFER_SIZE, MEMF_ANY);
	if (!b1 || !b2 || !xdms || !text) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	xdms->text = text;

	if (FRead(file, b1, 1, HEADLEN) != HEADLEN) {
		error = IoErr();
		goto error;
	}
	hcrc = rbe16(&b1[HEADLEN-2]);
	if (hcrc != CreateCRC(b1+4, HEADLEN-6)) {
		error = ERROR_BAD_NUMBER;
		error_string = MSG_BADCRC;
		goto error;
	}

	geninfo = rbe16(&b1[10]);
	disktype = rbe16(&b1[50]);

	if (disktype == 7) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}
	if (geninfo & 2) {
		STRPTR passwd;
		pwd = 1;
		passwd = IPlugin_RequestPassword(unit);
		if (!passwd) {
			error = ERROR_NO_FREE_STORE;
			goto error;
		}
		if (passwd[0]) {
			xdms->PWDCRC = CreateCRC(passwd, strlen(passwd));
			FreeVec(passwd);
		} else {
			FreeVec(passwd);
			error = ERROR_REQUIRED_ARG_MISSING;
			error_string = MSG_NOPASSWD;
			goto error;
		}
	}

	error = IPlugin_CreateTempFile(unit, "adf", &tmpdir, &tmpname);
	if (error) {
		goto error;
	}
	outfile = IPlugin_OpenTempFile(unit, MODE_NEWFILE);
	if (!outfile) {
		error = IoErr();
		goto error;
	}

	Init_Decrunchers(xdms);

	eof = 0;
	while (error == NO_ERROR && !eof)
		error = process_track(xdms, file, outfile, b1, b2, pwd, &eof, &error_string);
	if (error != NO_ERROR) goto error;

	done = TRUE;

	Close(outfile);
	outfile = IPlugin_OpenTempFile(unit, MODE_OLDFILE);
	if (!outfile) {
		error = IoErr();
		goto error;
	}

	done = TRUE;
	tmpdir = CurrentDir(tmpdir);
	image = IPlugin_OpenImage(unit, outfile, tmpname);
	outfile = ZERO;
	CurrentDir(tmpdir);

error:
	FreeVec(b1);
	FreeVec(b2);
	FreeVec(xdms);
	FreeVec(text);
	Close(file);
	Close(outfile);
	if (!done) {
		if (error == NO_ERROR) {
			error = ERROR_OBJECT_WRONG_TYPE;
			error_string = MSG_EOF;
		}
		IPlugin_SetDiskImageErrorA(unit, error, error_string, error_args);
	}
	return image;
}

static LONG process_track (struct xdms_data *xdms, BPTR in, BPTR out, UBYTE *b1, UBYTE *b2, UWORD pwd,
	UBYTE *eof, LONG *error_string)
{
	LONG error;
	UWORD l, hcrc, dcrc, number, pklen1, pklen2, unpklen, usum;
	UBYTE flags, cmode;

	l = FRead(in, b1, 1, THLEN);
	if (l != THLEN) {
		*eof = 1;
		if (l == 0) {
			return NO_ERROR;
		}
		error = IoErr();
		if (!error) {
			error = ERROR_OBJECT_WRONG_TYPE;
			*error_string = MSG_EOF;
		}
		return error;
	}

	if (b1[0] != 'T' || b1[1] != 'R') {
		*eof = 1;
		return NO_ERROR;
	}

	hcrc = rbe16(&b1[THLEN-2]);
	if (hcrc != CreateCRC(b1, THLEN-2)) {
		error = ERROR_BAD_NUMBER;
		*error_string = MSG_BADCRC;
		return error;
	}

	number = rbe16(&b1[2]);
	pklen1 = rbe16(&b1[6]);
	pklen2 = rbe16(&b1[8]);
	unpklen = rbe16(&b1[10]);
	flags = b1[12];
	cmode = b1[13];
	usum = rbe16(&b1[14]);
	dcrc = rbe16(&b1[16]);

	if (pklen1 > TRACK_BUFFER_SIZE || pklen2 > TRACK_BUFFER_SIZE ||
		unpklen > TRACK_BUFFER_SIZE)
	{
		return ERROR_BUFFER_OVERFLOW;
	}

	if (FRead(in, b1, 1, pklen1) != pklen1) {
		*eof = 1;
		error = IoErr();
		if (!error) {
			error = ERROR_OBJECT_WRONG_TYPE;
			*error_string = MSG_EOF;
		}
		return error;
	}

	if (dcrc != CreateCRC(b1, pklen1)) {
		error = ERROR_BAD_NUMBER;
		*error_string = MSG_BADCRC;
		return error;
	}

	if (pwd && number != 80) dms_decrypt(xdms, b1, pklen1);

	if (number < 80 && unpklen > 2048) {
		error = unpack_track(xdms, b1, b2, pklen2, unpklen, cmode, flags);
		switch (error) {
			case ERR_BADDECR:
				error = ERROR_BAD_NUMBER;
				*error_string = MSG_BADDATA;
				return error;
			case ERR_UNKNMODE:
				error = ERROR_BAD_NUMBER;
				*error_string = MSG_UNKNCOMPMETHOD;
				return error;
		}

		if (usum != Calc_CheckSum(b2, unpklen)) {
			error = ERROR_BAD_NUMBER;
			*error_string = MSG_BADCHECKSUM;
			return error;
		}

		if (Write(out, b2, unpklen) != unpklen) {
			return IoErr();
		}
	}

	return NO_ERROR;
}

static UWORD unpack_track (struct xdms_data *xdms, UBYTE *b1, UBYTE *b2, UWORD pklen2, UWORD unpklen, UWORD cmode,
	UBYTE flags)
{
	switch (cmode) {
		case 1:
			if (Unpack_RLE(xdms, b1, b2, unpklen)) return ERR_BADDECR;
			break;
		case 2:
			if (Unpack_QUICK(xdms, b1, b2, pklen2)) return ERR_BADDECR;
			if (Unpack_RLE(xdms, b2, b1, unpklen)) return ERR_BADDECR;
		case 0:
			memcpy(b2, b1, unpklen);
			break;
		case 3:
			if (Unpack_MEDIUM(xdms, b1, b2, pklen2)) return ERR_BADDECR;
			if (Unpack_RLE(xdms, b2, b1, unpklen)) return ERR_BADDECR;
			memcpy(b2, b1, unpklen);
			break;
		case 4:
			if (Unpack_DEEP(xdms, b1, b2, pklen2)) return ERR_BADDECR;
			if (Unpack_RLE(xdms, b2, b1, unpklen)) return ERR_BADDECR;
			memcpy(b2, b1, unpklen);
			break;
		case 5:
		case 6:
			if (Unpack_HEAVY(xdms, b1, b2, (cmode == 5) ? (flags & 7) : (flags | 8), pklen2))
				return ERR_BADDECR;
			if (flags & 4) {
				if (Unpack_RLE(xdms, b2, b1, unpklen)) return ERR_BADDECR;
				memcpy(b2, b1, unpklen);
			}
			break;
		default:
			return ERR_UNKNMODE;
	}

	if (!(flags & 1)) Init_Decrunchers(xdms);

	return NO_ERROR;
}

static void dms_decrypt (struct xdms_data *xdms, UBYTE *p, UWORD len) {
	UWORD t;
	while (len--) {
		t = *p;
		*p++ ^= (UBYTE)xdms->PWDCRC;
		xdms->PWDCRC = (UWORD)((xdms->PWDCRC >> 1) + t);
	}
}
