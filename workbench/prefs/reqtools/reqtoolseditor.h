#ifndef _REQTOOLSEDITOR_H_
#define _REQTOOLSEDITOR_H_

/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_ReqToolsEditor                  (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *ReqToolsEditor_CLASS;

#define ReqToolsEditorObject BOOPSIOBJMACRO_START(ReqToolsEditor_CLASS->mcc_Class)

#endif /*_REQTOOLSEDITOR_H_ */
