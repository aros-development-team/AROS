/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id: private.h,v 1.1 2005/04/21 20:52:04 damato Exp $

***************************************************************************/

#ifndef HOTKEYSTRING_MCC_PRIV_H
#define HOTKEYSTRING_MCC_PRIV_H

#include <proto/muimaster.h>

#ifndef __AROS__
#include <muiextra.h>
#else
#include <libraries/mui.h>
#endif
#include <mcc_common.h>
#include <mcc_debug.h>

#ifndef __AROS__
#include "HotkeyString_mcc.h"
#include "BetterString_mcc.h"
#else
#include <MUI/HotkeyString_mcc.h>
#include <MUI/BetterString_mcc.h>
#endif

struct InstData
{
	ULONG Flags;
	struct MUI_EventHandlerNode EventNode;
};

#define FLG_Active    (1<<0)
#define FLG_Backspace (1<<1)
#define FLG_Snoop     (1<<2)

// prototypes
ULONG HandleInput(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg);

#endif /* HOTKEYSTRING_MCC_PRIV_H */
