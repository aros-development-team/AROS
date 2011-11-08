#ifndef _ICONWINDOWICONLIST_H_
#define _ICONWINDOWICONLIST_H_

/*
    Copyright  2004-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier Base ********************************************************/
#define MUIB_IconWindowDrawerList                   (TAG_USER | 0x10000200)
#define MUIB_IconWindowVolumeList                   (TAG_USER | 0x10000300)
#define MUIB_IconWindowIconNetworkBrowserList       (TAG_USER | 0x10000A00)

/*** Public Attributes ******************************************************/
#define MUIA_IconWindowVolumeList_ShowNetwork       (MUIB_IconWindowVolumeList | 0x00000010)
#define MUIA_IconWindowVolumeList_ShowUserFiles     (MUIB_IconWindowVolumeList | 0x00000011)

/*** Public Methods *********************************************************/
#define MUIM_IconWindowDrawerList_FileSystemChanged (MUIB_IconWindowDrawerList | 0x00000001)
#define MUIM_IconWindowDrawerList_RateLimitRefresh  (MUIB_IconWindowDrawerList | 0x00000002)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *IconWindowDrawerList_CLASS;
extern struct MUI_CustomClass *IconWindowVolumeList_CLASS;
extern struct MUI_CustomClass *IconWindowIconNetworkBrowserList_CLASS;

/*** Macros *****************************************************************/
#ifdef __AROS__
#define IconWindowDrawerListObject BOOPSIOBJMACRO_START(IconWindowDrawerList_CLASS->mcc_Class)
#define IconWindowVolumeListObject BOOPSIOBJMACRO_START(IconWindowVolumeList_CLASS->mcc_Class)
#define IconWindowIconNetworkBrowserListObject BOOPSIOBJMACRO_START(IconWindowIconNetworkBrowserList_CLASS->mcc_Class)
#else
#define IconWindowDrawerListObject NewObject(IconWindowDrawerList_CLASS->mcc_Class, NULL
#define IconWindowVolumeListObject NewObject(IconWindowVolumeList_CLASS->mcc_Class, NULL
#define IconWindowIconNetworkBrowserListObject NewObject(IconWindowIconNetworkBrowserList_CLASS->mcc_Class, NULL
#endif

/* this macro is based on the ZUNE_CUSTOMCLASS_10 macros from zune/customclasses.h
and temporarily placed here */
#define ICONWINDOWICONDRAWERLIST_CUSTOMCLASS(name, base, parent_name, parent_class,   \
                           m1, m1_msg_type,                          \
                           m2, m2_msg_type,                          \
                           m3, m3_msg_type,                          \
                           m4, m4_msg_type,                          \
                           m5, m5_msg_type,                          \
                           m6, m6_msg_type,                          \
                           m7, m7_msg_type,                          \
                           m8, m8_msg_type)                          \
    __ZUNE_CUSTOMCLASS_START(name)                                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6, m6, m6_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m7, m7, m7_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m8, m8, m8_msg_type);    \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)    \

#define ICONWINDOWICONVOLUMELIST_CUSTOMCLASS(name, base, parent_name, parent_class,   \
                           m1, m1_msg_type,                          \
                           m2, m2_msg_type,                          \
                           m3, m3_msg_type,                          \
                           m4, m4_msg_type,                          \
                           m5, m5_msg_type,                          \
                           m6, m6_msg_type,                          \
                           m7, m7_msg_type,                          \
                           m8, m8_msg_type,                          \
                           m9, m9_msg_type,                          \
                           m10, m10_msg_type,                          \
                           m11, m11_msg_type)                          \
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
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m10, m10, m10_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m11, m11, m11_msg_type);    \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)    \

#define ICONWINDOWICONNETWORKBROWSERLIST_CUSTOMCLASS(name, base, parent_name, parent_class,   \
                           m1, m1_msg_type,                          \
                           m2, m2_msg_type,                          \
                           m3, m3_msg_type,                          \
                           m4, m4_msg_type,                          \
                           m5, m5_msg_type,                          \
                           m6, m6_msg_type,                          \
                           m7, m7_msg_type,                          \
                           m8, m8_msg_type)                          \
    __ZUNE_CUSTOMCLASS_START(name)                                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6, m6, m6_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m7, m7, m7_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m8, m8, m8_msg_type);    \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)    \

#endif /* _ICONWINDOWICONLIST_H_ */
