#ifndef EDITPANEL_CLASS_H
#define EDITPANEL_CLASS_H

/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2010, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include "psi.h"

#define MUIM_EditPanel_SetScreen (TAGBASE_STUNTZI | 0x1030)
#define MUIM_EditPanel_GetScreen (TAGBASE_STUNTZI | 0x1031)
#define MUIM_EditPanel_Update    (TAGBASE_STUNTZI | 0x1032)
#define MUIM_EditPanel_DefColors (TAGBASE_STUNTZI | 0x1035)
/*
#define MUIM_EditPanel_ToggleForeign  (TAGBASE_STUNTZI | 0x1036)
*/

struct MUIP_EditPanel_SetScreen { STACKED ULONG MethodID; STACKED struct MUI_PubScreenDesc *desc; };
struct MUIP_EditPanel_GetScreen { STACKED ULONG MethodID; STACKED struct MUI_PubScreenDesc *desc; };
struct MUIP_EditPanel_Update    { STACKED ULONG MethodID; STACKED LONG level; };
struct MUIP_EditPanel_DefColors { STACKED ULONG MethodID; STACKED LONG nr; };

struct MUI_CustomClass *CL_EditPanel;

VOID EditPanel_Init(VOID);
VOID EditPanel_Exit(VOID);

extern struct NewMenu PaletteMenu[];
extern CONST_STRPTR CYA_EditPages[];
extern CONST_STRPTR CYA_Overscan[];

#endif
