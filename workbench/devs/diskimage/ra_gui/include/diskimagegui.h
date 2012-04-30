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
#include <dos/dos.h>
#include <devices/diskimage.h>
#include <libraries/commodities.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <libraries/screennotify.h>
#include <reaction/reaction.h>
#include <classes/extwindow.h>
#include <gadgets/extscroller.h>
#include "support.h"
#include "prefs.h"
#include "locale.h"

#define PROGNAME "DiskImageGUI"
#define WORKBENCHNAME "Workbench"
#define GA(o) ((struct Gadget *)(o))
#define MENU_ID(id) ((APTR)(id))

#ifndef ACTION_GET_DISK_FSSM
#define ACTION_GET_DISK_FSSM  4201
#define ACTION_FREE_DISK_FSSM 4202
#endif

enum {
	EVT_DUMMY,
	EVT_POPKEY
};

enum {
	WID_MAIN,
	WID_PLUGINS,
	WID_SETDEVICETYPE,
	WID_MAX
};

enum {
	MID_DUMMY,
	MID_ABOUT,
	MID_HIDE,
	MID_ICONIFY,
	MID_QUIT,
	MID_SNAPSHOT,
	MID_CHANGETEMPDIR,
	MID_SAVESETTINGS,
	MID_PLUGINS
};

enum {
	IID_INSERT,
	IID_EJECT,
	IID_WRITEPROTECT,
	IID_PREFS,
	IID_REFRESH,
	IID_LIST_WRITEPROTECTED,
	IID_LIST_CHECKMARK,
	IID_LIST_PLUGIN,
	IID_LIST_DISK,
	IID_LIST_CDROM,
	IID_MAX
};

enum {
	LID_SPEEDBAR,
	LID_DRIVELIST,
	LID_PLUGINCHOOSER,
	LID_PLUGINLIST,
	LID_DEVICETYPECHOOSER,
	LID_MAX
};

enum {
	GID_SPEEDBAR,
	GID_DRIVELIST,
	GID_DRIVELISTVPROP,
	GID_PLUGINCHOOSER,
	GID_PLUGINLIST,
	GID_PLUGINLISTVPROP,
	GID_DEVICETYPECHOOSER,
	GID_SETDEVICETYPE_SAVE,
	GID_SETDEVICETYPE_CANCEL,
	GID_MAX
};

enum {
	SBID_DUMMY,
	SBID_INSERT,
	SBID_EJECT,
	SBID_WRITEPROTECT,
	SBID_SETDEVICETYPE,
	SBID_REFRESH
};

enum {
	DRIVE_COL_ICON,
	DRIVE_COL_UNIT,
	DRIVE_COL_DEVICE,
	DRIVE_COL_WP,
	DRIVE_COL_DISKIMAGE,
	DRIVE_COL_MAX
};

enum {
	PLUG_COL_ICON,
	PLUG_COL_PRI,
	PLUG_COL_WRITE,
	PLUG_COL_NAME,
	PLUG_COL_MAX
};

struct GUIElements {
	BOOL initialised;
	APTR pool;
	struct MsgPort *userport;
	struct MsgPort *appport;
	struct Screen *screen;
	APTR visualinfo;
	struct Menu *menustrip;
	Object *windows[WID_MAX];
	Object *images[IID_MAX];
	Object *gadgets[GID_MAX];
	struct List *lists[LID_MAX];
#ifdef FALLBACK_IMAGES
	BOOL fallback_image[IID_MAX];
#endif
};

/* main.c */
extern struct LocaleInfo LocaleInfo;
extern struct DiskObject *Icon;
extern struct FileRequester *FileReq;
extern PrefsObject *ApplicationPrefs;
int main (int argc, STRPTR *argv);
struct DiskObject *GetProgramIcon (void);
void BeginScreenNotify (void);
void StopScreenNotify (void);
LONG GetUnitNumber (struct Node *node);
LONG GetSelectedUnit (void);
CONST_STRPTR GetSelectedPlugin (void);
void ScanUnits (void);
void ScanPlugins (void);
void UpdateSpeedBar (void);
void ChangeTempDir (void);
void SaveSettings (void);

/* cxbroker.c */
extern struct MsgPort *BrokerPort;
extern CxObj *Broker;
BOOL RegisterCxBroker (void);
void UnregisterCxBroker (void);

/* snapshot.c */
void RestoreWindowSize (Object *window, CONST_STRPTR name);
void SaveWindowSize (Object *window, CONST_STRPTR name, BOOL envarc);

/* gui.c */
extern struct GUIElements Gui;
BOOL SetupGUI (void);
void CleanupGUI (void);
APTR ShowWindow (ULONG window_id);
void HideWindow (ULONG window_id);
void IconifyWindow (ULONG window_id);
void SetWindowBusy (ULONG window_id, ULONG busy);
ULONG GetGUISignals (void);
void UpdateLBVertScroller (ULONG wnd_id, ULONG lb_id, ULONG sc_id);

/* images.c */
BOOL FindImage (CONST_STRPTR image, STRPTR path, LONG path_size);
Object *LoadImage (struct Screen *screen, CONST_STRPTR image, BOOL selected, BOOL disabled);

/* requesters.c */
LONG DoReq (Object *req);
#ifndef FALLBACK_IMAGES
void ImageNotFoundRequester (CONST_STRPTR image);
#endif
void AboutRequester (void);
void IoErrRequester (LONG error);
void ErrorStringRequester (CONST_STRPTR error_string);
void RequiredVersionRequester (CONST_STRPTR res_name, LONG version, LONG revision);

/* driveops.c */
void InsertOrEjectDisk (void);
void InsertDisk (void);
void EjectDisk (void);
void ToggleWriteProtect (void);

/* setdevicetype.c */
void SetDeviceType (void);

#endif
