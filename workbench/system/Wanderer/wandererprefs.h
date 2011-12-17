#ifndef _WANDERERPREFS_H_
#define _WANDERERPREFS_H_

/*
    Copyright  2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier Base ********************************************************/
#define MUIB_WandererPrefs                                   (TAG_USER | 0x12000000)

/*** Public Attributes ******************************************************/
#define MUIA_WandererPrefs_Processing                        (MUIB_WandererPrefs | 0x00000001) /* --G Set (TRUE) while processing/ cleared (FALSE) when complete*/
#define MUIA_WandererPrefs_DefaultStack                      (MUIB_WandererPrefs | 0x00000002) /* --- Only used in the global prefs - may be moved! */

#define Wanderer_Viewsetting_Workbench                       0
#define Wanderer_Viewsetting_Drawer                          1

#define Wanderer_VSF_SupportsBackfills                       (1<<0)
#define Wanderer_VSF_SupportsIcons                           (1<<1)

/*** Public Methods *********************************************************/
#define MUIM_WandererPrefs_Reload                            (MUIB_WandererPrefs | 0x00000000)
#define MUIM_WandererPrefs_ReloadFontPrefs                   (MUIB_WandererPrefs | 0x00000001)

#define MUIM_WandererPrefs_ViewSettings_GetNotifyObject      (MUIB_WandererPrefs | 0x000000E0) /* --G */
#define MUIM_WandererPrefs_ViewSettings_GetAttribute         (MUIB_WandererPrefs | 0x000000E1) /* --G */
struct  MUIP_WandererPrefs_ViewSettings_GetNotifyObject      {STACKED ULONG MethodID; STACKED char *Background_Name;};
struct  MUIP_WandererPrefs_ViewSettings_GetAttribute         {STACKED ULONG MethodID; STACKED char *Background_Name; STACKED ULONG AttributeID;};

/*** Variables **************************************************************/
extern struct MUI_CustomClass *WandererPrefs_CLASS;

/*** Macros *****************************************************************/
#ifdef __AROS__
#define WandererPrefsObject BOOPSIOBJMACRO_START(WandererPrefs_CLASS->mcc_Class)
#else
#define WandererPrefsObject NewObject(WandererPrefs_CLASS->mcc_Class, NULL
#endif

#endif /* _WANDERERPREFS_H_ */
