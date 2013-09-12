#ifndef _REQTOOLSSTRINGIFY_H_
#define _REQTOOLSSTRINGIFY_H_

/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_ReqToolsStringify               (TAG_USER | 0x20000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *ReqToolsStringify_CLASS;

#define ReqToolsStringifyObject BOOPSIOBJMACRO_START(ReqToolsStringify_CLASS->mcc_Class)

#endif /*_REQTOOLSSTRINGIFY_H_ */
