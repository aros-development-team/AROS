#ifndef _FPEDITOR_H_
#define _FPEDITOR_H_

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_FPEditor                  (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *FPEditor_CLASS;

/*** Macros *****************************************************************/
#define FPEditorObject BOOPSIOBJMACRO_START(FPEditor_CLASS->mcc_Class)

#endif /* _FWPEDITOR_H_ */
