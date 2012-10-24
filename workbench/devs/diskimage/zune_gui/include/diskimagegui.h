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

#ifndef DISKIMAGEGUI_H
#define DISKIMAGEGUI_H

#include <exec/exec.h>
#include <devices/diskimage.h>
#include <dos/dos.h>
#include <dos/filehandler.h>
#include <libraries/mui.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include "support.h"
#include "locale.h"

#define PROGNAME "DiskImageGUI"
#define MENU_ID(id) ((APTR)(id))
#ifndef DtpicObject
#define DtpicObject MUI_NewObject(MUIC_Dtpic
#endif

enum {
	EVT_DUMMY,
	EVT_POPKEY
};

enum {
	WID_MAIN,
	WID_PLUGINS,
	WID_ABOUT,
	WID_SETDEVICETYPE,
	WID_MAX
};

enum {
	MID_DUMMY,
	MID_ABOUT,
	MID_HIDE,
	MID_ICONIFY,
	MID_SNAPSHOT,
	MID_QUIT,
	MID_CHANGETEMPDIR,
	MID_PLUGINS,
	MID_SAVESETTINGS
};

enum {
	GID_INSERT,
	GID_EJECT,
	GID_WRITEPROTECT,
	GID_SETDEVICETYPE,
	GID_REFRESH,
	GID_DRIVELIST,
	GID_PLUGINLIST,
	GID_ABOUT_OK,
	GID_DEVICETYPE,
	GID_SETDEVICETYPE_SAVE,
	GID_SETDEVICETYPE_CANCEL,
	GID_MAX
};

struct GUIElements {
	BOOL initialised;
	APTR pool;
	Object *app;
	Object *wnd[WID_MAX];
	Object *gad[GID_MAX];
};

struct DriveEntry {
	ULONG unit_num;
	UBYTE device_type;
	BOOL writeprotect;
	STRPTR unit;
	STRPTR drive;
	STRPTR diskimage;
	IPTR  list_pos;
};

struct PluginEntry {
	BYTE pri_num;
	UBYTE is_builtin;
	UBYTE has_write;
	STRPTR priority;
	STRPTR name;
	IPTR  list_pos;
};

/* main.c */
extern CONST TEXT verstag[];
extern struct LocaleInfo LocaleInfo;
extern BYTE DiskChangeSignal;
extern BYTE ReloadPluginsSignal;
extern struct DiskObject *Icon;
extern struct FileRequester *FileReq;
extern struct MUI_CustomClass *DriveListClass;
extern struct MUI_CustomClass *PluginListClass;
extern struct Hook BrokerHook;
extern struct Hook MenuHook;
extern struct Hook SignalHook;
int main (void);
void ScanUnits (void);
void ScanPlugins (void);
void ChangeTempDir (void);
void SaveSettings (void);

/* gui.c */
extern struct GUIElements Gui;
BOOL CreateGUI (void);
void CleanupGUI (void);
void SetWindowBusy (ULONG wnd_id, ULONG busy);

/* drivelist.c */
struct MUI_CustomClass *DriveList_CreateClass (void);
void DriveList_FreeClass (struct MUI_CustomClass *cl);
extern struct Hook DriveList_ActiveHook;
extern struct Hook DriveList_DoubleClickHook;

/* drivelist.c */
struct MUI_CustomClass *PluginList_CreateClass (void);
void PluginList_FreeClass (struct MUI_CustomClass *cl);

/* driveops.c */
extern struct Hook InsertHook;
extern struct Hook EjectHook;
extern struct Hook WriteProtectHook;
extern struct Hook SetDeviceTypeHook;
extern struct Hook DoSetDeviceTypeHook;

/* requesters.c */
void ImageNotFoundRequester (CONST_STRPTR image);
void IoErrRequester (LONG error);
void ErrorStringRequester (CONST_STRPTR error_string);

/* images.c */
BOOL FindImage (CONST_STRPTR image, STRPTR path, LONG path_size);
Object *LoadImage (CONST_STRPTR image, const struct TagItem *tags);
Object *MakeImageButton (CONST_STRPTR image, CONST_STRPTR help, BOOL disabled);

#endif
