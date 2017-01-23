#ifndef _SYSMON_TASKLIST_H_
#define _SYSMON_TASKLIST_H_

/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>

/*** Identifier Base ********************************************************/
#define MUIB_Tasklist                                            (TAG_USER | 0x10000000)

/*** Public Attributes ******************************************************/
#define MUIA_Tasklist_Refreshed                                  (MUIB_Tasklist | 0x00000000)
#define MUIA_Tasklist_RefreshMSecs                               (MUIB_Tasklist | 0x00000001)
#define MUIA_Tasklist_ReadyCount                                 (MUIB_Tasklist | 0x00000002)
#define MUIA_Tasklist_WaitingCount                               (MUIB_Tasklist | 0x00000003)

/*** Public Methods *********************************************************/
#define MUIM_Tasklist_Refresh                                    (MUIB_Tasklist | 0x00000000)

/*** Private Methods ********************************************************/
#define MUIM_Tasklist_HandleTimer                                (MUIB_Tasklist | 0x00000001)

/*** Public Constants ********************************************************/
#define MUIV_Tasklist_Refresh_Slow                               2000
#define MUIV_Tasklist_Refresh_Normal                             1000
#define MUIV_Tasklist_Refresh_Fast                               500

/*** Private Constants ********************************************************/

extern struct MUI_CustomClass                     *Tasklist_CLASS;

/*** Macros *****************************************************************/

#define TasklistObject                 BOOPSIOBJMACRO_START(Tasklist_CLASS->mcc_Class)

/* this macro is based on the ZUNE_CUSTOMCLASS_xx macros from zune/customclasses.h
and temporarily placed here */
#define TASKLIST_CUSTOMCLASS(name, base, parent_name, parent_class,  \
                           m1, m1_msg_type,                          \
                           m2, m2_msg_type,                          \
                           m3, m3_msg_type,                          \
                           m4, m4_msg_type,                          \
                           m5, m5_msg_type,                          \
                           m6, m6_msg_type,                          \
                           m7, m7_msg_type,                          \
                           m8, m8_msg_type,                          \
                           m9, m9_msg_type)                          \
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
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)    \

#endif /* _SYSMON_TASKLIST_H_ */
