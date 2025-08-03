#ifndef _OPENURLREGISTEREDITOR_H_
#define _OPENURLREGISTEREDITOR_H_

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_OpenURLRegisterEditor           (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *OpenURLRegister_CLASS;

/*** Macros *****************************************************************/
#define OpenURLRegisterObject BOOPSIOBJMACRO_START(OpenURLRegister_CLASS->mcc_Class)

#endif
