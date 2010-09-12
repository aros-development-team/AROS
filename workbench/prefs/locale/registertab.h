#ifndef _LOCALEREGISTEREDITOR_H_
#define _LOCALEREGISTEREDITOR_H_

/*
    Copyright © 2004-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_LocaleRegisterEditor           (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *LocaleRegister_CLASS;

/*** Macros *****************************************************************/
#define LocaleRegisterObject BOOPSIOBJMACRO_START(LocaleRegister_CLASS->mcc_Class)

#endif
