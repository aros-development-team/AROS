/***************************************************************************

 NBalance.mcc - New Balance MUI Custom Class
 Copyright (C) 2008 by NList Open Source Team

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

#ifndef PRIVATE_H
#define PRIVATE_H

/* system includes */
#include <dos/exall.h>
#include <exec/types.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

/* mcc includes */
#include <mcc_common.h>

/* local includes */
#include "Debug.h"
#include "NBalance.h"
#include "Pointer.h"

/* private structures */
struct InstData
{
  ULONG groupType;
  BOOL mouseSelectDown;
  BOOL mouseOverObject;
  BOOL ignoreNextHidePointer;

  LONG pointerType;

  Object *horizSizePointerObj;
  Object *vertSizePointerObj;
  enum PointerType activeCustomPointer;

  struct MUI_EventHandlerNode ehnode;
};

/* macros */
#define _id(obj) (muiNotifyData(obj)->mnd_ObjectID)
#define _between(a,x,b) ((x)>=(a) && (x)<=(b))
#define _isinobject(x,y) (_between(_mleft(obj),(x),_mright(obj)) && _between(_mtop(obj),(y),_mbottom(obj)))

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

/* prototypes */
IPTR mNew(struct IClass *cl, Object *obj, struct opSet *set);
IPTR mSet(struct IClass *cl, Object *obj, Msg msg);
IPTR mGet(struct IClass *cl, Object *obj, Msg msg);
IPTR mSetup(struct IClass *cl, Object *obj, struct MUI_RenderInfo *rinfo);
IPTR mCleanup(struct IClass *cl, Object *obj, Msg msg);
IPTR mHide(struct IClass *cl, Object *obj, Msg msg);
IPTR mHandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg);

#endif /* PRIVATE_H */
