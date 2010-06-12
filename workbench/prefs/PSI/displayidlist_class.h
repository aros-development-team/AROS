#ifndef DISPLAYIDLIST_CLASS_H
#define DISPLAYIDLIST_CLASS_H
/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2010, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include "psi.h"

#define MUIA_DispIDlist_CurrentID (TAGBASE_STUNTZI | 0x1020)
#define MUIA_DispIDlist_Quiet     (TAGBASE_STUNTZI | 0x1021)
#define MUIM_DispIDlist_Change    (TAGBASE_STUNTZI | 0x1022)

struct MUI_CustomClass *CL_DispIDlist;

VOID DispIDlist_Init(VOID);
VOID DispIDlist_Exit(VOID);

#endif
