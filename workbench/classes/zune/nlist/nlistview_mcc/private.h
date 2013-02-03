#ifndef MUI_NListview_priv_MCC_H
#define MUI_NListview_priv_MCC_H

/***************************************************************************

 NListview.mcc - New Listview MUI Custom Class
 Registered MUI class, Serial Number: 1d51 (0x9d510020 to 0x9d51002F)

 Copyright (C) 1996-2004 by Gilles Masson,
                            Carsten Scholling <aphaso@aphaso.de>,
                            Przemyslaw Grunchala,
                            Sebastian Bauer <sebauer@t-online.de>,
                            Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#include "amiga-align.h"
#include "NListview_mcc.h"
#include "default-align.h"

#include "Debug.h"

#include <mcc_common.h>

struct NLVData
{
  Object *LI_NList;
  Object *PR_Vert;
  Object *PR_Horiz;
  Object *Group;
  BOOL   Vert_Attached;
  BOOL   Horiz_Attached;
  LONG   VertSB;
  LONG   HorizSB;
  LONG   Vert_ScrollBar;
  LONG   Horiz_ScrollBar;
  BOOL   sem;
  BOOL   SETUP;
  UBYTE  ControlChar;
  struct MUI_EventHandlerNode eh;
};

/// xget()
//  Gets an attribute value from a MUI object
IPTR xget(Object *obj, const IPTR attr);
#if defined(__GNUC__)
  // please note that we do not evaluate the return value of GetAttr()
  // as some attributes (e.g. MUIA_Selected) always return FALSE, even
  // when they are supported by the object. But setting b=0 right before
  // the GetAttr() should catch the case when attr doesn't exist at all
  #define xget(OBJ, ATTR) ({IPTR b=0; GetAttr(ATTR, OBJ, &b); b;})
#endif
///

#endif /* MUI_NListview_priv_MCC_H */
