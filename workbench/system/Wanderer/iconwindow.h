#ifndef _ICONWINDOW_H_
#define _ICONWINDOW_H_

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

struct IconWindow_ActionMsg
{
    int type;
    Object *iconlist;
    int isroot;
    struct IconList_Click *click;
    struct IconList_Drop *drop;
    /* to be continued...*/
};

/*** Identifier Base ********************************************************/
#define MUIB_IconWindow               (TAG_USER | 0x10000000)

/*** Public Attributes ******************************************************/
#define MUIA_IconWindow_IsRoot        (MUIB_IconWindow | 0x00000000) /* I-G */
#define MUIA_IconWindow_Drawer        (MUIB_IconWindow | 0x00000001) /* I-G */
#define MUIA_IconWindow_ActionHook    (MUIB_IconWindow | 0x00000002) /* I-- */ /* Hook to call when some action happens */
#define MUIA_IconWindow_IsBackdrop    (MUIB_IconWindow | 0x00000003) /* ISG */ /* is Backdrop window ? */
#define MUIA_IconWindow_IconList      (MUIB_IconWindow | 0x00000004) /* --G */
#define MUIA_IconWindow_Toolbar_Enabled      (MUIB_IconWindow | 0x00000005) /* --G */

#define MUIA_IconWindow_Font          (MUIB_IconWindow | 0x00000010) /* ISG */

/*** Public Methods *********************************************************/
#define MUIM_IconWindow_Open          (MUIB_IconWindow | 0x00000000)
#define MUIM_IconWindow_UnselectAll   (MUIB_IconWindow | 0x00000001)

/*** Private Methods ********************************************************/
#define MUIM_IconWindow_DoubleClicked (MUIB_IconWindow | 0x00000002)
#define MUIM_IconWindow_IconsDropped  (MUIB_IconWindow | 0x00000003)
#define MUIM_IconWindow_Clicked       (MUIB_IconWindow | 0x00000004)
#define MUIM_IconWindow_DirectoryUp   (MUIB_IconWindow | 0x00000005)

#define ICONWINDOW_ACTION_OPEN 1
#define ICONWINDOW_ACTION_CLICK 2
#define ICONWINDOW_ACTION_ICONDROP 3
#define ICONWINDOW_ACTION_DIRUP 4

/*** Variables **************************************************************/
extern struct MUI_CustomClass *IconWindow_CLASS;

/*** Macros *****************************************************************/
#define IconWindowObject BOOPSIOBJMACRO_START(IconWindow_CLASS->mcc_Class)


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
                           m11, m11_msg_type)                        \
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
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)    \


#endif /* _ICONWINDOW_H_ */
