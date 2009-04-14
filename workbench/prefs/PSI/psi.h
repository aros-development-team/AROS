/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <exec/memory.h>
#include <prefs/prefhdr.h>
#include <datatypes/pictureclass.h>
#include <libraries/locale.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#define MUI_OBSOLETE
#include <libraries/mui.h>
#include <libraries/muiscreen.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/muimaster.h>
#include <proto/muiscreen.h>
#include <proto/alib.h>
#include <proto/asl.h>
#include <aros/debug.h>


#define CATCOMP_NUMBERS
#define CATCOMP_ARRAY
#include "strings.h"

#define USE_PSI_SIZES_BODY
#define USE_PSI_SIZES_COLORS
#include "psi_sizes.bh"

#define USE_PSI_COLORS_BODY
#include "psi_colors.bh"

#define USE_PSI_FREQS_BODY
#include "psi_freqs.bh"

/****************************************************************************/
/* Some Definitions                                                         */
/****************************************************************************/

#define MUISERIALNR_STUNTZI 1
#define TAGBASE_STUNTZI (TAG_USER | ( MUISERIALNR_STUNTZI << 16))

#define MUIA_DispIDinfo_ID            (TAGBASE_STUNTZI | 0x1010)

#define MUIA_DispIDlist_CurrentID     (TAGBASE_STUNTZI | 0x1020)
#define MUIA_DispIDlist_Quiet         (TAGBASE_STUNTZI | 0x1021)
#define MUIM_DispIDlist_Change        (TAGBASE_STUNTZI | 0x1022)

#define MUIM_EditPanel_SetScreen      (TAGBASE_STUNTZI | 0x1030)
#define MUIM_EditPanel_GetScreen      (TAGBASE_STUNTZI | 0x1031)
#define MUIM_EditPanel_Update         (TAGBASE_STUNTZI | 0x1032)
#define MUIM_EditPanel_DefColors      (TAGBASE_STUNTZI | 0x1035)
/*
#define MUIM_EditPanel_ToggleForeign  (TAGBASE_STUNTZI | 0x1036)
*/

#define MUIA_EditWindow_Title         (TAGBASE_STUNTZI | 0x1040)
#define MUIA_EditWindow_Originator    (TAGBASE_STUNTZI | 0x1041)
#define MUIM_EditWindow_Close         (TAGBASE_STUNTZI | 0x1042)

#define MUIM_ScreenList_Save          (TAGBASE_STUNTZI | 0x1050)
#define MUIM_ScreenList_Load          (TAGBASE_STUNTZI | 0x1051)
#define MUIM_ScreenList_Find          (TAGBASE_STUNTZI | 0x1052)

#define MUIM_ScreenPanel_Create       (TAGBASE_STUNTZI | 0x1060)
#define MUIM_ScreenPanel_Copy         (TAGBASE_STUNTZI | 0x1061)
#define MUIM_ScreenPanel_Delete       (TAGBASE_STUNTZI | 0x1062)
#define MUIM_ScreenPanel_Edit         (TAGBASE_STUNTZI | 0x1063)
#define MUIM_ScreenPanel_Finish       (TAGBASE_STUNTZI | 0x1064)
#define MUIM_ScreenPanel_CloseWindows (TAGBASE_STUNTZI | 0x1065)
#define MUIM_ScreenPanel_SetStates    (TAGBASE_STUNTZI | 0x1066)
#define MUIM_ScreenPanel_Open         (TAGBASE_STUNTZI | 0x1067)
#define MUIM_ScreenPanel_Close        (TAGBASE_STUNTZI | 0x1068)
#define MUIM_ScreenPanel_Jump         (TAGBASE_STUNTZI | 0x1069)
#define MUIM_ScreenPanel_Update       (TAGBASE_STUNTZI | 0x106a)
#define MUIM_ScreenPanel_Foo          (TAGBASE_STUNTZI | 0x106b)

#define MUIM_MainWindow_Finish        (TAGBASE_STUNTZI | 0x1070)
#define MUIM_MainWindow_About         (TAGBASE_STUNTZI | 0x1071)
#define MUIM_MainWindow_Restore       (TAGBASE_STUNTZI | 0x1072)
#define MUIM_MainWindow_Open          (TAGBASE_STUNTZI | 0x1073)
#define MUIM_MainWindow_SaveAs        (TAGBASE_STUNTZI | 0x1074)

#define MUIM_ColorEdit_SetColors      (TAGBASE_STUNTZI | 0x1082)
#define MUIM_ColorEdit_GetColors      (TAGBASE_STUNTZI | 0x1083)

struct MUIP_EditPanel_SetScreen { STACKED ULONG MethodID; STACKED struct MUI_PubScreenDesc *desc; };
struct MUIP_EditPanel_GetScreen { STACKED ULONG MethodID; STACKED struct MUI_PubScreenDesc *desc; };
struct MUIP_EditPanel_Update    { STACKED ULONG MethodID; STACKED LONG level; };
struct MUIP_EditPanel_DefColors { STACKED ULONG MethodID; STACKED LONG nr; };

struct MUIP_ScreenList_Save     { STACKED ULONG MethodID; STACKED char *name; };
struct MUIP_ScreenList_Load     { STACKED ULONG MethodID; STACKED char *name; STACKED LONG clear; };
struct MUIP_ScreenList_Find     { STACKED ULONG MethodID; STACKED char *name; STACKED struct MUI_PubScreenDesc **desc; };

struct MUIP_ScreenPanel_Finish  { STACKED ULONG MethodID; Object *win; LONG ok; };

struct MUIP_MainWindow_Finish   { STACKED ULONG MethodID; STACKED LONG level; };
struct MUIP_MainWindow_Restore  { STACKED ULONG MethodID; STACKED LONG envarc; };
struct MUIP_MainWindow_Open     { STACKED ULONG MethodID; STACKED LONG append; };

struct MUIP_ColorEdit_SetColors  { STACKED ULONG MethodID; STACKED struct MUI_RGBcolor *palette; STACKED BYTE *syspens; STACKED struct MUI_PenSpec *muipens; };
struct MUIP_ColorEdit_GetColors  { STACKED ULONG MethodID; STACKED struct MUI_RGBcolor *palette; STACKED BYTE *syspens; STACKED struct MUI_PenSpec *muipens; };

#define RectangleWidth(r)  ((r).MaxX-(r).MinX+1)
#define RectangleHeight(r) ((r).MaxY-(r).MinY+1)

#define SYSPEN_OFFSET   1
#define MUIPEN_OFFSET   1

#define ForEntries(list,entry,succ) for (entry=(APTR)((struct Node *)(((struct List *)list)->lh_Head));succ=(APTR)((struct Node *)entry)->ln_Succ;entry=(APTR)succ)
