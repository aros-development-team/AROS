#ifndef SCREENPANEL_CLASS_H
#define SCREENPANEL_CLASS_H

/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2010, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include "psi.h"

#define MUIM_ScreenPanel_Create       (TAGBASE_STUNTZI | 0x1060)
#define MUIM_ScreenPanel_Copy         (TAGBASE_STUNTZI | 0x1061)
#define MUIM_ScreenPanel_Delete       (TAGBASE_STUNTZI | 0x1062)
#define MUIM_ScreenPanel_Edit         (TAGBASE_STUNTZI | 0x1063)
#define MUIM_ScreenPanel_Finish       (TAGBASE_STUNTZI | 0x1064)
#define MUIM_ScreenPanel_CloseWindows (TAGBASE_STUNTZI | 0x1065)
#define MUIM_ScreenPanel_SetStates    (TAGBASE_STUNTZI | 0x1066)
#define MUIM_ScreenPanel_Open         (TAGBASE_STUNTZI | 0x1067)
#define MUIM_ScreenPanel_Close        (TAGBASE_STUNTZI | 0x1068)
#define MUIM_ScreenPanel_Jump         (TAGBASE_STUNTZI | 0x1069)
#define MUIM_ScreenPanel_Update       (TAGBASE_STUNTZI | 0x106a)
#define MUIM_ScreenPanel_Foo          (TAGBASE_STUNTZI | 0x106b)

struct MUIP_ScreenPanel_Finish { STACKED ULONG MethodID; Object *win; LONG ok; };

struct MUI_CustomClass *CL_ScreenPanel ;

VOID ScreenPanel_Init(VOID);
VOID ScreenPanel_Exit(VOID);

#endif
