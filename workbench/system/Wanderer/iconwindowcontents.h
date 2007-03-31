#ifndef _ICONWINDOWCONTENTS_H_
#define _ICONWINDOWCONTENTS_H_

/*
    Copyright  2004, The AROS Development Team. All rights reserved.
    $Id: iconwindowcontents.h 25432 2007-03-14 18:05:52Z NicJA $
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier Base ********************************************************/
#define MUIB_IconWindowContents               (TAG_USER | 0x10000200)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *IconWindowContents_CLASS;

/*** Macros *****************************************************************/
#define IconWindowContentsObject BOOPSIOBJMACRO_START(IconWindowContents_CLASS->mcc_Class)


/* this macro is based on the ZUNE_CUSTOMCLASS_10 macros from zune/customclasses.h
and temporarily placed here */
#define ICONWINDOWCONTENTS_CUSTOMCLASS(name, base, parent_name, parent_class,   \
                           m1, m1_msg_type,                          \
                           m2, m2_msg_type,                          \
                           m3, m3_msg_type,                          \
                           m4, m4_msg_type,                          \
                           m5, m5_msg_type,                          \
                           m6, m6_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6, m6, m6_msg_type);    \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)    \


#endif /* _ICONWINDOWCONTENTS_H_ */
