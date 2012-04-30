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

#ifndef INTERFACES_DIPLUGIN_H
#define INTERFACES_DIPLUGIN_H

#include <SDI_compiler.h>

struct InterfaceData {
	struct Library *LibBase;
	ULONG Version;
};

struct DIPluginIFace {
	struct InterfaceData Data;
	LONG (*DOS2IOErr)(struct DIPluginIFace *Self, LONG error);
	APTR (*OpenImage)(struct DIPluginIFace *Self, APTR unit, BPTR file, CONST_STRPTR name);
	LONG (*CreateTempFile)(struct DIPluginIFace *Self, APTR unit, CONST_STRPTR ext,
		BPTR *tmpdir, CONST_STRPTR *tmpname);
	BPTR (*OpenTempFile)(struct DIPluginIFace *Self, APTR unit, ULONG mode);
	void (*RemoveTempFile)(struct DIPluginIFace *Self, APTR unit);
	STRPTR (*RequestPassword)(struct DIPluginIFace *Self, APTR unit);
	APTR (*CreateProgressBar)(struct DIPluginIFace *Self, APTR unit, BOOL stop);
	void (*DeleteProgressBar)(struct DIPluginIFace *Self, APTR bar);
	BOOL (*ProgressBarInput)(struct DIPluginIFace *Self, APTR bar);
	void (*SetProgressBarAttrsA)(struct DIPluginIFace *Self, APTR bar, const struct TagItem *tags);
	VARARGS68K void (*SetProgressBarAttrs)(struct DIPluginIFace *Self, APTR bar, ...);
	void (*SetDiskImageErrorA)(struct DIPluginIFace *Self, APTR unit, LONG error, LONG error_string, CONST_APTR error_args);
	VARARGS68K void (*SetDiskImageError)(struct DIPluginIFace *Self, APTR unit, LONG error, LONG error_string, ...);
};

#define IPlugin_DOS2IOErr(a) IPlugin->DOS2IOErr(IPlugin,a)
#define IPlugin_OpenImage(a,b,c) IPlugin->OpenImage(IPlugin,a,b,c)
#define IPlugin_CreateTempFile(a,b,c,d) IPlugin->CreateTempFile(IPlugin,a,b,c,d)
#define IPlugin_OpenTempFile(a,b) IPlugin->OpenTempFile(IPlugin,a,b)
#define IPlugin_RemoveTempFile(a) IPlugin->RemoveTempFile(IPlugin,a)
#define IPlugin_RequestPassword(a) IPlugin->RequestPassword(IPlugin,a)
#define IPlugin_CreateProgressBar(a,b) IPlugin->CreateProgressBar(IPlugin,a,b)
#define IPlugin_DeleteProgressBar(a) IPlugin->DeleteProgressBar(IPlugin,a)
#define IPlugin_ProgressBarInput(a) IPlugin->ProgressBarInput(IPlugin,a)
#define IPlugin_SetProgressBarAttrsA(a,b) IPlugin->SetProgressBarAttrsA(IPlugin,a,b)
#define IPlugin_SetProgressBarAttrs(a,...) IPlugin->SetProgressBarAttrs(IPlugin,a,__VA_ARGS__)
#define IPlugin_SetDiskImageErrorA(a,b,c,d) IPlugin->SetDiskImageErrorA(IPlugin,a,b,c,d)
#define IPlugin_SetDiskImageError(a,b,...) IPlugin->SetDiskImageError(IPlugin,a,b,__VA_ARGS__)

#endif
