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

/* ansi includes */
#include <string.h>

/* system includes */
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/locale.h>

/* local includes */
#include "NBalance.h"
#include "private.h"
#include "Debug.h"

DISPATCHER(_Dispatcher)
{
  IPTR result;

  ENTER();

  switch(msg->MethodID)
  {
    case OM_NEW:
      result = mNew(cl, obj, (struct opSet *)msg);
    break;

    case OM_SET:
      result = mSet(cl, obj, msg);
    break;

    case OM_GET:
      result = mGet(cl, obj, msg);
    break;

    case MUIM_Setup:
      result = mSetup(cl, obj, (struct MUI_RenderInfo *)msg);
    break;

    case MUIM_Cleanup:
      result = mCleanup(cl, obj, msg);
    break;

    case MUIM_Hide:
      result = mHide(cl, obj, msg);
    break;

    case MUIM_HandleEvent:
      result = mHandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg);
    break;

    default:
      result = DoSuperMethodA(cl, obj, msg);
    break;
  }

  RETURN(result);
  return result;
}

