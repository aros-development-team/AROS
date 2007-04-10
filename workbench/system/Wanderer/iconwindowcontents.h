#ifndef _ICONWINDOWICONLIST_H_
#define _ICONWINDOWICONLIST_H_

/*
    Copyright  2004, The AROS Development Team. All rights reserved.
    $Id: iconwindowiconlist.h 25432 2007-03-14 18:05:52Z NicJA $
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier Base ********************************************************/
#define MUIB_IconWindowIconDrawerList               (TAG_USER | 0x10000200)
#define MUIB_IconWindowIconVolumeList               (TAG_USER | 0x10000300)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *IconWindowIconDrawerList_CLASS;
extern struct MUI_CustomClass *IconWindowIconVolumeList_CLASS;
	
/*** Macros *****************************************************************/
#define IconWindowIconDrawerListObject BOOPSIOBJMACRO_START(IconWindowIconDrawerList_CLASS->mcc_Class)
#define IconWindowIconVolumeListObject BOOPSIOBJMACRO_START(IconWindowIconVolumeList_CLASS->mcc_Class)


/* this macro is based on the ZUNE_CUSTOMCLASS_10 macros from zune/customclasses.h
and temporarily placed here */
#define ICONWINDOWICONDRAWERLIST_CUSTOMCLASS(name, funcnamebase, base, parent_name, parent_class,   \
                           m1, m1_msg_type,                          \
                           m2, m2_msg_type,                          \
                           m3, m3_msg_type,                          \
                           m4, m4_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                   \
    __ZUNE_CUSTOMCLASS_METHOD(funcnamebase ## __ ## m1, m1, m1_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(funcnamebase ## __ ## m2, m2, m2_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(funcnamebase ## __ ## m3, m3, m3_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(funcnamebase ## __ ## m4, m4, m4_msg_type);    \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)    \

#define ICONWINDOWICONVOLUMELIST_CUSTOMCLASS(name, funcnamebase, base, parent_name, parent_class,   \
                           m1, m1_msg_type,                          \
                           m2, m2_msg_type,                          \
                           m3, m3_msg_type,                          \
                           m4, m4_msg_type,                          \
                           m5, m5_msg_type,                          \
                           m6, m6_msg_type,                          \
                           m7, m7_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                   \
    __ZUNE_CUSTOMCLASS_METHOD(funcnamebase ## __ ## m1, m1, m1_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(funcnamebase ## __ ## m2, m2, m2_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(funcnamebase ## __ ## m3, m3, m3_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(funcnamebase ## __ ## m4, m4, m4_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(funcnamebase ## __ ## m5, m5, m5_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(funcnamebase ## __ ## m6, m6, m6_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(funcnamebase ## __ ## m7, m7, m7_msg_type);    \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)    \

#endif /* _ICONWINDOWICONLIST_H_ */
