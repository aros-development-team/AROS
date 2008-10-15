#ifndef _IPEDITOR_H_
#define _IPEDITOR_H_

/*
    Copyright  2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_SerEditor                  (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *LocaleRegister_CLASS;
extern struct SerialPrefs      serialprefs;


/*** Macros *****************************************************************/
#define LocaleRegisterObject BOOPSIOBJMACRO_START(LocaleRegister_CLASS->mcc_Class)

#define MUIA_MyStringifyType        0x8001

#define STRINGIFY_DoubleClickDelay  0x00
#define STRINGIFY_RepeatDelay       0x01
#define STRINGIFY_RepeatRate        0x02

#endif /* _IPEDITOR_H_ */
