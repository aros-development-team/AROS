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

#ifndef DEVICES_DISKIMAGE_H
#define DEVICES_DISKIMAGE_H

#ifndef EXEC_EXEC_H
#include <exec/exec.h>
#endif
#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#ifndef UTILITY_HOOKS_H
#include <utility/hooks.h>
#endif
#ifndef DEVICES_TRACKDISK_H
#include <devices/trackdisk.h>
#endif
#ifndef DEVICES_SCSIDISK_H
#include <devices/scsidisk.h>
#endif
#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif
#ifndef TYPES_H
#include <types.h>
#endif
#ifndef INTERFACES_DIPLUGIN_H
#include <interfaces/diplugin.h>
#endif

#define TEMPDIR_VAR "DiskImageTempPath"

#define NO_ERROR 0
#define NO_ERROR_STRING 0
#define ZERO MKBADDR(NULL)

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
#define clamp(a,b,c) ((a)>(b)?(min(a,c)):(b))
#endif

enum {
	DITAG_DUMMY = TAG_USER,
	DITAG_Error,
	DITAG_OBSOLETE1,
	DITAG_CurrentDir,
	DITAG_Filename,
	DITAG_WriteProtect,
	DITAG_GetImageName,
	DITAG_GetWriteProtect,
	DITAG_DiskImageType,
	DITAG_Screen,
	DITAG_Password,
	DITAG_OBSOLETE2,
	DITAG_Plugin,
	DITAG_OBSOLETE3,
	DITAG_ErrorStringLength,
	DITAG_ErrorString,
	DITAG_RESERVED1,
	DITAG_RESERVED2,
	DITAG_RESERVED3,
	DITAG_SetDeviceType,
	DITAG_GetDeviceType,
	DITAG_SetFlags,
	DITAG_GetFlags
};

enum {
	DITYPE_NONE,
	DITYPE_DISK
};
#define DITYPE_RAW DITYPE_DISK

struct PluginData {
	struct Library *SysBase;
	struct Library *DOSBase;
	struct Library *UtilityBase;
	struct DIPluginIFace *IPlugin;
#ifdef __AROS__
	struct Library *aroscbase;
#endif
};

#define PLUGIN_MAGIC            MAKE_ID('D','I','5','2')
#define MIN_PLUGIN_API_VERSION  8
#define MAX_PLUGIN_API_VERSION  8
#define PLUGIN_FLAG_BUILTIN     0x80000000 
#define PLUGIN_FLAG_M68K        0x40000000
#define PLUGIN_FLAG_USERCHOICE  0x20000000
#define PLUGIN_FLAG_FOOTER      0x00000001
#define PLUGIN_UNUSED_FLAGS     0x1ffffffe

#ifndef QUOTEME
#define _QUOTEME(x) #x
#define QUOTEME(x) _QUOTEME(x)
#endif

#ifdef DEVICE
#define PLUGIN_VERSTAG(name)
#define PLUGIN_TABLE(...)
#else
#define PLUGIN_VERSTAG(name) \
CONST TEXT USED verstag[] = "\0$VER: "name" "QUOTEME(VERSION)"."QUOTEME(REVISION)" ("DATE")";
#define PLUGIN_TABLE(...) \
static struct DiskImagePlugin * const plugin_array[] = { \
	__VA_ARGS__, NULL \
}; \
struct DiskImagePluginTable plugin_table = { \
	PLUGIN_MAGIC, \
	USED_PLUGIN_API_VERSION, \
	0, \
	plugin_array \
};
#endif

#if defined(__AROS__) && defined(AROS_FLAVOUR) && !(AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
#define PLUGIN_NODE(pri,name) { NULL, NULL, name, 0, pri }
#else
#define PLUGIN_NODE(pri,name) { NULL, NULL, 0, pri, name }
#endif

#ifdef USED_PLUGIN_API_VERSION
#if (USED_PLUGIN_API_VERSION < MIN_PLUGIN_API_VERSION) || (USED_PLUGIN_API_VERSION > MAX_PLUGIN_API_VERSION)
#error "Unsupported plugin API version"
#endif
#else
#define USED_PLUGIN_API_VERSION MAX_PLUGIN_API_VERSION
#endif

struct DiskImagePlugin;

struct DiskImagePluginTable {
	ULONG Magic;
	ULONG API_Version;
	ULONG RefCount;
	struct DiskImagePlugin * const *Plugins;
};

struct CDTrack;

struct DiskImagePlugin {
	struct Node Node;
	ULONG Flags;
	ULONG TestSize;
	BPTR SegList;
	ULONG *RefCount;

	BOOL (*plugin_Init)(struct DiskImagePlugin *Self, const struct PluginData *data);
	void (*plugin_Exit)(struct DiskImagePlugin *Self);
	BOOL (*plugin_CheckImage)(struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
		const UBYTE *test, LONG test_len);
	APTR (*plugin_OpenImage)(struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
	void (*plugin_CloseImage)(struct DiskImagePlugin *Self, APTR image);
	LONG (*plugin_Geometry)(struct DiskImagePlugin *Self, APTR image, struct DriveGeometry *dg);
	LONG (*plugin_Read)(struct DiskImagePlugin *Self, APTR image, struct IOStdReq *io);
	LONG (*plugin_Write)(struct DiskImagePlugin *Self, APTR image, struct IOStdReq *io);
	LONG (*plugin_RawRead)(struct DiskImagePlugin *Self, APTR image, struct IOStdReq *io);
	LONG (*plugin_RawWrite)(struct DiskImagePlugin *Self, APTR image, struct IOStdReq *io);
	void (*plugin_GetCDTracks)(struct DiskImagePlugin *Self, APTR image, struct CDTrack **tracks, ULONG *num_tracks);
	LONG (*plugin_ReadCDDA)(struct DiskImagePlugin *Self, APTR image, APTR buffer, ULONG addr, ULONG frames);
};

#define Plugin_Init(p,a) p->plugin_Init(p,a)
#define Plugin_Exit(p) p->plugin_Exit(p)
#define Plugin_CheckImage(p,a,b,c,d,e) p->plugin_CheckImage(p,a,b,c,d,e)
#define Plugin_OpenImage(p,a,b,c) p->plugin_OpenImage(p,a,b,c)
#define Plugin_CloseImage(p,a) p->plugin_CloseImage(p,a)
#define Plugin_Geometry(p,a,b) p->plugin_Geometry(p,a,b)
#define Plugin_Read(p,a,b) p->plugin_Read(p,a,b)
#define Plugin_Write(p,a,b) p->plugin_Write(p,a,b)
#define Plugin_RawRead(p,a,b) p->plugin_RawRead(p,a,b)
#define Plugin_RawWrite(p,a,b) p->plugin_RawWrite(p,a,b)
#define Plugin_GetCDTracks(p,a,b,c) p->plugin_GetCDTracks(p,a,b,c)
#define Plugin_ReadCDDA(p,a,b,c,d) p->plugin_ReadCDDA(p,a,b,c,d)

struct CDTrack {
	struct CDTrack *next;
	UBYTE track_num;
	UBYTE audio;
	UWORD sector_size;
	UBYTE sync_size;
	UQUAD offset;
	UQUAD length;
	UQUAD sectors;
	// plugin specific data starts here
};

#ifndef SUPPORT_H
#include "support.h"
#endif

#endif
