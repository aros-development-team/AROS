#ifndef EDITWINDOW_CLASS_H
#define EDITWINDOW_CLASS_H

/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2010, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include "psi.h"

#define MUIA_EditWindow_Title      (TAGBASE_STUNTZI | 0x1040)
#define MUIA_EditWindow_Originator (TAGBASE_STUNTZI | 0x1041)
#define MUIM_EditWindow_Close      (TAGBASE_STUNTZI | 0x1042)

struct MUI_CustomClass *CL_EditWindow;

VOID EditWindow_Init(VOID);
VOID EditWindow_Exit(VOID);

#endif
