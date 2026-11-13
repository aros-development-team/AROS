#ifndef MUI_NLISTTREE_priv_MCP_H
#define MUI_NLISTTREE_priv_MCP_H

/***************************************************************************

 NListtree.mcc - New Listtree MUI Custom Class
 Copyright (C) 1999-2001 by Carsten Scholling
 Copyright (C) 2001-2014 NList Open Source Team

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
#include "../nlisttree_mcc/private.h"
#include "default-align.h"

#include "Debug.h"

#include <mcc_common.h>

#define MCPMAXRAWBUF 64

struct NListtreeP_Data
{
	Object *NLT_Sample;
  Object *BT_Sample_Expand;
  Object *BT_Sample_Collapse;
  Object *GR_Prefs;
	Object *PI_ImageClosed;
	Object *PI_ImageOpen;
  Object *PI_ImageFolder;
  Object *PP_LinePen;
  Object *PP_ShadowPen;
  Object *PP_GlowPen;
  Object *CY_LineType;
  Object *SL_IndentWidth;
  Object *CH_RememberStatus;
  Object *CH_OpenAutoScroll;
  Object *CH_UseFolderImage;
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

#endif /* MUI_NLISTTREE_priv_MCP_H */
