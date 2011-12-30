#ifndef _ICONWINDOW_H_
#define _ICONWINDOW_H_

/*
    Copyright © 2004 - 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

#include "iconwindow_attributes.h"

#include "iconwindowbackfill.h"

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
#define MUIM_IconWindow_RateLimitRefresh                   (MUIB_IconWindow | 0x00000008)
#define MUIM_IconWindow_Snapshot                           (MUIB_IconWindow | 0x00000009)

#define MUIM_IconWindow_BackFill_Register                  (MUIB_IconWindow | 0x00000010)
#define MUIM_IconWindow_BackFill_Setup                     (MUIB_IconWindow | 0x00000012)
#define MUIM_IconWindow_BackFill_Cleanup                   (MUIB_IconWindow | 0x00000013)
#define MUIM_IconWindow_BackFill_ProcessBackground         (MUIB_IconWindow | 0x0000001a)
#define MUIM_IconWindow_BackFill_DrawBackground            (MUIB_IconWindow | 0x0000001b)

struct  MUIP_IconWindow_Snapshot                           {STACKED ULONG MethidID; STACKED BOOL snapshotall;};
struct  MUIP_IconWindow_BackFill_Register                  {STACKED ULONG MethodID; STACKED struct IconWindow_BackFill_Descriptor *register_Node;};
struct  MUIP_IconWindow_BackFill_Setup                     {STACKED ULONG MethodID;};
struct  MUIP_IconWindow_BackFill_Cleanup                   {STACKED ULONG MethodID; STACKED IPTR BackFill_Data;};
struct  MUIP_IconWindow_BackFill_ProcessBackground         {STACKED ULONG MethodID; STACKED IPTR BackFill_Data; STACKED Object *BackFill_Root;};
struct  MUIP_IconWindow_BackFill_DrawBackground            {STACKED ULONG MethodID; STACKED IPTR BackFill_Data; STACKED struct IconWindowBackFillMsg *draw_BFM; STACKED IPTR draw_RastPort;};
/*** Private Constants ********************************************************/

extern struct MUI_CustomClass                     *IconWindow_CLASS;

/*** Private Data Structures ********************************************************/

struct BackFillMsg
{
    STACKED struct Layer    *Layer;
    STACKED struct Rectangle Bounds;
    STACKED LONG             OffsetX;
    STACKED LONG             OffsetY;
};

struct IconWindowBackFillMsg
{
    STACKED struct Layer    *Layer;
    STACKED struct Rectangle AreaBounds;
    STACKED struct Rectangle DrawBounds;
    STACKED LONG             OffsetX;
    STACKED LONG             OffsetY;
};

struct IconWindow_ActionMsg
{
    STACKED int                         type;
    STACKED Object                      *iconlist;
    STACKED int                         isroot;
    STACKED struct IconList_Click       *click;
    STACKED struct IconList_Drop_Event  *drop;
};

struct IconWindow_BackFill_Descriptor
{
    struct Node      bfd_Node;
    char             *bfd_BackFillID;
    IPTR             (*bfd_MUIM_IconWindow_BackFill_Setup)(Class *, Object *, struct MUIP_IconWindow_BackFill_Setup *);
    IPTR             (*bfd_MUIM_IconWindow_BackFill_Cleanup)(Class *, Object *, struct MUIP_IconWindow_BackFill_Cleanup *);
    IPTR             (*bfd_MUIM_IconWindow_BackFill_ProcessBackground)(Class *, Object *, struct MUIP_IconWindow_BackFill_ProcessBackground *);
    IPTR             (*bfd_MUIM_IconWindow_BackFill_DrawBackground)(Class *, Object *, struct MUIP_IconWindow_BackFill_DrawBackground *);
};

/*** Private Instance Data **********************************************************/

struct IconWindow_BackFillHookData
{
    Class                                *bfhd_IWClass;
    Object                               *bfhd_IWObject;
};

struct IconWindow_Panel_Data
{
    Object                               *iwp_PanelContainerObj;
    Object                               *iwp_PanelGroupObj;
    Object                               *iwp_PanelGroupSpacerObj;
    IPTR                                 iwp_PanelPrivate;
};

struct IconWindow_DATA
{
    struct Screen                       *iwd_Screen;
    char                                *iwd_Title;
    char                                iwd_DirectoryPath[IWD_MAX_DIRECTORYPATHLEN];

    char                                *iwd_ViewSettings_Attrib;
    Object                              *iwd_ViewSettings_PrefsNotificationObject;

    Object                              *iwd_RootViewObj;
    Object                              *iwd_IconListObj;

    IPTR                                iwd_FSNotifyPort;
    struct IconWindow_Panel_Data        iwd_TopPanel;
    struct IconWindow_Panel_Data        iwd_LeftPanel;
    struct IconWindow_Panel_Data        iwd_BottomPanel;

#ifdef __AROS__
    struct Hook                         iwd_PrefsUpdated_hook;
    struct Hook                         iwd_ProcessBackground_hook;
#else
    struct Hook                         *iwd_PrefsUpdated_hook;
    struct Hook                         *iwd_ProcessBackground_hook;
#endif

    struct Hook                         *iwd_ActionHook;

    struct Hook                         *iwd_BackFill_hook;
    struct BackFillInfo                 *iwd_BackFillInfo;
    struct IconWindow_BackFillHookData  iwd_BackFillHookData;
    
    struct TextFont                     *iwd_WindowFont;

#define IWDFLAG_SETUP                   (1<<0)
#define IWDFLAG_NEEDSUPDATE             (1<<1)
#define IWDFLAG_ISROOT                  (1<<4)
#define IWDFLAG_ISBACKDROP              (1<<5)
    UBYTE                                iwd_Flags;
    UBYTE                                iwd_VolViewMode;
};

/*** Macros *****************************************************************/

#define SETUP_ICONWINDOW_INST_DATA       struct IconWindow_DATA *data = INST_DATA(CLASS, self)

#ifdef __AROS__
#define IconWindowObject                 BOOPSIOBJMACRO_START(IconWindow_CLASS->mcc_Class)
#else
#define IconWindowObject NewObject(IconWindow_CLASS->mcc_Class, NULL
//#define IconWindowObject NewObject(IconWindow_Class->mcc_Class, NULL
#endif

struct iconWindow_Extension
{
    struct Node      iwe_Node;
    IPTR             (*iwe_Setup)(Class *, Object *, struct opSet *);
    IPTR             (*iwe_Cleanup)(Class *, Object *, Msg);
    IPTR             (*iwe_Set)(Class *, Object *, struct opSet *);
    IPTR             (*iwe_Get)(Class *, Object *, struct opGet *);
};

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
                           m18, m18_msg_type,                        \
                           m19, m19_msg_type,                        \
                           m20, m20_msg_type,                        \
                           m21, m21_msg_type)                        \
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
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m19, m19, m19_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m20, m20, m20_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m21, m21, m21_msg_type); \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)    \

IPTR IconWindow__MUIM_IconWindow_BackFill_Register(Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_Register *message);

#endif /* _ICONWINDOW_H_ */
