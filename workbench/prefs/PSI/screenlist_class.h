#ifndef SCREENLIST_CLASS_H
#define SCREENLIST_CLASS_H

/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2010, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include "psi.h"

#define MUIM_ScreenList_Save (TAGBASE_STUNTZI | 0x1050)
#define MUIM_ScreenList_Load (TAGBASE_STUNTZI | 0x1051)
#define MUIM_ScreenList_Find (TAGBASE_STUNTZI | 0x1052)

struct MUIP_ScreenList_Save { STACKED ULONG MethodID; STACKED char *name; };
struct MUIP_ScreenList_Load { STACKED ULONG MethodID; STACKED char *name; STACKED LONG clear; };
struct MUIP_ScreenList_Find { STACKED ULONG MethodID; STACKED char *name; STACKED struct MUI_PubScreenDesc **desc; };

struct MUI_CustomClass *CL_ScreenList;

VOID ScreenList_Init(VOID);
VOID ScreenList_Exit(VOID);

#endif
