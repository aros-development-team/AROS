#ifndef _ASLSTRINGIFY_H_
#define _ASLSTRINGIFY_H_

/*
    Copyright © 2022, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_AsllsStringify               (TAG_USER | 0x20000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *AslStringify_CLASS;

#define AslStringifyObject BOOPSIOBJMACRO_START(AslStringify_CLASS->mcc_Class)

#endif /*_ASLSTRINGIFY_H_ */
