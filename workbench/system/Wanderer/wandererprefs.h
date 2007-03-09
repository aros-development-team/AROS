#ifndef _WANDERERPREFS_H_
#define _WANDERERPREFS_H_

/*
    Copyright  2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier Base ********************************************************/
#define MUIB_WandererPrefs                      (TAG_USER | 0x12000000)

/*** Public Attributes ******************************************************/
#define MUIA_WandererPrefs_WorkbenchBackground  (MUIB_WandererPrefs | 0x00000000) /* -SG */
#define MUIA_WandererPrefs_DrawerBackground     (MUIB_WandererPrefs | 0x00000001) /* -SG */
#define MUIA_WandererPrefs_NavigationMethod     (MUIB_WandererPrefs | 0x00000002) /* -SG */
#define MUIA_WandererPrefs_Toolbar_Enabled      (MUIB_WandererPrefs | 0x00000003) /* -SG */
#define MUIA_WandererPrefs_Icon_ListMode        (MUIB_WandererPrefs | 0x00000004) /* -SG */
#define MUIA_WandererPrefs_Icon_TextMode        (MUIB_WandererPrefs | 0x00000005) /* -SG */
#define MUIA_WandererPrefs_Icon_TextMaxLen      (MUIB_WandererPrefs | 0x00000006) /* -SG */
#define MUIA_WandererPrefs_DoubleBuffered       (MUIB_WandererPrefs | 0x00000007) /* -SG */

#define MUIA_WandererPrefs_BackgroundRenderMode       (MUIB_WandererPrefs | 0x000000F1) /* -SG */

#define WandererPrefs_BackgroundRenderMode_Tiled        0 // Default
#define WandererPrefs_BackgroundRenderMode_Scale       1 // Scaled to screen

/*** Public Methods *********************************************************/
#define MUIM_WandererPrefs_Reload               (MUIB_WandererPrefs | 0x00000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *WandererPrefs_CLASS;

/*** Macros *****************************************************************/
#define WandererPrefsObject BOOPSIOBJMACRO_START(WandererPrefs_CLASS->mcc_Class)

#endif /* _WANDERERPREFS_H_ */
