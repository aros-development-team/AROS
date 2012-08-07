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
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/diskimage.h>
#include <clib/alib_protos.h>

void SetDeviceType (void) {
	struct Window *window;
	Object *listbrowser;
	struct Node *node;
	LONG unit_num;
	STRPTR window_title;
	UBYTE old_device_type;
	UBYTE new_device_type;
	ULONG chooser_sel;

	window = NULL;
	GetAttr(WINDOW_Window, Gui.windows[WID_MAIN], (Tag *)&window);
	listbrowser = Gui.gadgets[GID_DRIVELIST];

	node = NULL;
	GetAttr(LISTBROWSER_SelectedNode, Gui.gadgets[GID_DRIVELIST], (Tag *)&node);
	unit_num = GetUnitNumber(node);
	if (unit_num == -1) {
		return;
	}

	window_title = ASPrintf(GetString(&LocaleInfo, MSG_SETDEVICETYPE_WND), unit_num);
	if (!window_title) {
		return;
	}

	StopScreenNotify();
	SetWindowBusy(~0, TRUE);

	old_device_type = DG_DIRECT_ACCESS;
	UnitControl(unit_num,
		DITAG_GetDeviceType,	&old_device_type,
		TAG_END);
	chooser_sel = 0;
	switch (old_device_type) {
		case DG_DIRECT_ACCESS: chooser_sel = 0; break;
		case DG_CDROM: chooser_sel = 1; break;
	}
	SetAttrs(Gui.windows[WID_SETDEVICETYPE],
		WA_Title,			window_title,
		WINDOW_Position,	window ? WPOS_CENTERWINDOW : WPOS_CENTERMOUSE,
		WINDOW_RefWindow,	window,
		TAG_END);
	SetAttrs(Gui.gadgets[GID_DEVICETYPECHOOSER],
		CHOOSER_Selected,	chooser_sel,
		TAG_END);

	if (ShowWindow(WID_SETDEVICETYPE)) {
		BOOL done = FALSE;
		ULONG sigs, res;
		UWORD code;
		while (!done) {
			GetAttr(WINDOW_SigMask, Gui.windows[WID_SETDEVICETYPE], &sigs);
			Wait(sigs);
			while ((res = RA_HandleInput(Gui.windows[WID_SETDEVICETYPE], &code)) != WMHI_LASTMSG) {
				switch (res & WMHI_CLASSMASK) {
					case WMHI_CLOSEWINDOW:
						done = TRUE;
						break;
					case WMHI_GADGETUP:
						switch (res & WMHI_GADGETMASK) {
							case GID_SETDEVICETYPE_SAVE:
								chooser_sel = 0;
								GetAttr(CHOOSER_Selected, Gui.gadgets[GID_DEVICETYPECHOOSER], &chooser_sel);
								new_device_type = DG_DIRECT_ACCESS;
								switch (chooser_sel) {
									case 0: new_device_type = DG_DIRECT_ACCESS; break;
									case 1: new_device_type = DG_CDROM; break;
								}
								if (new_device_type != old_device_type) {
									UBYTE device_type = DG_DIRECT_ACCESS;
									ULONG device_icon = IID_LIST_DISK;

									UnitControl(unit_num,
										DITAG_SetDeviceType,	new_device_type,
										DITAG_GetDeviceType,	&device_type,
										TAG_END);
									
									SetAttrs(listbrowser,
										LISTBROWSER_Labels,		~0,
										TAG_END);

									if (device_type == DG_CDROM) {
										device_icon = IID_LIST_CDROM;
									}
									
									SetListBrowserNodeAttrs(node,
										LBNA_Column,			DRIVE_COL_ICON,
										LBNCA_Image,			Gui.images[device_icon],
										TAG_END);

									SetGadgetAttrs(GA(listbrowser), window, NULL,
										LISTBROWSER_Labels,		Gui.lists[LID_DRIVELIST],
										TAG_END);
								}
								/* fall through */
							case GID_SETDEVICETYPE_CANCEL:
								done = TRUE;
								break;
						}
						break;
				}
			}
		}
		HideWindow(WID_SETDEVICETYPE);
	}
	FreeVec(window_title);

	SetWindowBusy(~0, FALSE);
	BeginScreenNotify();
}
