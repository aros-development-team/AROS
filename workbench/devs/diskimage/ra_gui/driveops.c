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

#include "diskimagegui.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/asl.h>
#include <proto/diskimage.h>

void InsertOrEjectDisk (void) {
	LONG unit_num;
	ULONG disk_type;

	unit_num = GetSelectedUnit();
	if (unit_num == -1) {
		return;
	}

	disk_type = DITYPE_NONE;
	UnitControl(unit_num,
		DITAG_DiskImageType,	&disk_type,
		TAG_END);
	if (disk_type == DITYPE_NONE) {
		InsertDisk();
	} else {
		EjectDisk();
	}
}

void InsertDisk (void) {
	struct Window *window;
	Object *listbrowser;
	struct Node *node;
	LONG unit_num;

	window = NULL;
	GetAttr(WINDOW_Window, Gui.windows[WID_MAIN], (Tag *)&window);
	listbrowser = Gui.gadgets[GID_DRIVELIST];

	node = NULL;
	GetAttr(LISTBROWSER_SelectedNode, Gui.gadgets[GID_DRIVELIST], (Tag *)&node);
	unit_num = GetUnitNumber(node);
	if (unit_num == -1) {
		return;
	}

	StopScreenNotify();
	SetWindowBusy(~0, TRUE);

	if (AslRequestTags(FileReq,
		ASLFR_Screen,	Gui.screen,
		TAG_END))
	{
		STRPTR filename;
		filename = CombinePaths(FileReq->fr_Drawer, FileReq->fr_File);
		if (filename) {
			ULONG disk_type = DITYPE_NONE;
			LONG error = NO_ERROR;
			TEXT error_buffer[256];
			LONG msge;
			
			UnitControl(unit_num,
				DITAG_DiskImageType,		&disk_type,
				DITAG_Filename,				NULL,
				TAG_END);
			if (disk_type != DITYPE_NONE) {
				Delay(5);
				disk_type = DITYPE_NONE;
			}

			error_buffer[0] = 0;
			msge = UnitControl(unit_num,
				DITAG_Error,				&error,
				DITAG_ErrorString,			error_buffer,
				DITAG_ErrorStringLength,	sizeof(error_buffer),
				DITAG_Plugin,				GetSelectedPlugin(),
				DITAG_CurrentDir,			GetCurrentDir(),
				DITAG_Filename,				filename,
				DITAG_DiskImageType,		&disk_type,
				TAG_END);

			if (error_buffer[0]) {
				ErrorStringRequester(error_buffer);
			} else {
				IoErrRequester(error ? error : msge);
			}
			
			SetAttrs(listbrowser,
				LISTBROWSER_Labels,			~0,
				TAG_END);

			SetListBrowserNodeAttrs(node,
				LBNA_Column,				DRIVE_COL_DISKIMAGE,
				LBNCA_CopyText,				disk_type ? TRUE : FALSE,
				LBNCA_Text,					disk_type ? FilePart(filename) : GetString(&LocaleInfo, MSG_NO_DISK),
				TAG_END);

			SetGadgetAttrs(GA(listbrowser), window, NULL,
				LISTBROWSER_Labels,			Gui.lists[LID_DRIVELIST],
				TAG_END);

			FreeVec(filename);

			UpdateSpeedBar();
		}
	}

	SetWindowBusy(~0, FALSE);
	BeginScreenNotify();
}

void EjectDisk (void) {
	struct Window *window;
	Object *listbrowser;
	struct Node *node;
	LONG unit_num;
	ULONG disk_type;

	window = NULL;
	GetAttr(WINDOW_Window, Gui.windows[WID_MAIN], (Tag *)&window);
	listbrowser = Gui.gadgets[GID_DRIVELIST];

	node = NULL;
	GetAttr(LISTBROWSER_SelectedNode, Gui.gadgets[GID_DRIVELIST], (Tag *)&node);
	unit_num = GetUnitNumber(node);
	if (unit_num == -1) {
		return;
	}

	disk_type = DITYPE_NONE;
	UnitControl(unit_num,
		DITAG_Filename,			NULL,
		DITAG_DiskImageType,	&disk_type,
		TAG_END);
	if (disk_type == DITYPE_NONE) {
		SetAttrs(listbrowser,
			LISTBROWSER_Labels,		~0,
			TAG_END);

		SetListBrowserNodeAttrs(node,
			LBNA_Column,			DRIVE_COL_DISKIMAGE,
			LBNCA_Text,				GetString(&LocaleInfo, MSG_NO_DISK),
			TAG_END);

		SetGadgetAttrs(GA(listbrowser), window, NULL,
			LISTBROWSER_Labels,		Gui.lists[LID_DRIVELIST],
			TAG_END);

		UpdateSpeedBar();
	}
}

void ToggleWriteProtect (void) {
	struct Window *window;
	Object *listbrowser;
	struct Node *node;
	LONG unit_num;
	Object *image;
	BOOL writeprotect;

	window = NULL;
	GetAttr(WINDOW_Window, Gui.windows[WID_MAIN], (Tag *)&window);
	listbrowser = Gui.gadgets[GID_DRIVELIST];

	node = NULL;
	GetAttr(LISTBROWSER_SelectedNode, listbrowser, (Tag *)&node);
	unit_num = GetUnitNumber(node);
	if (unit_num == -1) {
		return;
	}

	image = NULL;
	writeprotect = FALSE;
	GetListBrowserNodeAttrs(node,
		LBNA_Column,			DRIVE_COL_WP,
		LBNCA_Image,			&image,
		TAG_END);
	UnitControl(unit_num,
		DITAG_WriteProtect,		!image,
		DITAG_GetWriteProtect,	&writeprotect,
		TAG_END);
	if (writeprotect == !image) {
		SetAttrs(listbrowser,
			LISTBROWSER_Labels,		~0,
			TAG_END);

		SetListBrowserNodeAttrs(node,
			LBNA_Column,			DRIVE_COL_WP,
			LBNCA_Image,			writeprotect ? Gui.images[IID_LIST_WRITEPROTECTED] : NULL,
			TAG_END);

		SetGadgetAttrs(GA(listbrowser), window, NULL,
			LISTBROWSER_Labels,		Gui.lists[LID_DRIVELIST],
			TAG_END);
	}
}
