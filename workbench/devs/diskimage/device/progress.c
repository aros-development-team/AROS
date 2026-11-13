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

#include "diskimage_device.h"
#include "progress.h"
#include "device_locale.h"
#include <SDI_stdarg.h>

#ifndef __AROS__
#include <reaction/reaction_macros.h>

struct ProgressBar *CreateProgressBar (APTR Self, struct DiskImageUnit *unit, BOOL stop) {
	struct DiskImageBase *libBase = unit->LibBase;
	struct Screen *scr = unit->Screen;
	struct Library *SysBase = libBase->SysBase;
	struct ProgressBar *pb;
	pb = AllocMem(sizeof(struct ProgressBar), MEMF_CLEAR);
	if (pb) {
		pb->pb_Screen = scr;
		pb->pb_LibBase = libBase;

		pb->pb_WindowBase = OpenLibrary("window.class", MIN_OS_VERSION);
		pb->pb_LayoutBase = OpenLibrary("gadgets/layout.gadget", MIN_OS_VERSION);
		pb->pb_FuelGaugeBase = OpenLibrary("gadgets/fuelgauge.gadget", MIN_OS_VERSION);

		if (pb->pb_WindowBase && pb->pb_LayoutBase && pb->pb_FuelGaugeBase) {
			struct Library *IntuitionBase = libBase->IntuitionBase;
			struct Library *WindowBase = pb->pb_WindowBase;
			struct Library *LayoutBase = pb->pb_LayoutBase;
			struct Library *FuelGaugeBase = pb->pb_FuelGaugeBase;
			struct LocaleInfo *li = &libBase->LocaleInfo;

			pb->pb_Window = WindowObject,
				WA_Title,			unit->Node.ln_Name,
				WA_Flags,			(stop ? WFLG_CLOSEGADGET : 0)|WFLG_DRAGBAR|WFLG_DEPTHGADGET
									|WFLG_NOCAREREFRESH,
				WA_IDCMP,			stop ? (IDCMP_CLOSEWINDOW|IDCMP_GADGETUP) : 0,
				WA_PubScreen,		scr,
				WINDOW_Position,	WPOS_CENTERSCREEN,
				WINDOW_Layout,		VLayoutObject,
					LAYOUT_HorizAlignment,	LALIGN_CENTER,
					LAYOUT_AddChild,	pb->pb_ProgressBar = FuelGaugeObject,
						FUELGAUGE_Justification,	FGJ_CENTER,
						FUELGAUGE_Ticks,			5,
						FUELGAUGE_ShortTicks,		TRUE,
					End,
					CHILD_MinWidth,		300,
					CHILD_MinHeight,	40,
					LAYOUT_AddChild,	ButtonObject,
						GA_ID,			GID_PROGRESS_CANCEL,
						GA_RelVerify,	stop,
						GA_Text,		GetString(li, MSG_CANCEL_GAD),
						GA_Disabled,	!stop,
					End,
					CHILD_WeightedWidth,	0,
				End,
			End;

			if (pb->pb_Window && RA_OpenWindow(pb->pb_Window)) {
				return pb;
			}
		}

		DeleteProgressBar(Self, pb);
	}
	return NULL;
}

void DeleteProgressBar (APTR Self, struct ProgressBar *pb) {
	if (pb) {
		struct DiskImageBase *libBase = pb->pb_LibBase;
		struct Library *SysBase = libBase->SysBase;
		struct Library *IntuitionBase = libBase->IntuitionBase;

		DisposeObject(pb->pb_Window);

		CloseLibrary(pb->pb_FuelGaugeBase);
		CloseLibrary(pb->pb_LayoutBase);
		CloseLibrary(pb->pb_WindowBase);

		FreeMem(pb, sizeof(struct ProgressBar));
	}
}

BOOL ProgressBarInput (APTR Self, struct ProgressBar *pb) {
	BOOL quit = FALSE;
	if (pb) {
		struct DiskImageBase *libBase = pb->pb_LibBase;
		struct Library *IntuitionBase = libBase->IntuitionBase;
		ULONG res;
		UWORD code;
		while (WMHI_LASTMSG != (res = RA_HandleInput(pb->pb_Window, &code))) {
			switch (res & WMHI_CLASSMASK) {
				case WMHI_CLOSEWINDOW:
					quit = TRUE;
					break;
				case WMHI_GADGETUP:
					switch (res & WMHI_GADGETMASK) {
						case GID_PROGRESS_CANCEL:
							quit = TRUE;
							break;
					}
					break;
			}
		}
	}
	return quit;
}

void SetProgressBarAttrsA (APTR Self, struct ProgressBar *pb, const struct TagItem *tags) {
	if (pb) {
		struct Library *IntuitionBase = pb->pb_LibBase->IntuitionBase;
		struct Window *win;
		GetAttr(WINDOW_Window, pb->pb_Window, (IPTR *)&win);
		SetGadgetAttrsA((struct Gadget *)pb->pb_ProgressBar, win, NULL, tags);
	}
}

VARARGS68K void SetProgressBarAttrs (APTR Self, struct ProgressBar *pb, ...) {
	VA_LIST tags;
	VA_START(tags, pb);
	SetProgressBarAttrsA(Self, pb, (const struct TagItem *)VA_ARG(tags, const struct TagItem *));
	VA_END(tags);
}

#else

struct ProgressBar *CreateProgressBar (APTR Self, struct DiskImageUnit *unit, BOOL stop) {
	return NULL;
}

void DeleteProgressBar (APTR Self, struct ProgressBar *pb) {
}

BOOL ProgressBarInput (APTR Self, struct ProgressBar *pb) {
	return FALSE;
}

void SetProgressBarAttrsA (APTR Self, struct ProgressBar *pb, const struct TagItem *tags) {
}

VARARGS68K void SetProgressBarAttrs (APTR Self, struct ProgressBar *pb, ...) {
}

#endif
