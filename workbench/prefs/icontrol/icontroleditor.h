#ifndef _ICONTROLEDITOR_H_
#define _ICONTROLEDITOR_H_

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_IControlEditor                  (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *IControlEditor_CLASS;

#define IControlEditorObject BOOPSIOBJMACRO_START(IControlEditor_CLASS->mcc_Class)

#endif /* _ICONTROLEDITOR_H_ */
