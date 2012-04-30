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

#include "support.h"

void TranslateMenus (struct LocaleInfo *li, struct NewMenu *nm) {
	while (nm->nm_Type != NM_END) {
		if (nm->nm_Label && nm->nm_Label != NM_BARLABEL)
			nm->nm_Label = (STRPTR)GetString(li, (LONG)nm->nm_Label);
		nm++;
	}
}

void TranslateArray (struct LocaleInfo *li, CONST_STRPTR *array) {
	while (*array) {
		if (*array != (CONST_STRPTR)-1) {
			*array = GetString(li, (int32)*array);
		}
		array++;
	}
}

#ifndef __AROS__
void TranslateHints (struct LocaleInfo *li, struct HintInfo *hi) {
	while (hi->hi_GadgetID != -1) {
		hi->hi_Text = (STRPTR)GetString(li, (LONG)hi->hi_Text);
		hi++;
	}
}

void TranslateColumnTitles (struct LocaleInfo *li, struct ColumnInfo *ci) {
	while (ci->ci_Width > 0) {
		if ((LONG)ci->ci_Title != -1)
			ci->ci_Title = (STRPTR)GetString(li, (LONG)ci->ci_Title);
		ci++;
	}
}
#endif
