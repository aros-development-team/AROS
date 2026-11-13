#ifndef COLOREDIT_CLASS_H
#define COLOREDIT_CLASS_H

/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2010, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include "psi.h"

#define MUIM_ColorEdit_SetColors (TAGBASE_STUNTZI | 0x1082)
#define MUIM_ColorEdit_GetColors (TAGBASE_STUNTZI | 0x1083)

struct MUIP_ColorEdit_SetColors { STACKED ULONG MethodID; STACKED struct MUI_RGBcolor *palette; STACKED BYTE *syspens; STACKED struct MUI_PenSpec *muipens; };
struct MUIP_ColorEdit_GetColors { STACKED ULONG MethodID; STACKED struct MUI_RGBcolor *palette; STACKED BYTE *syspens; STACKED struct MUI_PenSpec *muipens; };

#endif
