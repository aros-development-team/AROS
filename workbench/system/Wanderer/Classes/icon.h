#ifndef _WANDERER_CLASSES_ICON_H
#define _WANDERER_CLASSES_ICON_H

/*
    Copyright  2002-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/intuitionbase.h>
#include <intuition/classusr.h>
#include <clib/alib_protos.h>
#include <utility/utility.h>
#include <dos/dosextens.h>
#include <libraries/mui.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>

#include "icon_attributes.h"

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Icon "Icon.mui"

/*** Methods ****************************************************************/
#define MUIM_Icon_Clear             (MUIB_Icon | 0x00000000)
#define MUIM_Icon_Update            (MUIB_Icon | 0x00000001)
#define MUIM_Icon_RethinkDimensions (MUIB_Icon | 0x00000002)
#define MUIM_Icon_CreateEntry       (MUIB_Icon | 0x00000010) /* returns 0 For Failure or (struct IconEntry *) */
#define MUIM_Icon_DestroyEntry      (MUIB_Icon | 0x00000011)
#define MUIM_Icon_DrawEntry         (MUIB_Icon | 0x00000012)
#define MUIM_Icon_DrawEntryLabel    (MUIB_Icon | 0x00000013)
#define MUIM_Icon_SelectAll         (MUIB_Icon | 0x00000020)
#define MUIM_Icon_UnselectAll       (MUIB_Icon | 0x00000021)
#define MUIM_Icon_GetIconPrivate    (MUIB_Icon | 0x00000022)
#define MUIM_Icon_NextIcon          (MUIB_Icon | 0x00000025)
#define MUIM_Icon_Sort              (MUIB_Icon | 0x00000031)
#define MUIM_Icon_CoordsSort        (MUIB_Icon | 0x00000032)
#define MUIM_Icon_PositionIcons     (MUIB_Icon | 0x00000033)
#define MUIM_Icon_ViewIcon          (MUIB_Icon | 0x00000034)

struct MUIP_Icon_CreateEntry        {STACKED ULONG MethodID; STACKED char *filename; STACKED char *label; STACKED struct FileInfoBlock *fib; STACKED struct DiskObject *icon_dob; STACKED ULONG type;};/* void *udata; More file attrs to add };*/
struct MUIP_Icon_DestroyEntry       {STACKED ULONG MethodID; STACKED struct IconEntry *icon;};
struct MUIP_Icon_DrawEntry          {STACKED ULONG MethodID; STACKED struct IconEntry *icon; STACKED IPTR drawmode;};
struct MUIP_Icon_DrawEntryLabel     {STACKED ULONG MethodID; STACKED struct IconEntry *icon; STACKED IPTR drawmode;};
struct MUIP_Icon_GetIconPrivate     {STACKED ULONG MethodID; STACKED struct Icon_Entry *entry;}; /* *entry must be a valid Icon */

extern const struct __MUIBuiltinClass _MUI_Icon_desc; /* PRIV */

#ifdef __AROS__
#define IconObject       MUIOBJMACRO_START(MUIC_Icon)
#else
#define IconObject         NewObject(Icon_Class->mcc_Class, NULL
#endif

#endif /* _WANDERER_CLASSES_ICON_H */
