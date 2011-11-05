#ifndef _ICONWINDOW_ATTRIBUTES_H_
#define _ICONWINDOW_ATTRIBUTES_H_

/*
    Copyright  2004 - 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Identifier Base ********************************************************/
#define MUIB_IconWindow                                         (TAG_USER | 0x10000000)

/*** Public Attributes ******************************************************/
#define MUIA_IconWindow_Location                                (MUIB_IconWindow | 0x00000001) /* ISG */
#define MUIA_IconWindow_Window                                  (MUIB_IconWindow | 0x00000002) /* I-G */
#define MUIA_IconWindow_Font                                    (MUIB_IconWindow | 0x00000003) /* ISG */
#define MUIA_IconWindow_IconList                                (MUIB_IconWindow | 0x00000004) /* --G */
#define MUIA_IconWindow_WindowNavigationMethod                  (MUIB_IconWindow | 0x00000005) /* ISG */

#define MUIA_IconWindow_ActionHook                              (MUIB_IconWindow | 0x00000010) /* I-- */ /* Hook to call when some action happens */

#define MUIA_IconWindow_BackgroundAttrib                        (MUIB_IconWindow | 0x00000020) /* --G */
#define MUIA_IconWindow_BackFillData                            (MUIB_IconWindow | 0x00000021) /* --G */

#define MUIA_IconWindow_IsRoot                                  (MUIB_IconWindow | 0x000000A1) /* I-G */
#define MUIA_IconWindow_IsBackdrop                              (MUIB_IconWindow | 0x000000A2) /* ISG */ /* is Backdrop window ? */

#define MUIA_IconWindow_Changed                                 (MUIB_IconWindow | 0x000000FF) /* -SG (TRUE) if the window(s) settings
                                                                                                 have changed (ie window needs refereshed)
                                                                                                 used in combination with MUIA_WandererPrefs_Processing
                                                                                                 to determine if we need to redraw */

#define MUIA_IconWindow_VolumeInfoMode                          (MUIB_IconWindow | 0x000000B1)

#define MUIV_IconWindow_VolumeInfoMode_ShowOnlyIcons            1
#define MUIV_IconWindow_VolumeInfoMode_ShowAll                  2
#define MUIV_IconWindow_VolumeInfoMode_ShowAllIfNoInfo          3


#define ICONWINDOW_ACTION_OPEN                                  1
#define ICONWINDOW_ACTION_CLICK                                 2
#define ICONWINDOW_ACTION_ICONDROP                              3
#define ICONWINDOW_ACTION_DIRUP                                 4
#define ICONWINDOW_ACTION_APPWINDOWDROP                         5

/* TODO: ImageBackFills Attributes etc should be in an own file */
/*** Identifier Base ********************************************************/
#define MUIB_IconWindowExt                                      (MUIB_IconWindow    | 0x0f000000)
#define MUIB_IconWindowExt_ImageBackFill                        (MUIB_IconWindowExt | 0x100000)

/*** Public Attributes ******************************************************/

#define MUIA_IconWindowExt_ImageBackFill_BGRenderMode           (MUIB_IconWindowExt_ImageBackFill | 0x00000001) /* ISG */
#define MUIA_IconWindowExt_ImageBackFill_BGTileMode             (MUIB_IconWindowExt_ImageBackFill | 0x00000002) /* ISG */
#define MUIA_IconWindowExt_ImageBackFill_BGXOffset              (MUIB_IconWindowExt_ImageBackFill | 0x00000003) /* ISG */
#define MUIA_IconWindowExt_ImageBackFill_BGYOffset              (MUIB_IconWindowExt_ImageBackFill | 0x00000004) /* ISG */

/*** Private Constants ********************************************************/

#define IconWindowExt_ImageBackFill_RenderMode_Tiled            1        // Default
#define IconWindowExt_ImageBackFill_RenderMode_Scale            2        // Scaled to screen

#define IconWindowExt_ImageBackFill_TileMode_Float              1        // Default (moves with window scrolling)
#define IconWindowExt_ImageBackFill_TileMode_Fixed              2        // Tile's are fixed in the background

/* TODO: NetworkBrowser Attributes etc should be in an own file */
/*** Identifier Base ********************************************************/
#define MUIB_IconWindowExt_NetworkBrowser                       (MUIB_IconWindowExt | 0x300000)

#define MUIA_IconWindowExt_NetworkBrowser_Show                  (MUIB_IconWindowExt_NetworkBrowser | 0x00000001) /* ISG */
/*** Variables **************************************************************/

/* TODO: UserFiles Attributes etc should be in an own file */
/*** Identifier Base ********************************************************/
#define MUIB_IconWindowExt_UserFiles                            (MUIB_IconWindowExt | 0x400000)

#define MUIA_IconWindowExt_UserFiles_ShowFilesFolder            (MUIB_IconWindowExt_UserFiles | 0x00000001) /* ISG */
#define MUIA_IconWindowExt_UserFiles_UseDesktopFolder           (MUIB_IconWindowExt_UserFiles | 0x00000002) /* ISG */

#define MUIA_IconWindowExt_ScreenTitle_String                   (MUIB_IconWindowExt_UserFiles | 0x00000003) /* ISG */

#define IWD_MAX_DIRECTORYPATHLEN                                1024

#endif /* _ICONWINDOW_ATTRIBUTES_H_ */
