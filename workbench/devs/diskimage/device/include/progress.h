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

#ifndef PROGRESS_H
#define PROGRESS_H

#ifndef __AROS__
#include <classes/window.h>
#include <gadgets/fuelgauge.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/fuelgauge.h>

#define GID_PROGRESS_CANCEL 1

struct ProgressBar {
	struct Screen *pb_Screen;
	Object *pb_Window;
	Object *pb_ProgressBar;

	struct DiskImageBase *pb_LibBase;
	struct Library *pb_WindowBase;
	struct Library *pb_LayoutBase;
	struct Library *pb_FuelGaugeBase;
};
#else
struct ProgressBar {
	ULONG pb_Dummy;
};
#endif

struct ProgressBar *CreateProgressBar (APTR Self, struct DiskImageUnit *unit, BOOL stop);
void DeleteProgressBar (APTR Self, struct ProgressBar *pb);
BOOL ProgressBarInput (APTR Self, struct ProgressBar *pb);
void SetProgressBarAttrsA (APTR Self, struct ProgressBar *pb, const struct TagItem *tags);
VARARGS68K void SetProgressBarAttrs (APTR Self, struct ProgressBar *pb, ...);

#endif
