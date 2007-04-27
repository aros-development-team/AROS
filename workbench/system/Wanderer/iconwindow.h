#ifndef _ICONWINDOW_H_
#define _ICONWINDOW_H_

/*
    Copyright  2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

#include "iconwindowbackfill.h"

/*** Identifier Base ********************************************************/

#define MUIB_IconWindow                           (TAG_USER | 0x10000000)

/*** Public Attributes ******************************************************/

#define MUIA_IconWindow_Location                           (MUIB_IconWindow | 0x00000001) /* ISG */
#define MUIA_IconWindow_Window                             (MUIB_IconWindow | 0x00000002) /* I-G */
#define MUIA_IconWindow_Font                               (MUIB_IconWindow | 0x00000003) /* ISG */
#define MUIA_IconWindow_IconList                           (MUIB_IconWindow | 0x00000004) /* --G */

#define MUIA_IconWindow_ActionHook                         (MUIB_IconWindow | 0x00000010) /* I-- */ /* Hook to call when some action happens */

#define MUIA_IconWindow_BackgroundAttrib                   (MUIB_IconWindow | 0x00000020) /* --G */
#define MUIA_IconWindow_BackFillData                       (MUIB_IconWindow | 0x00000021) /* --G */

#define MUIA_IconWindow_IsRoot                             (MUIB_IconWindow | 0x000000A1) /* I-G */
#define MUIA_IconWindow_IsBackdrop                         (MUIB_IconWindow | 0x000000A2) /* ISG */ /* is Backdrop window ? */
#define MUIA_IconWindow_Toolbar_Enabled                    (MUIB_IconWindow | 0x000000E1) /* ISG */

#define MUIA_IconWindow_Changed                            (MUIB_IconWindow | 0x000000FF) /* -SG (TRUE) if the window(s) settings
																								 have changed (ie window needs refereshed)
																								 used in combination with MUIA_WandererPrefs_Processing
																								 to determine if we need to redraw */

/*** Public Methods *********************************************************/

#define MUIM_IconWindow_Open                               (MUIB_IconWindow | 0x00000000)
#define MUIM_IconWindow_UnselectAll                        (MUIB_IconWindow | 0x00000001)

/*** Private Methods ********************************************************/

#define MUIM_IconWindow_DoubleClicked                      (MUIB_IconWindow | 0x00000002)
#define MUIM_IconWindow_IconsDropped                       (MUIB_IconWindow | 0x00000003)
#define MUIM_IconWindow_Clicked                            (MUIB_IconWindow | 0x00000004)
#define MUIM_IconWindow_DirectoryUp                        (MUIB_IconWindow | 0x00000005)
#define MUIM_IconWindow_AppWindowDrop                      (MUIB_IconWindow | 0x00000006)
#define MUIM_IconWindow_Remove                             (MUIB_IconWindow | 0x00000007)

#define MUIM_IconWindow_BackFill_Register		           (MUIB_IconWindow | 0x00000010)
#define MUIM_IconWindow_BackFill_Setup		               (MUIB_IconWindow | 0x00000012)
#define MUIM_IconWindow_BackFill_Cleanup                   (MUIB_IconWindow | 0x00000013)
#define MUIM_IconWindow_BackFill_ProcessBackground         (MUIB_IconWindow | 0x0000001a)
#define MUIM_IconWindow_BackFill_DrawBackground            (MUIB_IconWindow | 0x0000001b)

struct  MUIP_IconWindow_BackFill_Register                  {ULONG MethodID; struct IconWindow_BackFill_Descriptor *register_Node;};
struct  MUIP_IconWindow_BackFill_Setup                     {ULONG MethodID;};
struct  MUIP_IconWindow_BackFill_Cleanup                   {ULONG MethodID; IPTR BackFill_Data;};
struct  MUIP_IconWindow_BackFill_ProcessBackground         {ULONG MethodID; IPTR BackFill_Data; Object *BackFill_Root;};
struct  MUIP_IconWindow_BackFill_DrawBackground            {ULONG MethodID; IPTR BackFill_Data; struct IconWindowBackFillMsg *draw_BFM; IPTR draw_RastPort;};
/*** Private Constants ********************************************************/

#define ICONWINDOW_ACTION_OPEN                    1
#define ICONWINDOW_ACTION_CLICK                   2
#define ICONWINDOW_ACTION_ICONDROP                3
#define ICONWINDOW_ACTION_DIRUP                   4
#define ICONWINDOW_ACTION_APPWINDOWDROP           5

/*** Identifier Base ********************************************************/

#define MUIB_IconWindow_ImageBackFill                      (MUIB_IconWindow | 0x0fe00000)

/*** Public Attributes ******************************************************/

#define MUIA_IconWindow_ImageBackFill_BGRenderMode         (MUIB_IconWindow | 0x00000001) /* ISG */
#define MUIA_IconWindow_ImageBackFill_BGTileMode           (MUIB_IconWindow | 0x00000002) /* ISG */
#define MUIA_IconWindow_ImageBackFill_BGXOffset            (MUIB_IconWindow | 0x00000003) /* ISG */
#define MUIA_IconWindow_ImageBackFill_BGYOffset            (MUIB_IconWindow | 0x00000004) /* ISG */


/*** Variables **************************************************************/

extern struct MUI_CustomClass                     *IconWindow_CLASS;

/*** Private Data Structures ********************************************************/

struct BackFillMsg
{
	struct Layer    *Layer;
	struct Rectangle Bounds;
	LONG             OffsetX;
	LONG             OffsetY;
};

struct IconWindowBackFillMsg
{
	struct Layer    *Layer;
	struct Rectangle AreaBounds;
	struct Rectangle DrawBounds;
	LONG             OffsetX;
	LONG             OffsetY;
};

struct IconWindow_ActionMsg
{
    int type;
    Object *iconlist;
    int isroot;
    struct IconList_Click *click;
    struct IconList_Drop *drop;
    /* to be continued...*/
};

struct IconWindow_BackFill_Descriptor
{
	struct Node      bfd_Node;
	char             *bfd_BackFillID;
	IPTR             (*bfd_MUIM_IconWindow_BackFill_Setup)(Class *, Object *, struct MUIP_IconWindow_BackFill_Setup *);
	IPTR             (*bfd_MUIM_IconWindow_BackFill_Cleanup)(Class *, Object *, struct MUIP_IconWindow_BackFill_Cleanup *);
	IPTR             (*bfd_MUIM_IconWindow_BackFill_ProcessBackground)(Class *, Object *, struct MUIP_IconWindow_BackFill_ProcessBackground *);
	IPTR		     (*bfd_MUIM_IconWindow_BackFill_DrawBackground)(Class *, Object *, struct MUIP_IconWindow_BackFill_DrawBackground *);
};

/*** Private Instance Data **********************************************************/

struct IconWindow_BackFillHookData
{
	Class                                *bfhd_IWClass;
	Object                               *bfhd_IWObject;
};

#define IWD_MAX_DIRECTORYPATHLEN         1024

struct IconWindow_DATA
{
	char                                 *iwd_Title;
    char                                 iwd_DirectoryPath[IWD_MAX_DIRECTORYPATHLEN];

	char                                 *iwd_ViewSettings_Attrib;
	Object                               *iwd_ViewSettings_PrefsNotificationObject;

	Object                               *iwd_RootViewObj;
    Object                               *iwd_IconListObj;

    Object                               *iwd_ExtensionContainerObj;
    Object                               *iwd_ExtensionGroupObj;
    Object                               *iwd_ExtensionGroupSpacerObj;

	Object                               *iwd_Toolbar_PrefsNotificationObject;

    Object                               *iwd_Toolbar_PanelObj;
    Object                               *iwd_Toolbar_LocationStringObj;

	struct Hook                          iwd_PrefsUpdated_hook;
    struct Hook                          *iwd_ActionHook;
    struct Hook                          iwd_pathStrHook;
	struct Hook		                     iwd_ProcessBackground_hook;
	struct Hook                          *iwd_BackFill_hook;
	struct BackFillInfo                  *iwd_BackFillInfo;
	struct IconWindow_BackFillHookData   iwd_BackFillHookData;
	
    struct TextFont                      *iwd_WindowFont;
    
    BOOL                                 iwd_Flag_NEEDSUPDATE;
    BOOL                                 iwd_Flag_ISROOT;
    BOOL                                 iwd_Flag_ISBACKDROP;
    BOOL                                 iwd_Flag_EXT_TOOLBARENABLED;
};

/*** Macros *****************************************************************/

#define SETUP_ICONWINDOW_INST_DATA       struct IconWindow_DATA *data = INST_DATA(CLASS, self)

#define IconWindowObject                 BOOPSIOBJMACRO_START(IconWindow_CLASS->mcc_Class)


/* this macro is based on the ZUNE_CUSTOMCLASS_10 macros from zune/customclasses.h
and temporarily placed here */
#define ICONWINDOW_CUSTOMCLASS(name, base, parent_name, parent_class,   \
                           m1, m1_msg_type,                          \
                           m2, m2_msg_type,                          \
                           m3, m3_msg_type,                          \
                           m4, m4_msg_type,                          \
                           m5, m5_msg_type,                          \
                           m6, m6_msg_type,                          \
                           m7, m7_msg_type,                          \
                           m8, m8_msg_type,                          \
                           m9, m9_msg_type,                          \
                           m10, m10_msg_type,                        \
                           m11, m11_msg_type,                        \
                           m12, m12_msg_type,                        \
                           m13, m13_msg_type,                        \
                           m14, m14_msg_type,                        \
                           m15, m15_msg_type,                        \
                           m16, m16_msg_type,                        \
                           m17, m17_msg_type,                        \
                           m18, m18_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6, m6, m6_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m7, m7, m7_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m8, m8, m8_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m9, m9, m9_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m10, m10, m10_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m11, m11, m11_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m12, m12, m12_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m13, m13, m13_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m14, m14, m14_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m15, m15, m15_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m16, m16, m16_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m17, m17, m17_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m18, m18, m18_msg_type); \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)    \

IPTR IconWindow__MUIM_IconWindow_BackFill_Register(Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_Register *message);

#endif /* _ICONWINDOW_H_ */
