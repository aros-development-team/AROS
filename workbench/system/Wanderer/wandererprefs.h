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

/*** Public Methods *********************************************************/
#define MUIM_WandererPrefs_Reload                            (MUIB_WandererPrefs | 0x00000000)

#define MUIM_WandererPrefs_ViewSettings_GetNotifyObject      (MUIB_WandererPrefs | 0x000000E0) /* --G */
#define MUIM_WandererPrefs_ViewSettings_GetAttribute         (MUIB_WandererPrefs | 0x000000E1) /* --G */
struct  MUIP_WandererPrefs_ViewSettings_GetNotifyObject      {ULONG MethodID; char *Background_Name;};
struct  MUIP_WandererPrefs_ViewSettings_GetAttribute         {ULONG MethodID; char *Background_Name; ULONG AttributeID;};

#define WP_GLOBALTAGCOUNT 9

/*** Variables **************************************************************/
extern struct MUI_CustomClass *WandererPrefs_CLASS;

/*** Macros *****************************************************************/
#define WandererPrefsObject BOOPSIOBJMACRO_START(WandererPrefs_CLASS->mcc_Class)

#endif /* _WANDERERPREFS_H_ */
