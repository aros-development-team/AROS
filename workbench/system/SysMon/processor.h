#ifndef _SYSMON_PROCESSORGRP_H_
#define _SYSMON_PROCESSORGRP_H_

/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>

/* default InfoText to display */
extern CONST_STRPTR CPU_DEFSTR;

/*** Identifier Base ********************************************************/
#define MUIB_ProcessorGrp                                               (TAG_USER | 0x20000000)

/*** Public Attributes ******************************************************/
#define MUIA_ProcessorGrp_SingleMode                                    (MUIB_ProcessorGrp | 0x00000001) // BOOL
#define MUIA_ProcessorGrp_CPUCount                                      (MUIB_ProcessorGrp | 0x00000002) // BOOL

/*** Public Methods *********************************************************/
#define MUIM_ProcessorGrp_Update                                        (MUIB_ProcessorGrp | 0x00000001)

/*** Private Methods ********************************************************/

/*** Public Constants ********************************************************/

/*** Private Constants ********************************************************/

/*** Macros *****************************************************************/

/* this macro is based on the ZUNE_CUSTOMCLASS_xx macros from zune/customclasses.h
and temporarily placed here */
#define PROCESSORGRP_CUSTOMCLASS(name, base, parent_name, parent_class,  \
                           m1, m1_msg_type,                          \
                           m2, m2_msg_type,                          \
                           m3, m3_msg_type,                          \
                           m4, m4_msg_type,                          \
                           m5, m5_msg_type)                          \
    __ZUNE_CUSTOMCLASS_START(name)                                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);    \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)    \

#endif /* _SYSMON_PROCESSORGRP_H_ */
