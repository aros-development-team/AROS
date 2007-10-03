#ifndef _MUI_CLASSES_DIRLIST_H
#define _MUI_CLASSES_DIRLIST_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Dirlist "Dirlist.mui"

/*** Identifer base (for Zune extensions) ***********************************/
#define MUIB_Dirlist (MUIB_ZUNE | 0x00001800)  

/*** Methods ****************************************************************/

#define MUIM_Dirlist_ReRead         (MUIB_MUI|0x00422d71) /* MUI: V4  */
struct  MUIP_Dirlist_ReRead         {STACKED ULONG MethodID;};

/*** Attributes *************************************************************/
#define MUIA_Dirlist_AcceptPattern  (MUIB_MUI|0x0042760a) /* MUI: V4  is. STRPTR        */
#define MUIA_Dirlist_Directory      (MUIB_MUI|0x0042ea41) /* MUI: V4  isg STRPTR        */
#define MUIA_Dirlist_DrawersOnly    (MUIB_MUI|0x0042b379) /* MUI: V4  is. BOOL          */
#define MUIA_Dirlist_FilesOnly      (MUIB_MUI|0x0042896a) /* MUI: V4  is. BOOL          */
#define MUIA_Dirlist_FilterDrawers  (MUIB_MUI|0x00424ad2) /* MUI: V4  is. BOOL          */
#define MUIA_Dirlist_FilterHook     (MUIB_MUI|0x0042ae19) /* MUI: V4  is. struct Hook * */
#define MUIA_Dirlist_MultiSelDirs   (MUIB_MUI|0x00428653) /* MUI: V6  is. BOOL          */
#define MUIA_Dirlist_NumBytes       (MUIB_MUI|0x00429e26) /* MUI: V4  ..g LONG          */
#define MUIA_Dirlist_NumDrawers     (MUIB_MUI|0x00429cb8) /* MUI: V4  ..g LONG          */
#define MUIA_Dirlist_NumFiles       (MUIB_MUI|0x0042a6f0) /* MUI: V4  ..g LONG          */
#define MUIA_Dirlist_Path           (MUIB_MUI|0x00426176) /* MUI: V4  ..g STRPTR        */
#define MUIA_Dirlist_RejectIcons    (MUIB_MUI|0x00424808) /* MUI: V4  is. BOOL          */
#define MUIA_Dirlist_RejectPattern  (MUIB_MUI|0x004259c7) /* MUI: V4  is. STRPTR        */
#define MUIA_Dirlist_SortDirs       (MUIB_MUI|0x0042bbb9) /* MUI: V4  is. LONG          */
#define MUIA_Dirlist_SortHighLow    (MUIB_MUI|0x00421896) /* MUI: V4  is. BOOL          */
#define MUIA_Dirlist_SortType       (MUIB_MUI|0x004228bc) /* MUI: V4  is. LONG          */
#define MUIA_Dirlist_Status         (MUIB_MUI|0x004240de) /* MUI: V4  ..g LONG          */

enum {
    MUIV_Dirlist_SortDirs_First = 0,
    MUIV_Dirlist_SortDirs_Last,
    MUIV_Dirlist_SortDirs_Mix,
};

enum {
    MUIV_Dirlist_SortType_Name = 0,
    MUIV_Dirlist_SortType_Date,
    MUIV_Dirlist_SortType_Size,
};

enum {
    MUIV_Dirlist_Status_Invalid = 0,
    MUIV_Dirlist_Status_Reading,
    MUIV_Dirlist_Status_Valid,
};

extern const struct __MUIBuiltinClass _MUI_Dirlist_desc; /* PRIV */

#endif /* _MUI_CLASSES_DIRLIST_H */
