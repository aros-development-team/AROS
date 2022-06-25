#ifndef _ASLEDITOR_H_
#define _ASLEDITOR_H_

/*
    Copyright © 2022, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_AslEditor                  (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *AslEditor_CLASS;

#define AslEditorObject BOOPSIOBJMACRO_START(AslEditor_CLASS->mcc_Class)

#endif /*_ASLEDITOR_H_ */
