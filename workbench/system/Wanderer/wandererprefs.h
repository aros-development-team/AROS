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
#define MUIA_WandererPrefs_Processing                        (MUIB_WandererPrefs | 0x00000001) /* --G Set (TRUE) while processing/ cleared (FALSE) when complete*/
#define MUIA_WandererPrefs_NavigationMethod                  (MUIB_WandererPrefs | 0x00000002) /* -SG */
#define MUIA_WandererPrefs_Toolbar_Enabled                   (MUIB_WandererPrefs | 0x00000003) /* -SG */
#define MUIA_WandererPrefs_ShowNetworkBrowser                (MUIB_WandererPrefs | 0x00000011) /* -SG */
#define MUIA_WandererPrefs_ShowUserFolder                    (MUIB_WandererPrefs | 0x00000012) /* -SG */

#define MUIA_WandererPrefs_Icon_ListMode                     (MUIB_WandererPrefs | 0x000000D0) /* -SG */
#define MUIA_WandererPrefs_Icon_TextMode                     (MUIB_WandererPrefs | 0x000000D1) /* -SG */
#define MUIA_WandererPrefs_Icon_TextMaxLen                   (MUIB_WandererPrefs | 0x000000D2) /* -SG */

#define MUIA_WandererPrefs_Background                        (MUIB_WandererPrefs | 0x000000E0) /* I-G */

#define MUIA_WandererPrefs_Background_RenderMode             (MUIB_WandererPrefs | 0x000000E1) /* -SG */
#define MUIA_WandererPrefs_Background_TileMode               (MUIB_WandererPrefs | 0x000000E2) /* -SG */
#define MUIA_WandererPrefs_Background_XOffset                (MUIB_WandererPrefs | 0x000000E3) /* -SG */
#define MUIA_WandererPrefs_Background_YOffset                (MUIB_WandererPrefs | 0x000000E4) /* -SG */

/*** Public Methods *********************************************************/
#define MUIM_WandererPrefs_Reload                            (MUIB_WandererPrefs | 0x00000000)

#define MUIM_WandererPrefs_Background_GetNotifyObject        (MUIB_WandererPrefs | 0x000000E0) /* --G */
#define MUIM_WandererPrefs_Background_GetAttribute           (MUIB_WandererPrefs | 0x000000E1) /* --G */
struct  MUIP_WandererPrefs_Background_GetNotifyObject        {ULONG MethodID; char *Background_Name;};
struct  MUIP_WandererPrefs_Background_GetAttribute           {ULONG MethodID; char *Background_Name; ULONG AttributeID;};

#define WPD_BackgroundRenderMode_Tiled       				 1        // Default
#define WPD_BackgroundRenderMode_Scale       	             2        // Scaled to screen

#define WPD_BackgroundTileMode_Float       		             1        // Default (moves with window scrolling)
#define WPD_BackgroundTileMode_Fixed       		             2        // Tile's are fixed in the background

#define WP_GLOBALTAGCOUNT 6

/*** Variables **************************************************************/
extern struct MUI_CustomClass *WandererPrefs_CLASS;

/*** Macros *****************************************************************/
#define WandererPrefsObject BOOPSIOBJMACRO_START(WandererPrefs_CLASS->mcc_Class)

#endif /* _WANDERERPREFS_H_ */
